/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/ClientManager.hpp"
#include "bedrock/Exception.hpp"
#include "bedrock/ModuleContext.hpp"
#include "bedrock/AbstractServiceFactory.hpp"
#include "bedrock/DependencyFinder.hpp"

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

std::shared_ptr<NamedDependency> ClientManager::lookupClient(const std::string& name) const {
    std::lock_guard<tl::mutex> lock(self->m_clients_mtx);
    auto                       it = self->resolveSpec(name);
    if (it == self->m_clients.end())
        return nullptr;
    else
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
            throw Exception(
                "Could not find service factory for client type \"{}\"", type);
        }
        // find out if such a client has required dependencies
        for (const auto& dependency :
             service_factory->getClientDependencies()) {
            if (dependency.flags & BEDROCK_REQUIRED)
                throw Exception(
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
ClientManager::createClient(const ClientDescriptor&      descriptor,
                            const std::string&           config,
                            const ResolvedDependencyMap& dependencies) {
    if (descriptor.name.empty()) {
        throw Exception("Client name cannot be empty");
    }

    std::shared_ptr<ClientEntry> entry;
    auto service_factory = ModuleContext::getServiceFactory(descriptor.type);
    if (!service_factory) {
        throw Exception("Could not find service factory for client type \"{}\"",
                        descriptor.type);
    }
    spdlog::trace("Found client \"{}\" to be of type \"{}\"", descriptor.name,
                  descriptor.type);

    {
        std::lock_guard<tl::mutex> lock(self->m_clients_mtx);
        auto                       it = self->resolveSpec(descriptor.name);
        if (it != self->m_clients.end()) {
            throw Exception(
                "Could not register client: a client with the name \"{}\""
                " is already registered",
                descriptor.name);
        }

        auto margoCtx = MargoManager(self->m_margo_context);

        FactoryArgs args;
        args.name         = descriptor.name;
        args.mid          = margoCtx.getMargoInstance();
        args.pool         = ABT_POOL_NULL;
        args.config       = config;
        args.provider_id  = 0;
        args.dependencies = dependencies;

        auto handle = service_factory->initClient(args);

        entry = std::make_shared<ClientEntry>(
            descriptor.name, descriptor.type,
            handle, service_factory, dependencies);

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
        throw Exception("Could not find client with name \"{}\"", name);
    }
    auto& client = *it;
    spdlog::trace("Destroying client {}", client->getName());
    client->factory->finalizeClient(client->getHandle<void*>());
}

std::shared_ptr<NamedDependency>
ClientManager::addClientFromJSON(
    const std::string& jsonString, const DependencyFinder& dependencyFinder) {
    auto config = json::parse(jsonString);
    if (!config.is_object()) {
        throw Exception(
            "Invalid JSON configuration passed to "
            "ClientManager::addClientFromJSON (should be an object)");
    }

    ClientDescriptor descriptor;

    descriptor.name = config.value("name", "");

    auto type_it = config.find("type");
    if (type_it == config.end()) {
        throw Exception("No type provided for client in JSON configuration");
    }
    descriptor.type = type_it->get<std::string>();

    auto service_factory = ModuleContext::getServiceFactory(descriptor.type);
    if (!service_factory) {
        throw Exception("Could not find service factory for client type \"{}\"",
                        descriptor.type);
    }

    auto client_config    = "{}"s;
    auto client_config_it = config.find("config");
    if (client_config_it != config.end()) {
        client_config = client_config_it->dump();
    }

    auto deps_from_config = config.value("dependencies", json::object());

    ResolvedDependencyMap                         resolved_dependency_map;
    // dependencies is used to keep shared_ptrs alive long enough in case
    // they are temporary
    std::vector<std::shared_ptr<NamedDependency>> dependencies;

    for (const auto& dependency : service_factory->getClientDependencies()) {
        spdlog::trace("Resolving dependency {}", dependency.name);
        if (deps_from_config.contains(dependency.name)) {
            auto dep_config = deps_from_config[dependency.name];
            if (!(dependency.flags & BEDROCK_ARRAY)) {
                if (!dep_config.is_string()) {
                    throw Exception("Dependency {} should be a string",
                                    dependency.name);
                }
                std::string        resolved_spec;
                auto               ptr = dependencyFinder.find(dependency.type,
                                                 dep_config.get<std::string>(),
                                                 &resolved_spec);
                ResolvedDependency resolved_dependency;
                resolved_dependency.name   = dependency.name;
                resolved_dependency.type   = dependency.type;
                resolved_dependency.flags  = dependency.flags;
                resolved_dependency.spec   = resolved_spec;
                resolved_dependency.handle = ptr->getHandle<void*>();
                resolved_dependency_map[dependency.name].push_back(
                    resolved_dependency);
                dependencies.push_back(std::move(ptr));
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
                    std::string resolved_spec;
                    auto        ptr = dependencyFinder.find(dependency.type,
                                                     elem.get<std::string>(),
                                                     &resolved_spec);
                    ResolvedDependency resolved_dependency;
                    resolved_dependency.name   = dependency.name;
                    resolved_dependency.type   = dependency.type;
                    resolved_dependency.flags  = dependency.flags;
                    resolved_dependency.spec   = resolved_spec;
                    resolved_dependency.handle = ptr->getHandle<void*>();
                    resolved_dependency_map[dependency.name].push_back(resolved_dependency);
                    dependencies.push_back(std::move(ptr));
                }
            }
        } else if (dependency.flags & BEDROCK_REQUIRED) {
            throw Exception("Missing dependency {} in configuration",
                            dependency.name);
        }
    }

    return createClient(descriptor, client_config, resolved_dependency_map);
}

void ClientManager::addClientListFromJSON(
    const std::string& jsonString, const DependencyFinder& dependencyFinder) {
    auto config = json::parse(jsonString);
    if (config.is_null()) { return; }
    if (!config.is_array()) {
        throw Exception(
            "Invalid JSON configuration passed to "
            "ClientManager::addClientListFromJSON (expected array)");
    }
    for (const auto& client : config) {
        addClientFromJSON(client.dump(), dependencyFinder);
    }
}

std::string ClientManager::getCurrentConfig() const {
    return self->makeConfig().dump();
}

} // namespace bedrock
