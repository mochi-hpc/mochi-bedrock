/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/ProviderManager.hpp"
#include "bedrock/Exception.hpp"
#include "bedrock/ModuleContext.hpp"
#include "bedrock/AbstractServiceFactory.hpp"
#include "bedrock/DependencyFinder.hpp"

#include "ProviderManagerImpl.hpp"

#include <thallium/serialization/stl/string.hpp>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <cctype>

namespace tl = thallium;

namespace bedrock {

using namespace std::string_literals;
using nlohmann::json;

ProviderManager::ProviderManager(const MargoContext& margo,
                                 uint16_t provider_id, ABT_pool pool)
: self(std::make_shared<ProviderManagerImpl>(margo.getThalliumEngine(),
                                             provider_id, tl::pool(pool))) {
    self->m_margo_context = margo;
}

ProviderManager::ProviderManager(const ProviderManager&) = default;

ProviderManager::ProviderManager(ProviderManager&&) = default;

ProviderManager& ProviderManager::operator=(const ProviderManager&) = default;

ProviderManager& ProviderManager::operator=(ProviderManager&&) = default;

ProviderManager::~ProviderManager() = default;

ProviderManager::operator bool() const { return static_cast<bool>(self); }

bool ProviderManager::lookupProvider(const std::string& spec,
                                     ProviderWrapper*   wrapper) const {
    std::lock_guard<tl::mutex> lock(self->m_providers_mtx);
    auto                       it = self->resolveSpec(spec);
    if (it == self->m_providers.end()) { return false; }
    if (wrapper) { *wrapper = *it; }
    return true;
}

std::vector<ProviderDescriptor> ProviderManager::listProviders() const {
    std::lock_guard<tl::mutex>      lock(self->m_providers_mtx);
    std::vector<ProviderDescriptor> result;
    result.reserve(self->m_providers.size());
    for (const auto& w : self->m_providers) { result.push_back(w); }
    return result;
}

void ProviderManager::registerProvider(
    const ProviderDescriptor& descriptor, const std::string& pool_name,
    const std::string&                                         config,
    const std::unordered_map<std::string, std::vector<void*>>& dependencies) {
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
        auto                       it = self->resolveSpec(descriptor.name);
        if (it != self->m_providers.end()) {
            throw Exception(
                "Could not register provider: a provider with the name \"{}\""
                " is already registered",
                descriptor.name);
        }

        it = self->resolveSpec(descriptor.type, descriptor.provider_id);
        if (it != self->m_providers.end()) {
            throw Exception(
                "Could not register provider: a provider with the type \"{}\""
                " and provider id {} is already registered",
                descriptor.type, descriptor.provider_id);
        }

        auto margoCtx = MargoContext(self->m_margo_context);

        ProviderEntry entry;
        entry.name        = descriptor.name;
        entry.type        = descriptor.type;
        entry.provider_id = descriptor.provider_id;
        entry.factory     = service_factory;
        entry.margo_ctx   = self->m_margo_context;
        entry.pool        = margoCtx.getPool(pool_name);

        FactoryArgs args;
        args.name         = descriptor.name;
        args.mid          = margoCtx.getMargoInstance();
        args.pool         = entry.pool;
        args.config       = config;
        args.provider_id  = descriptor.provider_id;
        args.dependencies = dependencies;

        if (args.pool == ABT_POOL_NULL || args.pool == nullptr) {
            throw Exception("Could not find pool \"{}\"", pool_name);
        }

        spdlog::trace("Registering provider {} of type {} with provider id {}",
                      descriptor.name, descriptor.type, descriptor.provider_id);
        entry.handle = service_factory->registerProvider(args);

        self->m_providers.push_back(entry);
    }
    self->m_providers_cv.notify_all();
}

void ProviderManager::deregisterProvider(const std::string& spec) {
    std::lock_guard<tl::mutex> lock(self->m_providers_mtx);
    auto                       it = self->resolveSpec(spec);
    if (it == self->m_providers.end()) {
        throw Exception("Could not find provider for spec \"{}\"", spec);
    }
    auto& provider = *it;
    spdlog::trace("Deregistering provider {}", provider.name);
    provider.factory->deregisterProvider(provider.handle);
}

void ProviderManager::addProviderFromJSON(
    const std::string& jsonString, const DependencyFinder& dependencyFinder) {
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

    auto service_factory = ModuleContext::getServiceFactory(descriptor.type);
    if (!service_factory) {
        throw Exception(
            "Could not find service factory for provider type \"{}\"",
            descriptor.type);
    }

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

    auto deps_from_config = config.value("dependencies", json::object());

    std::unordered_map<std::string, std::vector<void*>>   dependencies;
    std::unordered_map<std::string, std::vector<VoidPtr>> dependency_wrappers;

    for (const auto& dependency : service_factory->getDependencies()) {
        spdlog::trace("Resolving dependency {}", dependency.name);
        if (deps_from_config.contains(dependency.name)) {
            auto dep_config = deps_from_config[dependency.name];
            if (!(dependency.flags & BEDROCK_ARRAY)) {
                if (!dep_config.is_string()) {
                    throw Exception("Dependency {} should be a string",
                                    dependency.name);
                }
                auto ptr = dependencyFinder.find(dependency.type,
                                                 dep_config.get<std::string>());
                dependencies[dependency.name].push_back(ptr.handle);
                dependency_wrappers[dependency.name].push_back(std::move(ptr));
            } else {
                if (!dep_config.is_array()) {
                    throw Exception("Dependency {} should be an array",
                                    dependency.name);
                }
                std::vector<std::string> deps;
                for (const auto& elem : dep_config) {
                    if (!elem.is_string()) {
                        throw Exception(
                            "Item in dependency array {} should be a string",
                            dependency.name);
                    }
                    auto ptr = dependencyFinder.find(dependency.type,
                                                     elem.get<std::string>());
                    dependencies[dependency.name].push_back(ptr.handle);
                    dependency_wrappers[dependency.name].push_back(
                        std::move(ptr));
                }
            }
        } else if (dependency.flags & BEDROCK_REQUIRED) {
            throw Exception("Missing dependency {} in configuration",
                            dependency.name);
        }
    }

    registerProvider(descriptor, pool_name, provider_config, dependencies);
}

void ProviderManager::addProviderListFromJSON(
    const std::string& jsonString, const DependencyFinder& dependencyFinder) {
    auto config = json::parse(jsonString);
    if (config.is_null()) { return; }
    if (!config.is_array()) {
        throw Exception(
            "Invalid JSON configuration passed to "
            "ProviderManager::addProviderListFromJSON (expected array)");
    }
    for (const auto& provider : config) {
        addProviderFromJSON(provider.dump(), dependencyFinder);
    }
}

std::string ProviderManager::getCurrentConfig() const {
    return self->makeConfig().dump();
}

} // namespace bedrock
