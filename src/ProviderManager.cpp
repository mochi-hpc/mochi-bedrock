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

// LCOV_EXCL_START

ProviderManager::ProviderManager(const ProviderManager&) = default;

ProviderManager::ProviderManager(ProviderManager&&) = default;

ProviderManager& ProviderManager::operator=(const ProviderManager&) = default;

ProviderManager& ProviderManager::operator=(ProviderManager&&) = default;

ProviderManager::~ProviderManager() = default;

ProviderManager::operator bool() const { return static_cast<bool>(self); }

// LCOV_EXCL_STOP

void ProviderManager::setDependencyFinder(const DependencyFinder& finder) {
    self->m_dependency_finder = finder;
}

std::shared_ptr<ProviderDependency>
ProviderManager::lookupProvider(const std::string& spec) const {
    std::lock_guard<tl::mutex> lock(self->m_providers_mtx);
    auto                       it = self->resolveSpec(spec);
    if (it == self->m_providers.end())
        throw DETAILED_EXCEPTION("Could not find provider with spec \"{}\"", spec);
    return *it;
}

std::vector<ProviderDescriptor> ProviderManager::listProviders() const {
    std::lock_guard<tl::mutex>      lock(self->m_providers_mtx);
    std::vector<ProviderDescriptor> result;
    result.reserve(self->m_providers.size());
    for (const auto& p : self->m_providers) {
        auto descriptor = ProviderDescriptor{p->getName(), p->getType(), p->getProviderID()};
        result.emplace_back(descriptor);
    }
    return result;
}

std::shared_ptr<ProviderDependency>
ProviderManager::registerProvider(
    const ProviderDescriptor& descriptor, const std::string& pool_name,
    const std::string& config, const ResolvedDependencyMap& dependencies,
    const std::vector<std::string>& tags) {

    auto& name       = descriptor.name;
    auto& type       = descriptor.type;
    auto provider_id = descriptor.provider_id;

    if (name.empty()) throw DETAILED_EXCEPTION("Provider name cannot be empty");

    auto service_factory = ModuleContext::getServiceFactory(type);
    if (!service_factory) {
        throw DETAILED_EXCEPTION(
            "Could not find service factory for provider type \"{}\"", type);
    }
    spdlog::trace("Found provider \"{}\" to be of type \"{}\"", name, type);

    std::shared_ptr<LocalProvider> entry;
    {
        std::lock_guard<tl::mutex> lock(self->m_providers_mtx);
        auto                       it = self->resolveSpec(name);
        if (it != self->m_providers.end()) {
            throw DETAILED_EXCEPTION(
                "Could not register provider: a provider with the name \"{}\""
                " is already registered",
                descriptor.name);
        }

        it = self->resolveSpec(type, provider_id);
        if (it != self->m_providers.end()) {
            throw DETAILED_EXCEPTION(
                "Could not register provider: a provider with the type \"{}\""
                " and provider id {} is already registered", type, provider_id);
        }

        if (provider_id == std::numeric_limits<uint16_t>::max())
            provider_id = self->getAvailableProviderID();

        auto margo = MargoManager(self->m_margo_context);
        auto pool = margo.getPool(pool_name);

        FactoryArgs args;
        args.name         = name;
        args.mid          = margo.getMargoInstance();
        args.engine       = margo.getThalliumEngine();
        args.pool         = pool->getHandle<ABT_pool>();
        args.config       = config;
        args.tags         = tags;
        args.dependencies = dependencies;
        args.provider_id  = provider_id;

        auto handle = service_factory->registerProvider(args);

        entry = std::make_shared<LocalProvider>(
            name, type, provider_id, handle, service_factory, pool,
            std::move(dependencies), tags);

        spdlog::trace("Registered provider {} of type {} with provider id {}",
                      name, type, provider_id);

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

std::shared_ptr<ProviderDependency>
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

    if(!config.contains("type"))
        throw DETAILED_EXCEPTION("\"type\" field missing in provider definition");
    if(!config["type"].is_string())
        throw DETAILED_EXCEPTION(
            "\"type\" field in provider definition should be a string");

    descriptor.name        = config["name"];
    descriptor.provider_id = config.value("provider_id", std::numeric_limits<uint16_t>::max());
    descriptor.type        = config["type"];

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

    auto margo = MargoManager(self->m_margo_context);

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

    auto tags_from_config = config.value("tags", json::array());
    std::vector<std::string> tags;
    if(!tags_from_config.is_array()) {
        throw DETAILED_EXCEPTION(
                "\"tags\" field in client definition should be an array");
    }
    for(auto& tag : tags_from_config) {
        if(!tag.is_string()) {
            throw DETAILED_EXCEPTION(
                    "Tag in client definition should be a string");
        }
        tags.push_back(tag.get<std::string>());
    }

    auto deps_from_config = config.value("dependencies", json::object());

    ResolvedDependencyMap resolved_dependency_map;

    for (const auto& dependency : service_factory->getProviderDependencies(provider_config.c_str())) {
        spdlog::trace("Resolving dependency {}", dependency.name);
        if (deps_from_config.contains(dependency.name)) {
            auto dep_config = deps_from_config[dependency.name];
            if (!(dependency.flags & BEDROCK_ARRAY)) { // dependency should be a string

                if (dep_config.is_array() && dep_config.size() == 1)
                    dep_config = dep_config[0];
                if (!dep_config.is_string()) {
                    throw DETAILED_EXCEPTION(
                        "Dependency \"{}\" should be a string", dependency.name);
                }
                auto dep_handle = dependencyFinder.find(
                        dependency.type, BEDROCK_GET_KIND_FROM_FLAG(dependency.flags),
                        dep_config.get<std::string>(), nullptr);
                resolved_dependency_map[dependency.name].is_array = false;
                resolved_dependency_map[dependency.name].dependencies.push_back(dep_handle);

            } else { // dependency is an array
                if (dep_config.is_string()) {
                    dep_config = json::array({dep_config});
                }
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
                            dependency.type, BEDROCK_GET_KIND_FROM_FLAG(dependency.flags),
                            elem.get<std::string>(), nullptr);
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
                            resolved_dependency_map, tags);
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

void ProviderManager::migrateProvider(
        const std::string& provider,
        const std::string& dest_addr,
        uint16_t           dest_provider_id,
        const std::string& migration_config,
        bool               remove_source) {
    // find the provider
    std::lock_guard<tl::mutex> lock(self->m_providers_mtx);
    auto                       it = self->resolveSpec(provider);
    if (it == self->m_providers.end())
        throw DETAILED_EXCEPTION("Provider with spec \"{}\" not found", provider);
    // call the provider's migrateProvider callback
    try {
        (*it)->factory->migrateProvider(
                (*it)->getHandle<void*>(),
                dest_addr.c_str(), dest_provider_id,
                migration_config.c_str(),
                remove_source);
    } catch(const std::exception& ex) {
        throw Exception{ex.what()};
    }
}

void ProviderManager::snapshotProvider(
        const std::string& provider,
        const std::string& dest_path,
        const std::string& snapshot_config,
        bool               remove_source) {
    // find the provider
    std::lock_guard<tl::mutex> lock(self->m_providers_mtx);
    auto                       it = self->resolveSpec(provider);
    if (it == self->m_providers.end())
        throw DETAILED_EXCEPTION("Provider with spec \"{}\" not found", provider);
    // call the provider's snapshotProvider callback
    try {
        (*it)->factory->snapshotProvider(
                (*it)->getHandle<void*>(),
                dest_path.c_str(),
                snapshot_config.c_str(),
                remove_source);
    } catch(const std::exception& ex) {
        throw Exception{ex.what()};
    }
}

void ProviderManager::restoreProvider(
        const std::string& provider,
        const std::string& src_path,
        const std::string& restore_config) {
    // find the provider
    std::lock_guard<tl::mutex> lock(self->m_providers_mtx);
    auto                       it = self->resolveSpec(provider);
    if (it == self->m_providers.end())
        throw DETAILED_EXCEPTION("Provider with spec \"{}\" not found", provider);
    // call the provider's restoreProvider callback
    try {
        (*it)->factory->restoreProvider(
                (*it)->getHandle<void*>(),
                src_path.c_str(),
                restore_config.c_str());
    } catch(const std::exception& ex) {
        throw Exception{ex.what()};
    }
}

std::string ProviderManager::getCurrentConfig() const {
    return self->makeConfig().dump();
}

} // namespace bedrock
