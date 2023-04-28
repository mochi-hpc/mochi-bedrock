/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/ProviderManager.hpp"
#include "bedrock/ModuleContext.hpp"
#include "bedrock/AbstractServiceFactory.hpp"
#include "bedrock/DependencyFinder.hpp"
#include "Exception.hpp"

#include "ProviderManagerImpl.hpp"

#include <thallium/serialization/stl/vector.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <cctype>

namespace tl = thallium;

namespace bedrock {

using namespace std::string_literals;
using nlohmann::json;

ProviderManager::ProviderManager(const MargoManager& margo,
                                 uint16_t provider_id,
                                 std::shared_ptr<NamedDependency> pool)
: self(std::make_shared<ProviderManagerImpl>(margo.getThalliumEngine(),
                                             provider_id, tl::pool(pool->getHandle<ABT_pool>()))) {
    self->m_margo_context = margo;
}

ProviderManager::ProviderManager(const ProviderManager&) = default;

ProviderManager::ProviderManager(ProviderManager&&) = default;

ProviderManager& ProviderManager::operator=(const ProviderManager&) = default;

ProviderManager& ProviderManager::operator=(ProviderManager&&) = default;

ProviderManager::~ProviderManager() = default;

ProviderManager::operator bool() const { return static_cast<bool>(self); }

void ProviderManager::setDependencyFinder(const DependencyFinder& finder) {
    self->m_dependency_finder = finder;
}

std::shared_ptr<NamedDependency>
ProviderManager::lookupProvider(const std::string& spec, uint16_t* provider_id) const {
    std::lock_guard<tl::mutex> lock(self->m_providers_mtx);
    auto                       it = self->resolveSpec(spec);
    if (it == self->m_providers.end())
        throw DETAILED_EXCEPTION("Could not find provider with spec \"{}\"", spec);
    if(provider_id) *provider_id = (*it)->provider_id;
    return *it;
}

std::vector<ProviderDescriptor> ProviderManager::listProviders() const {
    std::lock_guard<tl::mutex>      lock(self->m_providers_mtx);
    std::vector<ProviderDescriptor> result;
    result.reserve(self->m_providers.size());
    for (const auto& p : self->m_providers) {
        auto descriptor = ProviderDescriptor{p->getName(), p->getType(), p->provider_id};
        result.emplace_back(descriptor);
    }
    return result;
}

std::shared_ptr<NamedDependency>
ProviderManager::registerProvider(
    const ProviderDescriptor& descriptor, const std::string& pool_name,
    const std::string& config, const ResolvedDependencyMap& dependencies) {
    if (descriptor.name.empty()) {
        throw DETAILED_EXCEPTION("Provider name cannot be empty");
    }

    auto service_factory = ModuleContext::getServiceFactory(descriptor.type);
    if (!service_factory) {
        throw DETAILED_EXCEPTION(
            "Could not find service factory for provider type \"{}\"",
            descriptor.type);
    }
    spdlog::trace("Found provider \"{}\" to be of type \"{}\"", descriptor.name,
                  descriptor.type);

    std::shared_ptr<ProviderEntry> entry;
    {
        std::lock_guard<tl::mutex> lock(self->m_providers_mtx);
        auto                       it = self->resolveSpec(descriptor.name);
        if (it != self->m_providers.end()) {
            throw DETAILED_EXCEPTION(
                "Could not register provider: a provider with the name \"{}\""
                " is already registered",
                descriptor.name);
        }

        it = self->resolveSpec(descriptor.type, descriptor.provider_id);
        if (it != self->m_providers.end()) {
            throw DETAILED_EXCEPTION(
                "Could not register provider: a provider with the type \"{}\""
                " and provider id {} is already registered",
                descriptor.type, descriptor.provider_id);
        }

        auto margo = MargoManager(self->m_margo_context);
        auto pool = margo.getPool(pool_name);

        FactoryArgs args;
        args.name         = descriptor.name;
        args.mid          = margo.getMargoInstance();
        args.engine       = margo.getThalliumEngine();
        args.pool         = pool->getHandle<ABT_pool>();
        args.config       = config;
        args.provider_id  = descriptor.provider_id;
        args.dependencies = dependencies;

        auto handle = service_factory->registerProvider(args);

        entry = std::make_shared<ProviderEntry>(
            descriptor.name, descriptor.type, descriptor.provider_id,
            handle, service_factory, pool, std::move(dependencies));

        spdlog::trace("Registered provider {} of type {} with provider id {}",
                      descriptor.name, descriptor.type, descriptor.provider_id);

        self->m_providers.push_back(entry);
    }
    self->m_providers_cv.notify_all();
    return entry;
}

void ProviderManager::deregisterProvider(const std::string& spec) {
    std::lock_guard<tl::mutex> lock(self->m_providers_mtx);
    auto                       it = self->resolveSpec(spec);
    if (it == self->m_providers.end()) {
        throw DETAILED_EXCEPTION("Could not find provider for spec \"{}\"", spec);
    }
    spdlog::trace("Deregistering provider {}", spec);
    self->m_providers.erase(it);
}

std::shared_ptr<NamedDependency>
ProviderManager::addProviderFromJSON(const std::string& jsonString) {
    if (!self->m_dependency_finder) {
        throw DETAILED_EXCEPTION("No DependencyFinder set in ProviderManager");
    }
    auto dependencyFinder = DependencyFinder(self->m_dependency_finder);
    auto config           = json::parse(jsonString);
    if (!config.is_object()) {
        throw DETAILED_EXCEPTION(
            "Invalid JSON configuration passed to "
            "ProviderManager::addProviderFromJSON (should be an object)");
    }

    ProviderDescriptor descriptor;

    if(!config.contains("name")) {
        throw DETAILED_EXCEPTION("\"name\" field not found in provider definition");
    }
    if(!config["name"].is_string()) {
        throw DETAILED_EXCEPTION(
            "\"name\" field in provider definition should be a string");
    }

    if(config.contains("provider_id")) {
        if(!config["provider_id"].is_number())
            throw DETAILED_EXCEPTION(
                "\"provider_id\" field in provider definition should be an integer");
        if(!config["provider_id"].is_number_unsigned())
            throw DETAILED_EXCEPTION(
                "\"provider_id\" field in provider definition should be a positive integer");
    }

    descriptor.name        = config["name"];
    descriptor.provider_id = config.value("provider_id", (uint16_t)0);

    if (!config.contains("type")) {
        throw DETAILED_EXCEPTION("\"type\" field missing in provider definition");
    }
    if(!config["type"].is_string()) {
        throw DETAILED_EXCEPTION(
            "\"type\" field in provider definition should be a string");
    }
    descriptor.type = config["type"];

    auto service_factory = ModuleContext::getServiceFactory(descriptor.type);
    if (!service_factory) {
        throw DETAILED_EXCEPTION(
            "Could not find service factory for provider type \"{}\"",
            descriptor.type);
    }

    auto provider_config = "{}"s;
    if (config.contains("config")) {
        if (!config["config"].is_object()) {
            throw DETAILED_EXCEPTION(
                "\"config\" field in provider definition should be an object");
        } else {
            provider_config = config["config"].dump();
        }
    }

    auto margo   = MargoManager(self->m_margo_context);

    std::string pool_name;
    if(config.contains("pool")) {
        auto& pool_ref = config["pool"];
        if(pool_ref.is_string())
            pool_name = pool_ref.get<std::string>();
        else if(pool_ref.is_number_unsigned())
            pool_name = margo.getPool(pool_ref.get<uint32_t>())->getName();
        else
            throw DETAILED_EXCEPTION(
                "\"pool\" field in provider definition should "
                "be a string or an unsigned integer");
    } else {
        pool_name = margo.getDefaultHandlerPool()->getName();
    }

    auto deps_from_config = config.value("dependencies", json::object());

    ResolvedDependencyMap resolved_dependency_map;

    for (const auto& dependency : service_factory->getProviderDependencies()) {
        spdlog::trace("Resolving dependency {}", dependency.name);
        if (deps_from_config.contains(dependency.name)) {
            auto dep_config = deps_from_config[dependency.name];
            if (!(dependency.flags & BEDROCK_ARRAY)) {
                if (!dep_config.is_string()) {
                    throw DETAILED_EXCEPTION("Dependency {} should be a string",
                                    dependency.name);
                }
                auto dep_handle = dependencyFinder.find(
                        dependency.type, dep_config.get<std::string>(), nullptr);
                resolved_dependency_map[dependency.name].is_array = false;
                resolved_dependency_map[dependency.name].dependencies.push_back(dep_handle);

            } else { // dependency is an array

                if (!dep_config.is_array()) {
                    throw DETAILED_EXCEPTION("Dependency {} should be an array",
                                    dependency.name);
                }
                std::vector<std::string> deps;
                for (const auto& elem : dep_config) {
                    if (!elem.is_string()) {
                        throw DETAILED_EXCEPTION(
                            "Item in dependency array {} should be a string",
                            dependency.name);
                    }
                    auto dep_handle = dependencyFinder.find(
                            dependency.type, elem.get<std::string>(), nullptr);
                    resolved_dependency_map[dependency.name].is_array = true;
                    resolved_dependency_map[dependency.name].dependencies.push_back(dep_handle);
                }
            }
        } else if (dependency.flags & BEDROCK_REQUIRED) {
            throw DETAILED_EXCEPTION("Missing dependency {} in configuration",
                            dependency.name);
        }
    }

    return registerProvider(descriptor, pool_name, provider_config,
                            resolved_dependency_map);
}

void ProviderManager::addProviderListFromJSON(const std::string& jsonString) {
    auto config = json::parse(jsonString);
    if (config.is_null()) { return; }
    if (!config.is_array()) {
        throw DETAILED_EXCEPTION(
            "Invalid JSON configuration passed to "
            "ProviderManager::addProviderListFromJSON (should be an array)");
    }
    for (const auto& provider : config) {
        addProviderFromJSON(provider.dump());
    }
}

void ProviderManager::changeProviderPool(const std::string& provider,
                                         const std::string& pool_name) {
    // find the provider
    std::lock_guard<tl::mutex> lock(self->m_providers_mtx);
    auto                       it = self->resolveSpec(provider);
    if (it == self->m_providers.end())
        throw DETAILED_EXCEPTION("Provider with spec \"{}\" not found", provider);
    // find the pool
    auto margo_manager = MargoManager(self->m_margo_context);
    auto pool = margo_manager.getPool(pool_name);
    if(!pool) {
        throw DETAILED_EXCEPTION("Could not find pool named \"{}\"", pool_name);
    }
    // call the provider's change_pool callback
    (*it)->factory->changeProviderPool((*it)->getHandle<void*>(), pool->getHandle<ABT_pool>());
    (*it)->pool = pool;
}

std::string ProviderManager::getCurrentConfig() const {
    return self->makeConfig().dump();
}

} // namespace bedrock
