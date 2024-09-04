/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "DependencyFinderImpl.hpp"
#include "bedrock/DependencyFinder.hpp"
#include "bedrock/ModuleContext.hpp"
#include "bedrock/AbstractServiceFactory.hpp"
#include "bedrock/Exception.hpp"
#include <thallium.hpp>
#include <cctype>
#include <regex>

namespace tl = thallium;

namespace bedrock {

DependencyFinder::DependencyFinder(const MPIEnv&          mpi,
                                   const MargoManager&    margo,
                                   const ProviderManager& pmanager,
                                   const ClientManager&   cmanager)
: self(std::make_shared<DependencyFinderImpl>(margo.getMargoInstance())) {
    self->m_mpi              = mpi.self;
    self->m_margo_context    = margo;
    self->m_provider_manager = pmanager.self;
    self->m_client_manager   = cmanager.self;
}

// LCOV_EXCL_START

DependencyFinder::DependencyFinder(const DependencyFinder&) = default;

DependencyFinder::DependencyFinder(DependencyFinder&&) = default;

DependencyFinder& DependencyFinder::operator=(const DependencyFinder&)
    = default;

DependencyFinder& DependencyFinder::operator=(DependencyFinder&&) = default;

DependencyFinder::~DependencyFinder() = default;

DependencyFinder::operator bool() const { return static_cast<bool>(self); }

// LCOV_EXCL_STOP

std::shared_ptr<NamedDependency> DependencyFinder::find(
        const std::string& type, int32_t kind,
        const std::string& spec, std::string* resolved) const {
    spdlog::trace("DependencyFinder search for {} of type {}", spec, type);

    if (type == "pool") { // Argobots pool

        auto pool = MargoManager(self->m_margo_context).getPool(spec);
        if (!pool) {
            throw Exception("Could not find pool with name \"{}\"", spec);
        }
        if (resolved) { *resolved = spec; }
        return pool;

    } else if (type == "xstream") { // Argobots xstream

        auto xstream = MargoManager(self->m_margo_context).getXstream(spec);
        if (!xstream) {
            throw Exception("Could not find xstream with name \"{}\"", spec);
        }
        if (resolved) { *resolved = spec; }
        return xstream;

    } else if (kind == BEDROCK_KIND_CLIENT) {

        auto client = findClient(type, spec);
        if (client && resolved) { *resolved = client->getName(); }
        return client;

    } else if (kind == BEDROCK_KIND_PROVIDER) {

        // the spec can be in the form "name" or "type:id"
        std::regex re(
            "([a-zA-Z_][a-zA-Z0-9_]*)" // identifier (name or type)
            "(?::([0-9]+))?");         // specifier ("client" or provider id)
        std::smatch match;
        if (!std::regex_search(spec, match, re) || match.str(0) != spec) {
            throw Exception("Ill-formated dependency specification \"{}\"", spec);
        }
        auto identifier      = match.str(1); // name or type
        auto provider_id_str = match.str(2); // provider id

        if(provider_id_str.empty()) { // identifier is a name
            uint16_t provider_id;
            auto ptr = findProvider(type, identifier, &provider_id);
            if (resolved) *resolved = type + ":" + std::to_string(provider_id);
            return ptr;
        } else {
            uint16_t provider_id = std::atoi(provider_id_str.c_str());
            auto ptr = findProvider(type, provider_id);
            if (resolved) *resolved = type + ":" + std::to_string(provider_id);
            return ptr;
        }

    } else { // Provider handle

        std::regex re(
            "(?:([a-zA-Z_][a-zA-Z0-9_]*)\\->)?" // client name (name->)
            "([a-zA-Z_][a-zA-Z0-9_]*)"          // identifier (name or type)
            "(?::([0-9]+))?"                    // optional provider id
            "(?:@(.+))?");                      // optional locator (@address)
        std::smatch match;
        if (!std::regex_search(spec, match, re) || match.str(0) != spec) {
            throw Exception("Ill-formated dependency specification \"{}\"", spec);
        }

        auto client_name     = match.str(1); // client to use for provider handles
        auto identifier      = match.str(2); // name or type
        auto provider_id_str = match.str(3); // provider id
        auto locator         = match.str(4); // address or "local" or MPI rank
        if(locator.empty()) locator = "local";

        if (provider_id_str.empty()) {
            // dependency specified as client->name@location
            return makeProviderHandle(
                client_name, type, identifier, locator, resolved);

        } else {
            // dependency specified as client->type:id@location
            uint16_t provider_id = atoi(provider_id_str.c_str());
            if (type != identifier) {
                throw Exception(
                        "Invalid provider type in \"{}\" (expected {})",
                        spec, type);
            }
            return makeProviderHandle(
                    client_name, type, provider_id, locator, resolved);
        }
    }
    return nullptr;
}

std::shared_ptr<NamedDependency>
DependencyFinder::findProvider(const std::string& type,
                               uint16_t           provider_id) const {
    auto provider_manager_impl = self->m_provider_manager.lock();
    if (!provider_manager_impl) {
        throw Exception("Could not resolve provider dependency: no ProviderManager found");
    }
    auto provider = ProviderManager(provider_manager_impl).lookupProvider(
            type + ":" + std::to_string(provider_id));
    if (!provider) {
        throw Exception("Could not find provider of type {} with id {}", type,
                        provider_id);
    }
    return provider;
}

std::shared_ptr<NamedDependency>
DependencyFinder::findProvider(const std::string& type,
                               const std::string& name,
                               uint16_t*          provider_id) const {
    auto provider_manager_impl = self->m_provider_manager.lock();
    if (!provider_manager_impl) {
        throw Exception("Could not resolve provider dependency: no ProviderManager found");
    }
    auto provider = ProviderManager(provider_manager_impl).lookupProvider(name);
    if (!provider) {
        throw Exception("Could not find provider named \"{}\"", name);
    }
    if (provider->getType() != type) {
        throw Exception("Invalid type {} for dependency \"{}\" (expected {})",
                        provider->getType(), name, type);
    }
    if(provider_id) *provider_id = provider->getProviderID();
    return provider;
}

std::shared_ptr<NamedDependency>
DependencyFinder::findClient(
        const std::string& type,
        const std::string& name) const {
    auto client_manager_impl = self->m_client_manager.lock();
    if (!client_manager_impl) {
        throw Exception("Could not resolve client dependency: no ClientManager found");
    }
    if (!name.empty()) {
        auto client = ClientManager(client_manager_impl).getClient(name);
        if (!client) {
            throw Exception("Could not find client named \"{}\"", name);
        } else if (type != client->getType()) {
            throw Exception(
                "Invalid type {} for dependency \"{}\" (expected {})",
                client->getType(), name, type);
        }
        return client;
    } else {
        auto client = ClientManager(client_manager_impl)
            .getOrCreateAnonymous(type);
        return client;
    }
}

std::shared_ptr<NamedDependency>
DependencyFinder::makeProviderHandle(const std::string& client_name,
                                     const std::string& type,
                                     uint16_t           provider_id,
                                     const std::string& locatorArg,
                                     std::string* resolved) const {
    auto locator = locatorArg;
    spdlog::trace("Making provider handle of type {} with id {} and locator {}",
                  type, provider_id, locator);
    auto        mid    = MargoManager(self->m_margo_context).getMargoInstance();
    auto        client = findClient(type, client_name);
    auto        service_factory = ModuleContext::getServiceFactory(type);
    hg_addr_t   addr            = HG_ADDR_NULL;
    bool locator_is_number = true;
    int rank = 0;
    for(auto c : locator) {
        if(c >= '0' && c <= '9') {
            rank = rank*10 + (c - '0');
            continue;
        }
        locator_is_number = false;
        break;
    }
    if(locator_is_number) locator = MPIEnv(self->m_mpi).addressOfRank(rank);

    if (locator == "local") {

        auto provider_manager_impl = self->m_provider_manager.lock();
        if (!provider_manager_impl) {
            throw Exception(
                "Could not resolve provider handle: no ProviderManager found");
        }
        auto provider = ProviderManager(provider_manager_impl)
            .lookupProvider(type + ":" + std::to_string(provider_id));
        if(!provider) {
            throw Exception(
                "Could not find local provider of type {} with id {}", type,
                provider_id);
        }
        if (provider->getType() != type) {
            throw Exception(
                "Invalid type {} for provider handle to provider of type {}",
                type, provider->getType());
        }
        hg_return_t hret = margo_addr_self(mid, &addr);
        if (hret != HG_SUCCESS) {
            throw Exception(
                "Failed to get self address (margo_addr_self returned {})",
                std::to_string(hret));
        }

    } else {

        hg_return_t hret = margo_addr_lookup(mid, locator.c_str(), &addr);
        if (hret != HG_SUCCESS) {
            throw Exception(
                    "Failed to lookup address {} "
                    "(margo_addr_lookup returned {})",
                    locator, std::to_string(hret));
        }

        ProviderDescriptor descriptor;
        try {
            auto spec = type + ":" + std::to_string(provider_id);
            auto provider_manager_impl = self->m_provider_manager.lock();
            if (!provider_manager_impl) {
                throw Exception("Could not lookup provider: no ProviderManager found");
            }
            auto pid  = provider_manager_impl->get_provider_id();
            self->lookupRemoteProvider(addr, pid, spec, &descriptor);
        } catch (...) {
            margo_addr_free(mid, addr);
            throw;
        }
    }

    std::string name;
    char        addr_str[256];
    hg_size_t   addr_str_size = 256;
    margo_addr_to_string(mid, addr_str, &addr_str_size, addr);
    name = client->getName() + "->" + type + ":"
         + std::to_string(provider_id) + "@" + addr_str;

    if (resolved) *resolved = name;

    void* ph = service_factory->createProviderHandle(
        client->getHandle<void*>(), addr, provider_id);

    margo_addr_free(mid, addr);
    return std::make_shared<NamedDependency>(
        std::to_string(provider_id) + "@" + addr_str,
        client->getType(), ph,
        [service_factory](void* ph) {
        service_factory->destroyProviderHandle(ph);
    });
}

std::shared_ptr<NamedDependency>
DependencyFinder::makeProviderHandle(const std::string& client_name,
                                     const std::string& type,
                                     const std::string& name,
                                     const std::string& locatorArg,
                                     std::string* resolved) const {
    auto locator = locatorArg;
    auto        mid    = self->m_margo_context->m_mid;
    auto        client = findClient(type, client_name);
    auto        service_factory = ModuleContext::getServiceFactory(type);
    hg_addr_t   addr            = HG_ADDR_NULL;
    ProviderDescriptor descriptor;
    uint16_t provider_id;
    spdlog::trace("Making provider handle to provider {} of type {} at {}",
                  name, type, locator);

    bool locator_is_number = true;
    int rank = 0;
    for(auto c : locator) {
        if(c >= '0' && c <= '9') {
            rank = rank*10 + (c - '0');
            continue;
        }
        locator_is_number = false;
        break;
    }
    if(locator_is_number) locator = MPIEnv(self->m_mpi).addressOfRank(rank);

    if (locator == "local") {

        auto provider_manager_impl = self->m_provider_manager.lock();
        if (!provider_manager_impl) {
            throw Exception("Could not make provider handle: no ProviderManager found");
        }
        auto provider = ProviderManager(provider_manager_impl).lookupProvider(name);
        if (!provider) {
            throw Exception("Could not find local provider with name {}", name);
        }
        if (provider->getType() != type) {
            throw Exception(
                "Invalid type {} for provider handle to provider of type {}",
                type, provider->getType());
        }
        hg_return_t hret = margo_addr_self(mid, &addr);
        if (hret != HG_SUCCESS) {
            throw Exception(
                "Failed to get self address (margo_addr_self returned {})",
                std::to_string(hret));
        }
        descriptor = ProviderDescriptor{
            provider->getName(),
            provider->getType(),
            provider->getProviderID(),
        };
        provider_id = provider->getProviderID();

    } else {

        hg_return_t hret = margo_addr_lookup(mid, locator.c_str(), &addr);
        if (hret != HG_SUCCESS) {
            throw Exception(
                    "Failed to lookup address {} "
                    "(margo_addr_lookup returned {})",
                    locator, std::to_string(hret));
        }

        try {
            auto provider_manager_impl = self->m_provider_manager.lock();
            if (!provider_manager_impl) {
                throw Exception("Could not get provider id: no ProviderManager found");
            }
            auto pid = provider_manager_impl->get_provider_id();
            self->lookupRemoteProvider(addr, pid, name, &descriptor);
        } catch (...) {
            margo_addr_free(mid, addr);
            throw;
        }
        provider_id = descriptor.provider_id;
    }

    std::string ph_name;
    char        addr_str[256];
    hg_size_t   addr_str_size = 256;
    margo_addr_to_string(mid, addr_str, &addr_str_size, addr);
    ph_name = client->getName() + "->" + type + ":"
            + std::to_string(provider_id) + "@" + addr_str;

    if (resolved) *resolved = ph_name;

    void* ph = service_factory->createProviderHandle(
        client->getHandle<void*>(), addr, descriptor.provider_id);

    margo_addr_free(mid, addr);
    return std::make_shared<NamedDependency>(
        std::to_string(provider_id) + "@" + addr_str,
        client->getType(), ph,
        [service_factory](void* ph) {
        service_factory->destroyProviderHandle(ph);
    });
}

} // namespace bedrock
