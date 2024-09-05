/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "DependencyFinderImpl.hpp"
#include "bedrock/DependencyFinder.hpp"
#include "bedrock/ModuleManager.hpp"
#include "bedrock/AbstractComponent.hpp"
#include "bedrock/Exception.hpp"
#include "bedrock/ProviderHandle.hpp"
#include <thallium.hpp>
#include <cctype>
#include <regex>

namespace tl = thallium;

namespace bedrock {

DependencyFinder::DependencyFinder(const MPIEnv&          mpi,
                                   const MargoManager&    margo,
                                   const ProviderManager& pmanager)
: self(std::make_shared<DependencyFinderImpl>(margo.getMargoInstance())) {
    self->m_mpi              = mpi.self;
    self->m_margo_context    = margo;
    self->m_provider_manager = pmanager.self;
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
        const std::string& type,
        const std::string& spec,
        std::string* resolved) const {
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

    } else if (spec.find("@") == std::string::npos) { // local provider

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
            "([a-zA-Z_][a-zA-Z0-9_]*)"          // identifier (name or type)
            "(?::([0-9]+))?"                    // optional provider id
            "(?:@(.+))?");                      // optional locator (@address)
        std::smatch match;
        if (!std::regex_search(spec, match, re) || match.str(0) != spec) {
            throw Exception("Ill-formated dependency specification \"{}\"", spec);
        }

        auto identifier      = match.str(1); // name or type
        auto provider_id_str = match.str(2); // provider id
        auto locator         = match.str(3); // address or "local" or MPI rank
        if(locator.empty()) locator = "local";

        if (provider_id_str.empty()) {
            // dependency specified as name@location
            return makeProviderHandle(type, identifier, locator, resolved);

        } else {
            // dependency specified as type:id@location
            uint16_t provider_id = atoi(provider_id_str.c_str());
            if (type != identifier) {
                throw Exception(
                        "Invalid provider type in \"{}\" (expected {})",
                        spec, type);
            }
            return makeProviderHandle(type, provider_id, locator, resolved);
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
DependencyFinder::makeProviderHandle(const std::string& type,
                                     uint16_t           provider_id,
                                     const std::string& locatorArg,
                                     std::string* resolved) const {
    auto locator = locatorArg;
    spdlog::trace("Making provider handle of type {} with id {} and locator {}",
                  type, provider_id, locator);
    auto engine = MargoManager(self->m_margo_context).getThalliumEngine();
    thallium::endpoint endpoint;

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
        try {
            endpoint = engine.self();
        } catch(const std::exception& ex) {
            throw Exception(
                    "Failed to get self address (engine.self() exception: {})",
                    ex.what());
        }

    } else {

        try {
            endpoint = engine.lookup(locator);
        } catch(const std::exception& ex) {
            throw Exception(
                    "Failed to lookup address {} "
                    "(engine.lookup() exception: {})",
                    locator, ex.what());
        }

        ProviderDescriptor descriptor;
        auto spec = type + ":" + std::to_string(provider_id);
        auto provider_manager_impl = self->m_provider_manager.lock();
        if (!provider_manager_impl) {
            throw Exception("Could not lookup provider: no ProviderManager found");
        }
        auto pid  = provider_manager_impl->get_provider_id();
        self->lookupRemoteProvider(endpoint, pid, spec, &descriptor);
    }

    auto name = type + ":" + std::to_string(provider_id) + "@" + static_cast<std::string>(endpoint);

    if (resolved) *resolved = name;

    auto ph = ProviderHandle{endpoint, provider_id};

    return std::make_shared<NamedDependency>(name, type, ph);
}

std::shared_ptr<NamedDependency>
DependencyFinder::makeProviderHandle(const std::string& type,
                                     const std::string& name,
                                     const std::string& locatorArg,
                                     std::string* resolved) const {
    auto locator = locatorArg;
    auto engine = MargoManager(self->m_margo_context).getThalliumEngine();
    thallium::endpoint endpoint;
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
        try {
              endpoint = engine.self();
          } catch(const std::exception& ex) {
              throw Exception(
                  "Failed to get self address (engine.self() exception: {})",
                  ex.what());
          }
          descriptor = ProviderDescriptor{
              provider->getName(),
              provider->getType(),
              provider->getProviderID()
          };
          provider_id = provider->getProviderID();

    } else {

        try {
            endpoint = engine.lookup(locator);
        } catch(const std::exception& ex) {
            throw Exception(
                    "Failed to lookup address {} "
                    "(engine.lookup() exception: {})",
                    locator, ex.what());
        }

        auto provider_manager_impl = self->m_provider_manager.lock();
        if (!provider_manager_impl) {
            throw Exception("Could not get provider id: no ProviderManager found");
        }
        auto pid = provider_manager_impl->get_provider_id();
        self->lookupRemoteProvider(endpoint, pid, name, &descriptor);
        provider_id = descriptor.provider_id;
    }

    auto ph_name = type + ":" + std::to_string(provider_id) + "@" + static_cast<std::string>(endpoint);

    if (resolved) *resolved = ph_name;

    auto ph = ProviderHandle{endpoint, provider_id};

    return std::make_shared<NamedDependency>(ph_name, type, ph);
}

} // namespace bedrock
