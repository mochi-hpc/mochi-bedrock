/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <bedrock/ProviderManager.hpp>
#include <bedrock/ModuleManager.hpp>
#include <bedrock/AbstractComponent.hpp>
#include <bedrock/DependencyFinder.hpp>
#include <bedrock/DetailedException.hpp>

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
                                             provider_id, tl::pool(pool->getHandle<tl::pool>()))) {
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
        throw BEDROCK_DETAILED_EXCEPTION("Could not find provider with spec \"{}\"", spec);
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
        throw BEDROCK_DETAILED_EXCEPTION("Could not find provider \"{}\"", name);
    return *it;
}

std::shared_ptr<ProviderDependency> ProviderManager::getProvider(size_t index) const {
    std::lock_guard<tl::mutex> lock(self->m_providers_mtx);
    if (index >= self->m_providers.size())
        throw BEDROCK_DETAILED_EXCEPTION("Could not find provider at index {}", index);
    return self->m_providers[index];
}

void ProviderManager::deregisterProvider(const std::string& spec) {
    std::lock_guard<tl::mutex> lock(self->m_providers_mtx);
    auto                       it = self->resolveSpec(spec);
    if (it == self->m_providers.end()) {
        throw BEDROCK_DETAILED_EXCEPTION("Could not find provider for spec \"{}\"", spec);
    }
    spdlog::trace("Deregistering provider {}", spec);
    self->m_providers.erase(it);
}

std::shared_ptr<ProviderDependency>
ProviderManager::addProviderFromJSON(const json& description) {
    if (!self->m_dependency_finder) {
        throw BEDROCK_DETAILED_EXCEPTION("No DependencyFinder set in ProviderManager");
    }
    auto dependencyFinder = DependencyFinder(self->m_dependency_finder);
    static const json configSchema = R"(
    {
        "$schema": "https://json-schema.org/draft/2019-09/schema",
        "type": "object",
        "properties": {
            "name": {"type": "string", "pattern": "^[a-zA-Z_][a-zA-Z0-9_]*$" },
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
            "config": {"type": "object"}
        },
        "required": ["name", "type"]
    }
    )"_json;
    static const JsonValidator validator{configSchema};
    validator.validate(description, "ProviderManager");

    auto& type = description["type"].get_ref<const std::string&>();
    ComponentArgs args;
    args.name = description["name"];
    args.provider_id = description.value("provider_id", std::numeric_limits<uint16_t>::max());
    args.config = description.value("config", json::object()).dump();
    args.engine = MargoManager{self->m_margo_manager}.getThalliumEngine();
    for(auto& tag : description.value("tags", json::array()))
        args.tags.push_back(tag.get<std::string>());

    auto deps_from_config = description.value("dependencies", json::object());
    auto requested_dependencies = ModuleManager::getDependencies(type, args);
    auto& resolved_dependency_map = args.dependencies;

    for (const auto& dependency : requested_dependencies) {
        spdlog::trace("Resolving dependency {}", dependency.name);
        if (deps_from_config.contains(dependency.name)) {
            auto dep_config = deps_from_config[dependency.name];
            if (!(dependency.is_array)) { // dependency should be a string
                if (dep_config.is_array() && dep_config.size() == 1) {
                    // an array of length 1 can be converted into a single string
                    dep_config = dep_config[0];
                }
                if (!dep_config.is_string()) {
                    throw BEDROCK_DETAILED_EXCEPTION(
                            "Dependency \"{}\" should be a string", dependency.name);
                }
                auto dep_handle = dependencyFinder.find(
                        dependency.type, dep_config.get<std::string>(), nullptr);
                resolved_dependency_map[dependency.name].push_back(dep_handle);

            } else { // dependency is an array
                if (dep_config.is_string()) {
                    // a single string can be converted into an array of size 1
                    auto tmp_array = json::array();
                    tmp_array.push_back(dep_config);
                    dep_config = tmp_array;
                }
                if (!dep_config.is_array()) {
                    throw BEDROCK_DETAILED_EXCEPTION(
                            "Dependency \"{}\" should be an array",
                            dependency.name);
                }
                std::vector<std::string> deps;
                for (const auto& elem : dep_config) {
                    auto dep_handle = dependencyFinder.find(
                            dependency.type, elem.get<std::string>(), nullptr);
                    resolved_dependency_map[dependency.name].push_back(dep_handle);
                }
            }
        } else if (dependency.is_required) {
            throw BEDROCK_DETAILED_EXCEPTION(
                    "Missing dependency \"{}\" of type \"{}\" in provider configuration",
                    dependency.name, dependency.type);
        }
    }

    std::shared_ptr<LocalProvider> entry;
    {
        std::unique_lock<tl::mutex> lock(self->m_providers_mtx);
        auto                        it = self->resolveSpec(args.name);
        if (it != self->m_providers.end()) {
            throw BEDROCK_DETAILED_EXCEPTION(
                    "Name \"{}\" already used by another provider", args.name);
        }

        if (args.provider_id == std::numeric_limits<uint16_t>::max())
            args.provider_id = self->getAvailableProviderID();

        it = std::find_if(self->m_providers.begin(), self->m_providers.end(),
                [&](const auto& p) { return p->getProviderID() == args.provider_id; });
        if (it != self->m_providers.end()) {
            throw BEDROCK_DETAILED_EXCEPTION(
                    "Another provider already uses provider ID {}", args.provider_id);
        }

        auto handle = ModuleManager::createComponent(type, args);

        entry = std::make_shared<LocalProvider>(
                args.name, type, args.provider_id, handle,
                requested_dependencies, args.dependencies, args.tags);

        spdlog::trace("Registered provider {} of type {} with provider id {}",
                args.name, type, args.provider_id);

        self->m_providers.push_back(entry);
    }
    self->m_providers_cv.notify_all();
    return entry;
}

void ProviderManager::addProviderListFromJSON(const json& list) {
    if (list.is_null()) { return; }
    if (!list.is_array()) {
        throw BEDROCK_DETAILED_EXCEPTION(
            "Invalid JSON configuration passed to "
            "ProviderManager::addProviderListFromJSON (should be an array)");
    }
    for (const auto& provider : list) {
        addProviderFromJSON(provider);
    }
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
        throw BEDROCK_DETAILED_EXCEPTION("Provider with spec \"{}\" not found", provider);
    ComponentPtr theProvider = (*it)->getHandle<ComponentPtr>();
    try {
        theProvider->migrate(
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
        throw BEDROCK_DETAILED_EXCEPTION("Provider with spec \"{}\" not found", provider);
    ComponentPtr theProvider = (*it)->getHandle<ComponentPtr>();
    try {
        theProvider->snapshot(
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
        throw BEDROCK_DETAILED_EXCEPTION("Provider with spec \"{}\" not found", provider);
    ComponentPtr theProvider = (*it)->getHandle<ComponentPtr>();
    try {
        theProvider->restore(
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
