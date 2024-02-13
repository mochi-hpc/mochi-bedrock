/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/ClientManager.hpp"
#include "bedrock/ModuleContext.hpp"
#include "bedrock/AbstractServiceFactory.hpp"
#include "bedrock/DependencyFinder.hpp"

#include "Exception.hpp"
#include "ClientManagerImpl.hpp"
#include "JsonUtil.hpp"

#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/vector.hpp>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <cctype>

namespace tl = thallium;

namespace bedrock {

using namespace std::string_literals;
using nlohmann::json;

ClientManager::ClientManager(const MargoManager& margo,
                             const Jx9Manager& jx9,
                             uint16_t provider_id,
                             const std::shared_ptr<NamedDependency>& pool)
: self(std::make_shared<ClientManagerImpl>(margo.getThalliumEngine(),
                                           provider_id, tl::pool(pool->getHandle<ABT_pool>()))) {
    self->m_margo_manager = margo;
    self->m_jx9_manager   = jx9;
}

// LCOV_EXCL_START

ClientManager::ClientManager(const ClientManager&) = default;

ClientManager::ClientManager(ClientManager&&) = default;

ClientManager& ClientManager::operator=(const ClientManager&) = default;

ClientManager& ClientManager::operator=(ClientManager&&) = default;

ClientManager::~ClientManager() = default;

ClientManager::operator bool() const { return static_cast<bool>(self); }

// LCOV_EXCL_STOP

void ClientManager::setDependencyFinder(const DependencyFinder& finder) {
    self->m_dependency_finder = finder;
}

size_t ClientManager::numClients() const {
    std::lock_guard<tl::mutex> lock(self->m_clients_mtx);
    return self->m_clients.size();
}

std::shared_ptr<NamedDependency> ClientManager::getClient(const std::string& name) const {
    std::lock_guard<tl::mutex> lock(self->m_clients_mtx);
    auto                       it = self->findByName(name);
    if (it == self->m_clients.end())
        throw DETAILED_EXCEPTION("Could not find client \"{}\"", name);
    return *it;
}

std::shared_ptr<NamedDependency> ClientManager::getClient(size_t index) const {
    std::lock_guard<tl::mutex> lock(self->m_clients_mtx);
    if (index >= self->m_clients.size())
        throw DETAILED_EXCEPTION("Could not find client at index {}", index);
    return self->m_clients[index];
}

std::shared_ptr<NamedDependency> ClientManager::getOrCreateAnonymous(const std::string& type) {
    {
        std::lock_guard<tl::mutex> lock(self->m_clients_mtx);
        for (const auto& client : self->m_clients) {
            if (client->getType() == type) {
                return client;
            }
        }
        // no client of this type found, try creating one with name
        // __type_client__ first we need to check whether we have the module for
        // such a client
        auto service_factory = ModuleContext::getServiceFactory(type);
        if (!service_factory) {
            throw DETAILED_EXCEPTION(
                "Could not find service factory for client type \"{}\"", type);
        }
        // find out if such a client has required dependencies
        for (const auto& dependency :
             service_factory->getClientDependencies("{}")) {
            if (dependency.flags & BEDROCK_REQUIRED)
                throw DETAILED_EXCEPTION(
                    "Could not create default client of type \"{}\" because"
                    " it requires dependency \"{}\"",
                    type, dependency.name);
        }
    }

    // we can create the client
    std::string name = "__"s + type + "_client__";
    ResolvedDependencyMap dependencies;

    addClient(name, type, json::object(), dependencies);
    // get the client
    auto it  = self->findByName(name);
    return *it;
}

std::vector<ClientDescriptor> ClientManager::listClients() const {
    std::lock_guard<tl::mutex>    lock(self->m_clients_mtx);
    std::vector<ClientDescriptor> result;
    result.reserve(self->m_clients.size());
    for (const auto& w : self->m_clients) {
        auto descriptor = ClientDescriptor{w->getName(), w->getType()};
        result.push_back(descriptor);
    }
    return result;
}

std::shared_ptr<NamedDependency>
ClientManager::addClient(const std::string&              name,
                         const std::string&              type,
                         const json&                     config,
                         const ResolvedDependencyMap&    dependencies,
                         const std::vector<std::string>& tags) {
    std::shared_ptr<ClientEntry> entry;
    auto service_factory = ModuleContext::getServiceFactory(type);
    if (!service_factory) {
        throw DETAILED_EXCEPTION(
            "Could not find service factory for client type \"{}\"", type);
    }

    {
        std::lock_guard<tl::mutex> lock(self->m_clients_mtx);
        auto                       it = self->findByName(name);
        if (it != self->m_clients.end()) {
            throw DETAILED_EXCEPTION(
                "Name \"{}\" is already used by another client", name);
        }

        auto margoCtx = MargoManager(self->m_margo_manager);

        FactoryArgs args;
        args.name         = name;
        args.mid          = margoCtx.getMargoInstance();
        args.engine       = margoCtx.getThalliumEngine();
        args.pool         = ABT_POOL_NULL;
        args.config       = config.dump();
        args.provider_id  = std::numeric_limits<uint16_t>::max();
        args.tags         = tags;
        args.dependencies = dependencies;

        auto handle = service_factory->initClient(args);

        entry = std::make_shared<ClientEntry>(
            name, type,
            handle, service_factory, dependencies, tags);

        spdlog::trace("Registered client {} of type {}", name, type);

        self->m_clients.push_back(entry);
    }
    self->m_clients_cv.notify_all();
    return entry;
}

void ClientManager::removeClient(const std::string& name) {
    std::lock_guard<tl::mutex> lock(self->m_clients_mtx);
    auto                       it = self->findByName(name);
    if (it == self->m_clients.end()) {
        throw DETAILED_EXCEPTION("Could not find client with name \"{}\"", name);
    }
    if(it->use_count() > 1) {
        throw DETAILED_EXCEPTION(
            "Cannot destroy client \"{}\" as it is used as dependency",
            (*it)->getName());
    }
    self->m_clients.erase(it);
}

void ClientManager::removeClient(size_t index) {
    std::lock_guard<tl::mutex> lock(self->m_clients_mtx);
    if (index >= self->m_clients.size())
        throw DETAILED_EXCEPTION("Could not find client at index {}", index);
    if(self->m_clients[index].use_count() > 1) {
        throw DETAILED_EXCEPTION(
            "Cannot destroy client at index {} as it is used as dependency",
            index);
    }
    self->m_clients.erase(self->m_clients.begin() + index);
}

std::shared_ptr<NamedDependency>
ClientManager::addClientFromJSON(const json& description) {
    auto dependencyFinder = DependencyFinder(self->m_dependency_finder);
    static constexpr const char* configSchema = R"(
    {
        "$schema": "https://json-schema.org/draft/2019-09/schema",
        "type": "object",
        "properties": {
            "name": {"type": "string", "pattern": "^[a-zA-Z_][a-zA-Z0-9_]*$" },
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
    validator.validate(description, "ClientManager");

    if(description.contains("__if__")) {
        bool b = Jx9Manager(self->m_jx9_manager).evaluateCondition(
                description["__if__"].get_ref<const std::string&>(), {});
        if(!b) return nullptr;
    }

    auto& name = description["name"].get_ref<const std::string&>();
    auto& type = description["type"].get_ref<const std::string&>();

    auto service_factory = ModuleContext::getServiceFactory(type);
    if (!service_factory) {
        throw DETAILED_EXCEPTION(
            "Could not find service factory for client type \"{}\"", type);
    }

    auto config = description.value("config", json::object());
    auto configStr = config.dump();
    std::vector<std::string> tags;
    for(auto& tag : config.value("tags", json::array())) {
        tags.push_back(tag.get<std::string>());
    }

    auto deps_from_config = description.value("dependencies", json::object());
    ResolvedDependencyMap resolved_dependency_map;

    for (const auto& dependency : service_factory->getClientDependencies(configStr.c_str())) {
        spdlog::trace("Resolving dependency {}", dependency.name);
        if (deps_from_config.contains(dependency.name)) {
            auto dep_config = deps_from_config[dependency.name];
            if (!(dependency.flags & BEDROCK_ARRAY)) {
                if (!dep_config.is_string()) {
                    throw DETAILED_EXCEPTION("Dependency {} should be a string",
                                    dependency.name);
                }
                auto ptr = dependencyFinder.find(
                        dependency.type, BEDROCK_GET_KIND_FROM_FLAG(dependency.flags),
                        dep_config.get<std::string>(), nullptr);
                resolved_dependency_map[dependency.name].dependencies.push_back(ptr);
                resolved_dependency_map[dependency.name].is_array = false;
            } else {
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
                    auto ptr = dependencyFinder.find(
                            dependency.type, BEDROCK_GET_KIND_FROM_FLAG(dependency.flags),
                            elem.get<std::string>(), nullptr);
                    resolved_dependency_map[dependency.name].dependencies.push_back(ptr);
                    resolved_dependency_map[dependency.name].is_array = true;
                }
            }
        } else if (dependency.flags & BEDROCK_REQUIRED) {
            throw DETAILED_EXCEPTION(
                "Missing client dependency \"{}\" of type \"{}\" in configuration",
                dependency.name, dependency.type);
        }
    }

    return addClient(name, type, config, resolved_dependency_map, tags);
}

void ClientManager::addClientListFromJSON(const json& list) {
    if (list.is_null()) { return; }
    if (!list.is_array()) {
        throw DETAILED_EXCEPTION(
            "Invalid JSON configuration passed to "
            "ClientManager::addClientListFromJSON (expected array)");
    }
    for (const auto& description : list) {
        addClientFromJSON(description);
    }
}

json ClientManager::getCurrentConfig() const {
    return self->makeConfig();
}

} // namespace bedrock
