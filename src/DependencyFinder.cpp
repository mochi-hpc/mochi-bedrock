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
#include <cctype>
#include <regex>

namespace bedrock {

DependencyFinder::DependencyFinder(const MargoContext&    margo,
                                   const ABTioContext&    abtio,
                                   const SSGContext&      ssg,
                                   const ProviderManager& pmanager)
: self(std::make_shared<DependencyFinderImpl>()) {
    self->m_margo_context    = margo;
    self->m_abtio_context    = abtio;
    self->m_ssg_context      = ssg;
    self->m_provider_manager = pmanager;
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

VoidPtr DependencyFinder::find(const std::string& type,
                               const std::string& spec) const {
    if (type == "pool") { // Argobots pool
        ABT_pool p = MargoContext(self->m_margo_context).getPool(spec);
        if (p == ABT_POOL_NULL) {
            throw Exception("Could not find pool with name \"{}\"", spec);
        }
        return VoidPtr(p);
    } else if (type == "abt_io") { // ABTIO instance
        abt_io_instance_id abt_id
            = ABTioContext(self->m_abtio_context).getABTioInstance(spec);
        if (abt_id == ABT_IO_INSTANCE_NULL) {
            throw Exception("Could not find ABT-IO instance with name \"{}\"",
                            spec);
        }
        return VoidPtr(abt_id);
    } else if (type == "ssg") { // SSG group
        ssg_group_id_t gid = SSGContext(self->m_ssg_context).getGroup(spec);
        if (gid == SSG_GROUP_ID_INVALID) {
            throw Exception("Could not find SSG group with name \"{}\"", spec);
        }
        return VoidPtr(reinterpret_cast<void*>(gid));
    } else { // Provider or provider handle

        std::regex re(
            "([a-zA-Z_][a-zA-Z0-9_]*)(?::([0-9]+|client))?(?:@(.+))?");
        std::smatch match;
        if (std::regex_search(spec, match, re)) {
            if (match.str(0) != spec) {
                throw Exception("Ill-formated dependency specification \"{}\"",
                                spec);
            }
            auto identifier = match.str(1); // name or type
            auto specifier
                = match.str(2);          // provider id, or "client" or "admin"
            auto locator = match.str(3); // address or "local"

            if (locator.empty()) { // local dependency to a provider or a client
                                   // or admin
                if (specifier == "client") { // dependency to a client
                    if (!locator.empty()) {
                        throw Exception(
                            "Ill-formated dependency specification \"{}\"",
                            spec);
                    }
                    if (type != identifier) {
                        throw Exception("Invalid client type in \"{}\"", spec);
                    }
                    return getClient(type);
                } else if (isPositiveNumber(
                               specifier)) { // dependency to a provider
                                             // specified by type:id
                    uint16_t provider_id = atoi(specifier.c_str());
                    if (type != identifier) {
                        throw Exception(
                            "Invalid provider type in \"{}\" (expected {})",
                            spec, type);
                    }
                    return findProvider(type, provider_id);
                } else { // dependency to a provider specified by name
                    return findProvider(type, identifier);
                }
            } else { // dependency to a provider handle
                if (specifier
                        .empty()) { // dependency specified as name@location
                    return makeProviderHandle(type, identifier, locator);
                } else if (isPositiveNumber(
                               specifier)) { // dependency specified as
                                             // type:id@location
                    uint16_t provider_id = atoi(specifier.c_str());
                    if (type != identifier) {
                        throw Exception(
                            "Invalid provider type in \"{}\" (expected {})",
                            spec, type);
                    }
                    return makeProviderHandle(type, provider_id, locator);
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
    return VoidPtr();
}

VoidPtr DependencyFinder::findProvider(const std::string& type,
                                       uint16_t           provider_id) const {
    ProviderWrapper provider;
    bool            exists = ProviderManager(self->m_provider_manager)
                      .lookupProvider(type + ":" + std::to_string(provider_id),
                                      &provider);
    if (!exists) {
        throw Exception("Could not find provider of type {} with id {}", type,
                        provider_id);
    }
    return VoidPtr(provider.handle);
}

VoidPtr DependencyFinder::findProvider(const std::string& type,
                                       const std::string& name) const {
    ProviderWrapper provider;
    bool            exists = ProviderManager(self->m_provider_manager)
                      .lookupProvider(name, &provider);
    if (!exists) {
        throw Exception("Could not find provider named \"{}\"", name);
    }
    if (provider.type != type) {
        throw Exception("Invalid type {} for dependency \"{}\" (expected {})",
                        provider.type, name, type);
    }
    return VoidPtr(provider.handle);
}

VoidPtr DependencyFinder::getClient(const std::string& type) const {
    auto it = self->m_cached_clients.find(type);
    if (it != self->m_cached_clients.end()) {
        return VoidPtr(it->second.handle);
    }
    auto service_factory = ModuleContext::getServiceFactory(type);
    if (!service_factory) {
        throw Exception(
            "Could not create client for unknown service type \"{}\"", type);
    }
    void* client = service_factory->initClient(self->m_margo_context->m_mid);
    self->m_cached_clients.emplace(type,
                                   VoidPtr(client, [service_factory](void* c) {
                                       service_factory->finalizeClient(c);
                                   }));
    return VoidPtr(client);
}

VoidPtr DependencyFinder::makeProviderHandle(const std::string& type,
                                             uint16_t           provider_id,
                                             const std::string& locator) const {
    if (locator == "local") {
        ProviderWrapper wrapper;
        if (!ProviderManager(self->m_provider_manager)
                 .lookupProvider(type + ":" + std::to_string(provider_id),
                                 &wrapper)) {
            throw Exception(
                "Could not find local provider of type {} with id {}", type,
                provider_id);
        }
        if (wrapper.type != type) {
            throw Exception(
                "Invalid type {} for provider handle to provider of type {}",
                type, wrapper.type);
        }
        auto        client          = getClient(type);
        auto        service_factory = ModuleContext::getServiceFactory(type);
        hg_addr_t   self_addr       = HG_ADDR_NULL;
        hg_return_t hret
            = margo_addr_self(self->m_margo_context->m_mid, &self_addr);
        if (hret != HG_SUCCESS) {
            throw Exception(
                "Failed to get self address, margo_addr_self returned {}",
                hret);
        }
        void* ph = service_factory->createProviderHandle(
            client.handle, self_addr, provider_id);
        return VoidPtr(ph, [service_factory](void* ph) {
            service_factory->destroyProviderHandle(ph);
        });
    } else {
        // TODO
        throw Exception("Looking up remote provider is not yet supported");
    }
}

VoidPtr DependencyFinder::makeProviderHandle(const std::string& type,
                                             const std::string& name,
                                             const std::string& locator) const {
    if (locator == "local") {
        ProviderWrapper wrapper;
        if (!ProviderManager(self->m_provider_manager)
                 .lookupProvider(name, &wrapper)) {
            throw Exception("Could not find local provider named \"{}\"", name);
        }
        if (wrapper.type != type) {
            throw Exception(
                "Invalid type {} for provider handle to provider of type {}",
                type, wrapper.type);
        }
        auto        client          = getClient(type);
        auto        service_factory = ModuleContext::getServiceFactory(type);
        hg_addr_t   self_addr       = HG_ADDR_NULL;
        hg_return_t hret
            = margo_addr_self(self->m_margo_context->m_mid, &self_addr);
        if (hret != HG_SUCCESS) {
            throw Exception(
                "Failed to get self address, margo_addr_self returned {}",
                hret);
        }
        void* ph = service_factory->createProviderHandle(
            client.handle, self_addr, wrapper.provider_id);
        return VoidPtr(ph, [service_factory](void* ph) {
            service_factory->destroyProviderHandle(ph);
        });
    } else {
        // TODO
        throw Exception("Looking up remote provider is not yet supported");
    }
}

} // namespace bedrock
