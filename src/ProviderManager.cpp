/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/ProviderManager.hpp"
#include "bedrock/Exception.hpp"
#include "bedrock/ModuleContext.hpp"
#include "bedrock/AbstractServiceFactory.hpp"

#include "ProviderManagerImpl.hpp"

#include <thallium/serialization/stl/string.hpp>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <cctype>

namespace tl = thallium;

namespace bedrock {

using namespace std::string_literals;
using nlohmann::json;

ProviderManager::ProviderManager(const MargoContext& margo)
: self(std::make_shared<ProviderManagerImpl>()) {
    self->m_margo_context = margo;
}

ProviderManager::ProviderManager(const ProviderManager&) = default;

ProviderManager::ProviderManager(ProviderManager&&) = default;

ProviderManager& ProviderManager::operator=(const ProviderManager&) = default;

ProviderManager& ProviderManager::operator=(ProviderManager&&) = default;

ProviderManager::~ProviderManager() = default;

ProviderManager::operator bool() const { return static_cast<bool>(self); }

static auto resolveSpec(ProviderManagerImpl* manager, const std::string& spec)
    -> decltype(manager->m_providers.begin()) {
    auto                                   column = spec.find('.');
    decltype(manager->m_providers.begin()) it;
    if (column == std::string::npos) {
        it = std::find_if(
            manager->m_providers.begin(), manager->m_providers.end(),
            [&spec](const ProviderWrapper& item) { return item.name == spec; });
    } else {
        auto     type            = spec.substr(0, column);
        auto     provider_id_str = spec.substr(column + 1);
        uint16_t provider_id     = atoi(provider_id_str.c_str());
        it                       = std::find_if(
            manager->m_providers.begin(), manager->m_providers.end(),
            [&type, &provider_id](const ProviderWrapper& item) {
                return item.type == type && item.provider_id == provider_id;
            });
    }
    return it;
}

bool ProviderManager::lookupProvider(const std::string& spec,
                                     ProviderWrapper*   wrapper) const {
    std::lock_guard<tl::mutex> lock(self->m_providers_mtx);
    auto                       it = resolveSpec(self.get(), spec);
    if (it == self->m_providers.end()) { return false; }
    if (wrapper) { *wrapper = *it; }
    return true;
}

void ProviderManager::registerProvider(
    const ProviderDescriptor& descriptor, const std::string& pool_name,
    const std::string&                            config,
    const std::unordered_map<std::string, void*>& dependencies) {
    if (descriptor.name.empty()) {
        throw Exception("Provider name cannot be empty");
    }

    auto service_factory = ModuleContext::getServiceFactory(descriptor.type);
    if (!service_factory) {
        throw Exception(
            "Could not find service factory for provider type \"{}\"",
            descriptor.type);
    }
    spdlog::trace("Found provider \"{}\" to be of type \"{}\"", descriptor.name,
                  descriptor.type);

    {
        std::lock_guard<tl::mutex> lock(self->m_providers_mtx);
        auto it = resolveSpec(self.get(), descriptor.name);
        if (it != self->m_providers.end()) {
            throw Exception(
                "Could not register provider: a provider with the name \"{}\""
                " is already registered",
                descriptor.name);
        }

        it = resolveSpec(self.get(),
                         descriptor.type + ":"
                             + std::to_string(descriptor.provider_id));
        if (it != self->m_providers.end()) {
            throw Exception(
                "Could not register provider: a provider with the type \"{}\""
                " and provider id {} is already registered",
                descriptor.type, descriptor.provider_id);
        }

        ProviderWrapper wrapper;
        wrapper.name        = descriptor.name;
        wrapper.type        = descriptor.type;
        wrapper.provider_id = descriptor.provider_id;
        wrapper.factory     = service_factory;

        auto margoCtx = MargoContext(self->m_margo_context);

        FactoryArgs args;
        args.name         = descriptor.name;
        args.mid          = margoCtx.getMargoInstance();
        args.pool         = margoCtx.getPool(pool_name);
        args.config       = config;
        args.dependencies = dependencies;

        if (args.pool == ABT_POOL_NULL || args.pool == nullptr) {
            throw Exception("Could not find pool \"{}\"", pool_name);
        }

        spdlog::info("Registering provider {} of type {} with provider id {}",
                     descriptor.name, descriptor.type, descriptor.provider_id);
        wrapper.handle = service_factory->registerProvider(args);

        self->m_providers.push_back(wrapper);
    }
    self->m_providers_cv.notify_all();
}

void ProviderManager::deregisterProvider(const std::string& spec) {
    std::lock_guard<tl::mutex> lock(self->m_providers_mtx);
    auto                       it = resolveSpec(self.get(), spec);
    if (it == self->m_providers.end()) {
        throw Exception("Could not find provider for spec \"{}\"", spec);
    }
    auto& provider = *it;
    spdlog::info("Deregistering provider {}", provider.name);
    provider.factory->deregisterProvider(provider.handle);
}

void ProviderManager::addProviderFromJSON(const std::string& jsonString) {
    auto config = json::parse(jsonString);
    if (!config.is_object()) {
        throw Exception(
            "Invalid JSON configuration passed to "
            "ProviderManager::addProviderFromJSON (should be an object)");
    }

    ProviderDescriptor descriptor;

    descriptor.name        = config.value("name", "");
    descriptor.provider_id = config.value("provider_id", (uint16_t)0);

    auto type_it = config.find("type");
    if (type_it == config.end()) {
        throw Exception("No type provided for provider in JSON configuration");
    }
    descriptor.type = type_it->get<std::string>();

    auto provider_config    = "{}"s;
    auto provider_config_it = config.find("config");
    if (provider_config_it != config.end()) {
        provider_config = provider_config_it->dump();
    }

    auto pool_it = config.find("pool");
    if (pool_it == config.end()) {
        throw Exception("No pool specified for provider in JSON configuration");
    }
    auto pool_name = pool_it->get<std::string>();

    // TODO resolve dependencies
    std::unordered_map<std::string, void*> dependencies;

    registerProvider(descriptor, pool_name, provider_config, dependencies);
}

void ProviderManager::addProviderListFromJSON(const std::string& jsonString) {
    auto config = json::parse(jsonString);
    if (config.is_null()) { return; }
    if (!config.is_array()) {
        throw Exception(
            "Invalid JSON configuration passed to "
            "ProviderManager::addProviderListFromJSON (expected array)");
    }
    for (const auto& provider : config) {
        addProviderFromJSON(provider.dump());
    }
}

} // namespace bedrock
