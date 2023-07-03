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

DependencyFinder::DependencyFinder(const MargoManager&    margo,
                                   const ABTioManager&    abtio,
                                   const SSGManager&      ssg,
                                   const MonaManager&     mona,
                                   const ProviderManager& pmanager,
                                   const ClientManager&   cmanager)
: self(std::make_shared<DependencyFinderImpl>(margo.getMargoInstance())) {
    self->m_margo_context    = margo;
    self->m_abtio_context    = abtio.self;
    self->m_ssg_context      = ssg.self;
    self->m_mona_context     = mona.self;
    self->m_provider_manager = pmanager.self;
    self->m_client_manager   = cmanager.self;
}

DependencyFinder::DependencyFinder(const DependencyFinder&) = default;

DependencyFinder::DependencyFinder(DependencyFinder&&) = default;

DependencyFinder& DependencyFinder::operator=(const DependencyFinder&)
    = default;

DependencyFinder& DependencyFinder::operator=(DependencyFinder&&) = default;

DependencyFinder::~DependencyFinder() = default;

DependencyFinder::operator bool() const { return static_cast<bool>(self); }

static bool isPositiveNumber(const std::string& str) {
    if (str.empty()) return false;
    for (unsigned i = 0; i < str.size(); i++)
        if (!isdigit(str[i])) return false;
    return true;
}

std::shared_ptr<NamedDependency> DependencyFinder::find(
        const std::string& type, const std::string& spec,
        std::string* resolved) const {
    spdlog::trace("DependencyFinder search for {} of type {}", spec, type);

    if (type == "pool") { // Argobots pool

        auto pool = MargoManager(self->m_margo_context).getPool(spec);
        if (!pool) {
            throw Exception("Could not find pool with name \"{}\"", spec);
        }
        if (resolved) { *resolved = spec; }
        return pool;

    } else if (type == "abt_io") { // ABTIO instance

        auto abtio_manager_impl = self->m_abtio_context.lock();
        if (!abtio_manager_impl) {
            throw Exception("Could not resolve ABT-IO dependency: no ABTioManager found");
        }
        auto abt_io = ABTioManager(abtio_manager_impl).getABTioInstance(spec);
        if (resolved) { *resolved = spec; }
        return abt_io;

    } else if (type == "mona") { // MoNA instance

        auto mona_manager_impl = self->m_mona_context.lock();
        if (!mona_manager_impl) {
            throw Exception("Could not resolve MoNA dependency: no MonaManager found");
        }
        auto mona_id = MonaManager(mona_manager_impl).getMonaInstance(spec);
        if (!mona_id) {
            throw Exception("Could not find MoNA instance with name \"{}\"",
                            spec);
        }
        if (resolved) { *resolved = spec; }
        return mona_id;

    } else if (type == "ssg") { // SSG group
                                //
        auto ssg_manager_impl = self->m_ssg_context.lock();
        if(!ssg_manager_impl) {
            throw Exception("Could not resolve SSG dependency: no SSGManager found");
        }
        auto group = SSGManager(ssg_manager_impl).getGroup(spec);
        if (!group) {
            throw Exception("Could not find SSG group with name \"{}\"", spec);
        }
        if (resolved) { *resolved = spec; }
        return group;

    } else { // Provider or provider handle

        std::regex re(
            "(?:([a-zA-Z_][a-zA-Z0-9_]*)\\->)?" // client name (name->)
            "([a-zA-Z_][a-zA-Z0-9_]*)"          // identifier (name or type)
            "(?::(client|[0-9]+))?" // specifier ("client" or provider id)
            "(?:@(.+))?");          // locator (@address)
        std::smatch match;
        if (std::regex_search(spec, match, re)) {
            if (match.str(0) != spec) {
                throw Exception("Ill-formated dependency specification \"{}\"",
                                spec);
            }
            auto client_name = match.str(1);
            auto identifier  = match.str(2); // name or type
            auto specifier
                = match.str(3);          // provider id, or "client" or "admin"
            auto locator = match.str(4); // address or "local"

            if (locator.empty() && !client_name.empty()) {
                throw Exception(
                    "Client name (\"{}\") specified in dependency that is not "
                    "a provider handle",
                    client_name);
            }

            if (locator.empty()) { // local dependency to a provider or a client
                                   // or admin
                if (specifier == "client") { // requesting a client
                    auto client = findClient(type, "");
                    if (client) { *resolved = client->getName(); }
                    return client;
                } else if (isPositiveNumber(
                               specifier)) { // dependency to a provider
                                             // specified by type:id
                    uint16_t provider_id = atoi(specifier.c_str());
                    if (type != identifier) {
                        throw Exception(
                            "Invalid provider type in \"{}\" (expected {})",
                            spec, type);
                    }
                    if (resolved) { *resolved = spec; }
                    return findProvider(type, provider_id);
                } else { // dependency to a provider specified by name, or a
                         // client
                    try {
                        try {
                            auto ptr = findClient(type, identifier);
                            if (resolved) { *resolved = identifier; }
                            return ptr;
                        } catch (const Exception&) {
                            // didn't fine a client, try a provider
                            uint16_t provider_id;
                            auto     ptr
                                = findProvider(type, identifier, &provider_id);
                            if (resolved) {
                                *resolved
                                    = type + ":" + std::to_string(provider_id);
                            }
                            return ptr;
                        }
                    } catch (const Exception&) {
                        throw Exception(
                            "Could not find client or provider with "
                            "specification \"{}\"",
                            spec);
                    }
                }
            } else { // dependency to a provider handle
                if (specifier.empty()) {
                    // dependency specified as name@location
                    return makeProviderHandle(client_name, type, identifier,
                                              locator, resolved);
                } else if (isPositiveNumber(specifier)) {
                    // dependency specified as type:id@location
                    uint16_t provider_id = atoi(specifier.c_str());
                    if (type != identifier) {
                        throw Exception(
                            "Invalid provider type in \"{}\" (expected {})",
                            spec, type);
                    }
                    return makeProviderHandle(client_name, type, provider_id,
                                              locator, resolved);
                } else { // invalid
                    throw Exception(
                        "Ill-formated dependency specification \"{}\"", spec);
                }
            }

        } else {
            throw Exception("Ill-formated dependency specification \"{}\"",
                            spec);
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
            type + ":" + std::to_string(provider_id), nullptr);
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
    auto provider = ProviderManager(provider_manager_impl)
        .lookupProvider(name, provider_id);
    if (!provider) {
        throw Exception("Could not find provider named \"{}\"", name);
    }
    if (provider->getType() != type) {
        throw Exception("Invalid type {} for dependency \"{}\" (expected {})",
                        provider->getType(), name, type);
    }
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
        auto client = ClientManager(client_manager_impl).lookupClient(name);
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
            .lookupOrCreateAnonymous(type);
        return client;
    }
}

std::shared_ptr<NamedDependency>
DependencyFinder::makeProviderHandle(const std::string& client_name,
                                     const std::string& type,
                                     uint16_t           provider_id,
                                     const std::string& locator,
                                     std::string* resolved) const {
    spdlog::trace("Making provider handle of type {} with id {} and locator {}",
                  type, provider_id, locator);
    auto        mid    = MargoManager(self->m_margo_context).getMargoInstance();
    auto        client = findClient(type, client_name);
    auto        service_factory = ModuleContext::getServiceFactory(type);
    hg_addr_t   addr            = HG_ADDR_NULL;

    if (locator == "local") {

        auto provider_manager_impl = self->m_provider_manager.lock();
        if (!provider_manager_impl) {
            throw Exception(
                "Could not resolve provider handle: no ProviderManager found");
        }
        auto provider = ProviderManager(provider_manager_impl)
            .lookupProvider(type + ":" + std::to_string(provider_id), nullptr);
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

        if (locator.rfind("ssg://", 0) == 0) {
            auto ssg_manager_impl = self->m_ssg_context.lock();
            if (!ssg_manager_impl) {
                throw Exception(
                    "Could not resolve SSG address: no SSGManager found");
            }
            hg_addr_t ssg_addr
                = SSGManager(ssg_manager_impl).resolveAddress(locator);
            hg_return_t hret = margo_addr_dup(mid, ssg_addr, &addr);
            if (hret != HG_SUCCESS) {
                throw Exception(
                    "Failed to duplicate address returned by "
                    "SSGManager (margo_addr_dup returned {})",
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
                                     const std::string& locator,
                                     std::string* resolved) const {
    auto        mid    = self->m_margo_context->m_mid;
    auto        client = findClient(type, client_name);
    auto        service_factory = ModuleContext::getServiceFactory(type);
    hg_addr_t   addr            = HG_ADDR_NULL;
    uint16_t    provider_id     = 0;
    ProviderDescriptor descriptor;
    spdlog::trace("Making provider handle to provider {} of type {} at {}",
                  name, type, locator);

    if (locator == "local") {

        auto provider_manager_impl = self->m_provider_manager.lock();
        if (!provider_manager_impl) {
            throw Exception("Could not make provider handle: no ProviderManager found");
        }
        auto provider = ProviderManager(provider_manager_impl)
            .lookupProvider(name, &provider_id);
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
            provider_id
        };

    } else {

        if (locator.rfind("ssg://", 0) == 0) {
            auto ssg_manager_impl = self->m_ssg_context.lock();
            if (!ssg_manager_impl) {
                throw Exception("Could not resolve SSG address: no SSGManager found");
            }
            hg_addr_t ssg_addr
                = SSGManager(ssg_manager_impl).resolveAddress(locator);
            hg_return_t hret = margo_addr_dup(mid, ssg_addr, &addr);
            if (hret != HG_SUCCESS) {
                throw Exception(
                    "Failed to duplicate address returned by "
                    "SSGManager (margo_addr_dup returned {})",
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
    }

    std::string ph_name;
    char        addr_str[256];
    hg_size_t   addr_str_size = 256;
    margo_addr_to_string(mid, addr_str, &addr_str_size, addr);
    ph_name = client->getName() + "->" + type + ":"
            + std::to_string(descriptor.provider_id) + "@" + addr_str;

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
