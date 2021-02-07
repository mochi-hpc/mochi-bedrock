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
                                   const ProviderManager& pmanager)
: self(std::make_shared<DependencyFinderImpl>(margo.getMargoInstance())) {
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

VoidPtr DependencyFinder::find(const std::string& type, const std::string& spec,
                               std::string* resolved) const {
    spdlog::trace("DependencyFinder search for {} of type {}", spec, type);
    if (type == "pool") { // Argobots pool
        ABT_pool p = MargoManager(self->m_margo_context).getPool(spec);
        if (p == ABT_POOL_NULL) {
            throw Exception("Could not find pool with name \"{}\"", spec);
        }
        if (resolved) { *resolved = spec; }
        return VoidPtr(p);
    } else if (type == "abt_io") { // ABTIO instance
        abt_io_instance_id abt_id
            = ABTioManager(self->m_abtio_context).getABTioInstance(spec);
        if (abt_id == ABT_IO_INSTANCE_NULL) {
            throw Exception("Could not find ABT-IO instance with name \"{}\"",
                            spec);
        }
        if (resolved) { *resolved = spec; }
        return VoidPtr(abt_id);
    } else if (type == "ssg") { // SSG group
        ssg_group_id_t gid = SSGManager(self->m_ssg_context).getGroup(spec);
        if (gid == SSG_GROUP_ID_INVALID) {
            throw Exception("Could not find SSG group with name \"{}\"", spec);
        }
        if (resolved) { *resolved = spec; }
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
                    if (resolved) { *resolved = spec; }
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
                    if (resolved) { *resolved = spec; }
                    return findProvider(type, provider_id);
                } else { // dependency to a provider specified by name
                    uint16_t provider_id;
                    auto     ptr = findProvider(type, identifier, &provider_id);
                    if (resolved) {
                        *resolved = type + ":" + std::to_string(provider_id);
                    }
                    return ptr;
                }
            } else { // dependency to a provider handle
                if (specifier.empty()) {
                    // dependency specified as name@location
                    return makeProviderHandle(type, identifier, locator,
                                              resolved);
                } else if (isPositiveNumber(specifier)) {
                    // dependency specified as type:id@location
                    uint16_t provider_id = atoi(specifier.c_str());
                    if (type != identifier) {
                        throw Exception(
                            "Invalid provider type in \"{}\" (expected {})",
                            spec, type);
                    }
                    return makeProviderHandle(type, provider_id, locator,
                                              resolved);
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
                                       const std::string& name,
                                       uint16_t*          provider_id) const {
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
    if (provider_id) *provider_id = provider.provider_id;
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
                                             const std::string& locator,
                                             std::string* resolved) const {
    spdlog::trace("Making provider handle of type {} with id {} and locator {}",
                  type, provider_id, locator);
    auto      mid             = self->m_margo_context->m_mid;
    auto      client          = getClient(type);
    auto      service_factory = ModuleContext::getServiceFactory(type);
    hg_addr_t addr            = HG_ADDR_NULL;

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
        hg_return_t hret = margo_addr_self(mid, &addr);
        if (hret != HG_SUCCESS) {
            throw Exception(
                "Failed to get self address, margo_addr_self returned {}",
                hret);
        }

    } else {

        if (locator.rfind("ssg://", 0) == 0) {
            hg_addr_t ssg_addr
                = SSGManager(self->m_ssg_context).resolveAddress(locator);
            hg_return_t hret = margo_addr_dup(mid, ssg_addr, &addr);
            if (hret != HG_SUCCESS) {
                throw Exception(
                    "Failed to duplicate address returned by "
                    "SSGManager (margo_addr_dup returned {})",
                    hret);
            }

        } else {

            hg_return_t hret = margo_addr_lookup(mid, locator.c_str(), &addr);
            if (hret != HG_SUCCESS) {
                throw Exception(
                    "Failed to lookup address {} "
                    "(margo_addr_lookup returned {})",
                    locator, hret);
            }
        }

        ProviderDescriptor descriptor;
        try {
            auto spec = type + ":" + std::to_string(provider_id);
            auto pid  = self->m_provider_manager->get_provider_id();
            self->lookupRemoteProvider(addr, pid, spec, &descriptor);
        } catch (...) {
            margo_addr_free(mid, addr);
            throw;
        }
    }

    if (resolved) {
        char      addr_str[256];
        hg_size_t addr_str_size = 256;
        margo_addr_to_string(mid, addr_str, &addr_str_size, addr);
        *resolved = type + ":" + std::to_string(provider_id) + "@" + addr_str;
    }

    void* ph = service_factory->createProviderHandle(client.handle, addr,
                                                     provider_id);
    margo_addr_free(mid, addr);
    return VoidPtr(ph, [service_factory](void* ph) {
        service_factory->destroyProviderHandle(ph);
    });
}

VoidPtr DependencyFinder::makeProviderHandle(const std::string& type,
                                             const std::string& name,
                                             const std::string& locator,
                                             std::string* resolved) const {
    auto               mid             = self->m_margo_context->m_mid;
    auto               client          = getClient(type);
    auto               service_factory = ModuleContext::getServiceFactory(type);
    hg_addr_t          addr            = HG_ADDR_NULL;
    ProviderDescriptor descriptor;
    spdlog::trace("Making provider handle to provider {} of type {} at {}",
                  name, type, locator);

    if (locator == "local") {

        ProviderWrapper wrapper;
        if (!ProviderManager(self->m_provider_manager)
                 .lookupProvider(name, &wrapper)) {
            throw Exception("Could not find local provider of name {}", name);
        }
        if (wrapper.type != type) {
            throw Exception(
                "Invalid type {} for provider handle to provider of type {}",
                type, wrapper.type);
        }
        hg_return_t hret = margo_addr_self(mid, &addr);
        if (hret != HG_SUCCESS) {
            throw Exception(
                "Failed to get self address, margo_addr_self returned {}",
                hret);
        }
        descriptor = wrapper;

    } else {

        if (locator.rfind("ssg://", 0) == 0) {
            hg_addr_t ssg_addr
                = SSGManager(self->m_ssg_context).resolveAddress(locator);
            hg_return_t hret = margo_addr_dup(mid, ssg_addr, &addr);
            if (hret != HG_SUCCESS) {
                throw Exception(
                    "Failed to duplicate address returned by "
                    "SSGManager (margo_addr_dup returned {})",
                    hret);
            }

        } else {

            hg_return_t hret = margo_addr_lookup(mid, locator.c_str(), &addr);
            if (hret != HG_SUCCESS) {
                throw Exception(
                    "Failed to lookup address {} "
                    "(margo_addr_lookup returned {})",
                    locator, hret);
            }
        }

        try {
            auto pid = self->m_provider_manager->get_provider_id();
            self->lookupRemoteProvider(addr, pid, name, &descriptor);
        } catch (...) {
            margo_addr_free(mid, addr);
            throw;
        }
    }

    if (resolved) {
        char      addr_str[256];
        hg_size_t addr_str_size = 256;
        margo_addr_to_string(mid, addr_str, &addr_str_size, addr);
        *resolved = type + ":" + std::to_string(descriptor.provider_id) + "@"
                  + addr_str;
    }

    void* ph = service_factory->createProviderHandle(client.handle, addr,
                                                     descriptor.provider_id);
    margo_addr_free(mid, addr);
    return VoidPtr(ph, [service_factory](void* ph) {
        service_factory->destroyProviderHandle(ph);
    });
}

} // namespace bedrock
