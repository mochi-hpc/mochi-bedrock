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

#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/vector.hpp>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <cctype>

namespace tl = thallium;

namespace bedrock {

using namespace std::string_literals;
using nlohmann::json;

ClientManager::ClientManager(const MargoManager& margo, uint16_t provider_id,
                             const std::shared_ptr<NamedDependency>& pool)
: self(std::make_shared<ClientManagerImpl>(margo.getThalliumEngine(),
                                           provider_id, tl::pool(pool->getHandle<ABT_pool>()))) {
    self->m_margo_context = margo;
}

ClientManager::ClientManager(const ClientManager&) = default;

ClientManager::ClientManager(ClientManager&&) = default;

ClientManager& ClientManager::operator=(const ClientManager&) = default;

ClientManager& ClientManager::operator=(ClientManager&&) = default;

ClientManager::~ClientManager() = default;

ClientManager::operator bool() const { return static_cast<bool>(self); }

void ClientManager::setDependencyFinder(const DependencyFinder& finder) {
    self->m_dependency_finder = finder;
}

std::shared_ptr<NamedDependency> ClientManager::lookupClient(const std::string& name) const {
    std::lock_guard<tl::mutex> lock(self->m_clients_mtx);
    auto                       it = self->resolveSpec(name);
    if (it == self->m_clients.end())
        throw DETAILED_EXCEPTION("Could not find client \"{}\"", name);
    return *it;
}

std::shared_ptr<NamedDependency> ClientManager::lookupOrCreateAnonymous(const std::string& type) {
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
    ClientDescriptor descriptor;
    descriptor.name = "__"s + type + "_client__";
    descriptor.type = type;
    ResolvedDependencyMap dependencies;

    createClient(descriptor, "{}", dependencies);
    // get the client
    auto it  = self->resolveSpec(descriptor.name);
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
ClientManager::createClient(const ClientDescriptor&         descriptor,
                            const std::string&              config,
                            const ResolvedDependencyMap&    dependencies,
                            const std::vector<std::string>& tags) {
    if (descriptor.name.empty()) {
        throw DETAILED_EXCEPTION("Client name cannot be empty");
    }

    std::shared_ptr<ClientEntry> entry;
    auto service_factory = ModuleContext::getServiceFactory(descriptor.type);
    if (!service_factory) {
        throw DETAILED_EXCEPTION("Could not find service factory for client type \"{}\"",
                        descriptor.type);
    }
    spdlog::trace("Found client \"{}\" to be of type \"{}\"", descriptor.name,
                  descriptor.type);

    {
        std::lock_guard<tl::mutex> lock(self->m_clients_mtx);
        auto                       it = self->resolveSpec(descriptor.name);
        if (it != self->m_clients.end()) {
            throw DETAILED_EXCEPTION(
                "Could not register client: a client with the name \"{}\""
                " is already registered",
                descriptor.name);
        }

        auto margoCtx = MargoManager(self->m_margo_context);

        FactoryArgs args;
        args.name         = descriptor.name;
        args.mid          = margoCtx.getMargoInstance();
        args.engine       = margoCtx.getThalliumEngine();
        args.pool         = ABT_POOL_NULL;
        args.config       = config;
        args.provider_id  = std::numeric_limits<uint16_t>::max();
        args.tags         = tags;
        args.dependencies = dependencies;

        auto handle = service_factory->initClient(args);

        entry = std::make_shared<ClientEntry>(
            descriptor.name, descriptor.type,
            handle, service_factory, dependencies, tags);

        spdlog::trace("Registered client {} of type {}", descriptor.name,
                      descriptor.type);

        self->m_clients.push_back(entry);
    }
    self->m_clients_cv.notify_all();
    return entry;
}

void ClientManager::destroyClient(const std::string& name) {
    std::lock_guard<tl::mutex> lock(self->m_clients_mtx);
    auto                       it = self->resolveSpec(name);
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

std::shared_ptr<NamedDependency>
ClientManager::addClientFromJSON(const std::string& jsonString) {
    auto dependencyFinder = DependencyFinder(self->m_dependency_finder);
    auto config = jsonString.empty() ? json::object() : json::parse(jsonString);
    if (!config.is_object()) {
        throw DETAILED_EXCEPTION("Client configuration should be an object");
    }

    ClientDescriptor descriptor;

    if(!config.contains("name")) {
        throw DETAILED_EXCEPTION("\"name\" field not found in client definition");
    }
    if(!config["name"].is_string()) {
        throw DETAILED_EXCEPTION(
                "\"name\" field in client definition should be a string");
    }

    descriptor.name = config["name"];

    if (!config.contains("type")) {
        throw DETAILED_EXCEPTION("\"type\" field missing in client definition");
    }
    if(!config["type"].is_string()) {
        throw DETAILED_EXCEPTION(
                "\"type\" field in client definition should be a string");
    }
    descriptor.type = config["type"];

    auto service_factory = ModuleContext::getServiceFactory(descriptor.type);
    if (!service_factory) {
        throw DETAILED_EXCEPTION("Could not find service factory for client type \"{}\"",
                        descriptor.type);
    }

    auto client_config = "{}"s;
    if (config.contains("config")) {
        if (!config["config"].is_object()) {
            throw DETAILED_EXCEPTION(
                    "\"config\" field in client definition should be an object");
        } else {
            client_config = config["config"].dump();
        }
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
    if(!deps_from_config.is_object()) {
        throw DETAILED_EXCEPTION(
                "\"dependencies\" field in client definition should be an object (found {})",
                deps_from_config.type_name());
    }

    ResolvedDependencyMap resolved_dependency_map;

    for (const auto& dependency : service_factory->getClientDependencies(client_config.c_str())) {
        spdlog::trace("Resolving dependency {}", dependency.name);
        if (deps_from_config.contains(dependency.name)) {
            auto dep_config = deps_from_config[dependency.name];
            if (!(dependency.flags & BEDROCK_ARRAY)) {
                if (!dep_config.is_string()) {
                    throw DETAILED_EXCEPTION("Dependency {} should be a string",
                                    dependency.name);
                }
                auto ptr = dependencyFinder.find(dependency.type,
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
                    auto ptr = dependencyFinder.find(dependency.type,
                            elem.get<std::string>(), nullptr);
                    resolved_dependency_map[dependency.name].dependencies.push_back(ptr);
                    resolved_dependency_map[dependency.name].is_array = true;
                }
            }
        } else if (dependency.flags & BEDROCK_REQUIRED) {
            throw DETAILED_EXCEPTION("Missing dependency {} in configuration",
                            dependency.name);
        }
    }

    return createClient(descriptor, client_config, resolved_dependency_map, tags);
}

void ClientManager::addClientListFromJSON(const std::string& jsonString) {
    auto config = json::parse(jsonString);
    if (config.is_null()) { return; }
    if (!config.is_array()) {
        throw DETAILED_EXCEPTION(
            "Invalid JSON configuration passed to "
            "ClientManager::addClientListFromJSON (expected array)");
    }
    for (const auto& client : config) {
        addClientFromJSON(client.dump());
    }
}

std::string ClientManager::getCurrentConfig() const {
    return self->makeConfig().dump();
}

} // namespace bedrock
