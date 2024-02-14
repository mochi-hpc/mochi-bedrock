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
#include "JsonUtil.hpp"

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
                                 const Jx9Manager& jx9,
                                 uint16_t provider_id,
                                 std::shared_ptr<NamedDependency> pool)
: self(std::make_shared<ProviderManagerImpl>(margo.getThalliumEngine(),
                                             provider_id, tl::pool(pool->getHandle<ABT_pool>()))) {
    self->m_margo_manager = margo;
    self->m_jx9_manager   = jx9;
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

size_t ProviderManager::numProviders() const {
    std::lock_guard<tl::mutex> lock(self->m_providers_mtx);
    return self->m_providers.size();
}

std::shared_ptr<ProviderDependency> ProviderManager::getProvider(const std::string& name) const {
    std::lock_guard<tl::mutex> lock(self->m_providers_mtx);
    auto                       it = std::find_if(
        self->m_providers.begin(), self->m_providers.end(),
        [&name](const auto& p) { return p->getName() == name; });
    if (it == self->m_providers.end())
        throw DETAILED_EXCEPTION("Could not find provider \"{}\"", name);
    return *it;
}

std::shared_ptr<ProviderDependency> ProviderManager::getProvider(size_t index) const {
    std::lock_guard<tl::mutex> lock(self->m_providers_mtx);
    if (index >= self->m_providers.size())
        throw DETAILED_EXCEPTION("Could not find provider at index {}", index);
    return self->m_providers[index];
}

std::shared_ptr<ProviderDependency>
ProviderManager::registerProvider(
    const std::string& name, const std::string& type, uint16_t provider_id,
    std::shared_ptr<NamedDependency> pool, const json& config,
    const ResolvedDependencyMap& dependencies,
    const std::vector<std::string>& tags) {

    std::shared_ptr<LocalProvider> entry;
    auto service_factory = ModuleContext::getServiceFactory(type);
    if (!service_factory) {
        throw DETAILED_EXCEPTION(
            "Could not find service factory for provider type \"{}\"", type);
    }

    {
        std::lock_guard<tl::mutex> lock(self->m_providers_mtx);
        auto                       it = self->resolveSpec(name);
        if (it != self->m_providers.end()) {
            throw DETAILED_EXCEPTION(
                "Name \"{}\" already used by another provider", name);
        }

        if (provider_id == std::numeric_limits<uint16_t>::max())
            provider_id = self->getAvailableProviderID();

        it = std::find_if(self->m_providers.begin(), self->m_providers.end(),
                [provider_id](const auto& p) { return p->getProviderID() == provider_id; });
        if (it != self->m_providers.end()) {
            throw DETAILED_EXCEPTION(
                "Another provider already uses provider ID {}", provider_id);
        }

        auto margo = MargoManager(self->m_margo_manager);

        FactoryArgs args;
        args.name         = name;
        args.mid          = margo.getMargoInstance();
        args.engine       = margo.getThalliumEngine();
        args.pool         = pool->getHandle<ABT_pool>();
        args.config       = config.is_null() ? std::string{"{}"} : config.dump();
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
ProviderManager::addProviderFromJSON(const json& description) {
    if (!self->m_dependency_finder) {
        throw DETAILED_EXCEPTION("No DependencyFinder set in ProviderManager");
    }
    auto dependencyFinder = DependencyFinder(self->m_dependency_finder);

    static constexpr const char* configSchema = R"(
    {
        "$schema": "https://json-schema.org/draft/2019-09/schema",
        "type": "object",
        "properties": {
            "name": {"type": "string", "pattern": "^[a-zA-Z_][a-zA-Z0-9_]*$" },
            "pool": {"oneOf": [
                {"type": "string", "pattern": "^[a-zA-Z_][a-zA-Z0-9_]*$" },
                {"type": "integer", "minimum": 0 }
            ]},
            "provider_id": {"type": "integer", "minimum": 0, "maximum": 65535},
            "type": {"type": "string"},
            "tags": {
                "type": "array",
                "items": {"type": "string"}
            },
            "dependencies": {
                "type": "object",
                "additionalProperties": {
                    "anyOf": [
                        {"type": "string"},
                        {"type": "array", "items": {"type": "string"}}
                    ]
                }
            },
            "config": {"type": "object"},
            "__if__": {"type": "string", "minLength":1}
        },
        "required": ["name", "type"]
    }
    )";
    static const JsonValidator validator{configSchema};
    validator.validate(description, "ProviderManager");

    if(description.contains("__if__")) {
        bool b = Jx9Manager(self->m_jx9_manager).evaluateCondition(
                description["__if__"].get_ref<const std::string&>(), {});
        if(!b) return nullptr;
    }

    auto& name = description["name"].get_ref<const std::string&>();
    auto& type = description["type"].get_ref<const std::string&>();
    auto provider_id = description.value("provider_id", std::numeric_limits<uint16_t>::max());

    auto service_factory = ModuleContext::getServiceFactory(type);
    if (!service_factory) {
        throw DETAILED_EXCEPTION(
            "Could not find service factory for provider type \"{}\"", type);
    }

    auto config = description.value("config", json::object());
    auto configStr = config.dump();

    // find pool
    std::shared_ptr<NamedDependency> pool;
    if(description.contains("pool") && description["pool"].is_number()) {
        pool = MargoManager(self->m_margo_manager)
            .getPool(description["pool"].get<uint32_t>());
    } else {
        pool = MargoManager(self->m_margo_manager)
            .getPool(description.value("pool", "__primary__"));
    }

    std::vector<std::string> tags;
    for(auto& tag : description.value("tags", json::array())) {
        tags.push_back(tag.get<std::string>());
    }

    auto deps_from_config = description.value("dependencies", json::object());
    ResolvedDependencyMap resolved_dependency_map;

    for (const auto& dependency : service_factory->getProviderDependencies(configStr.c_str())) {
        spdlog::trace("Resolving dependency {}", dependency.name);
        if (deps_from_config.contains(dependency.name)) {
            auto dep_config = deps_from_config[dependency.name];
            if (!(dependency.flags & BEDROCK_ARRAY)) { // dependency should be a string
                if (!dep_config.is_string()) {
                    throw DETAILED_EXCEPTION("Dependency {} should be a string",
                            dependency.name);
                }
                auto dep_handle = dependencyFinder.find(
                        dependency.type, BEDROCK_GET_KIND_FROM_FLAG(dependency.flags),
                        dep_config.get<std::string>(), nullptr);
                resolved_dependency_map[dependency.name].is_array = false;
                resolved_dependency_map[dependency.name].dependencies.push_back(dep_handle);

            } else { // dependency is an array
                if (!dep_config.is_array()) {
                    throw DETAILED_EXCEPTION("Dependency {} should be an array",
                            dependency.name);
                }
                std::vector<std::string> deps;
                for (const auto& elem : dep_config) {
                    auto dep_handle = dependencyFinder.find(
                            dependency.type, BEDROCK_GET_KIND_FROM_FLAG(dependency.flags),
                            elem.get<std::string>(), nullptr);
                    resolved_dependency_map[dependency.name].is_array = true;
                    resolved_dependency_map[dependency.name].dependencies.push_back(dep_handle);
                }
            }
        } else if (dependency.flags & BEDROCK_REQUIRED) {
            throw DETAILED_EXCEPTION(
                "Missing dependency \"{}\" of type \"{}\" in provider configuration",
                dependency.name, dependency.type);
        }
    }

    return registerProvider(name, type, provider_id, pool, config,
                            resolved_dependency_map, tags);
}

void ProviderManager::addProviderListFromJSON(const json& list) {
    if (list.is_null()) { return; }
    if (!list.is_array()) {
        throw DETAILED_EXCEPTION(
            "Invalid JSON configuration passed to "
            "ProviderManager::addProviderListFromJSON (should be an array)");
    }
    for (const auto& provider : list) {
        addProviderFromJSON(provider);
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
    auto margo_manager = MargoManager(self->m_margo_manager);
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

json ProviderManager::getCurrentConfig() const {
    return self->makeConfig();
}

} // namespace bedrock
