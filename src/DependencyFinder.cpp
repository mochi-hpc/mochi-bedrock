/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "DependencyFinderImpl.hpp"
#include "bedrock/DependencyFinder.hpp"
#include "bedrock/Exception.hpp"
#include <cctype>

namespace bedrock {

DependencyFinder::DependencyFinder(const MargoContext&    margo,
                                   const ABTioContext&    abtio,
                                   const ProviderManager& pmanager)
: self(std::make_shared<DependencyFinderImpl>()) {
    self->m_margo_context    = margo;
    self->m_abtio_context    = abtio;
    self->m_provider_manager = pmanager;
}

DependencyFinder::DependencyFinder(const DependencyFinder&) = default;

DependencyFinder::DependencyFinder(DependencyFinder&&) = default;

DependencyFinder& DependencyFinder::operator=(const DependencyFinder&)
    = default;

DependencyFinder& DependencyFinder::operator=(DependencyFinder&&) = default;

DependencyFinder::~DependencyFinder() = default;

DependencyFinder::operator bool() const { return static_cast<bool>(self); }

static bool isValidQualifier(const std::string& str) {
    if (str.empty()) return false;
    if (!isalpha(str[0]) && str[0] != '_') return false;
    for (unsigned i = 1; i < str.size(); i++)
        if (!isalnum(str[i])) return false;
    return true;
}

static bool isPositiveNumber(const std::string& str) {
    if (str.empty()) return false;
    for (unsigned i = 0; i < str.size(); i++)
        if (!isdigit(str[i])) return false;
    return true;
}

DependencyWrapper DependencyFinder::find(const std::string& type,
                                         const std::string& spec) const {
    if (type == "pool") { // Argobots pool
        ABT_pool p = MargoContext(self->m_margo_context).getPool(spec);
        if (p == ABT_POOL_NULL) {
            throw Exception("Could not find pool with name \"{}\"", spec);
        }
        return DependencyWrapper(p);
    } else if (type == "abt_io") { // ABTIO instance
        abt_io_instance_id abt_id
            = ABTioContext(self->m_abtio_context).getABTioInstance(spec);
        if (abt_id == ABT_IO_INSTANCE_NULL) {
            throw Exception("Could not find ABT-IO instance with name \"{}\"",
                            spec);
        }
        return DependencyWrapper(abt_id);
    } else { // Provider or provider handle

        // TODO use a regex (([a-zA-Z_][a-zA-Z0-9_]*)(:[0-9]+)?)(@(.+))?
        size_t      x          = spec.find('@');
        std::string identifier = spec.substr(0, x);
        std::string locator
            = (x != std::string::npos) ? spec.substr(x + 1) : "";
        if ((locator.empty() && (x != std::string::npos))
            || identifier.empty()) {
            throw Exception("Invalid dependency specification \"{}\"", spec);
        }
        size_t      y    = identifier.find(':');
        std::string name = identifier.substr(0, y);
        std::string id
            = (y != std::string::npos) ? identifier.substr(y + 1) : "";
        if (!isValidQualifier(name)) {
            throw Exception("Invalid dependency name \"{}\"", name);
        }
        if (y != std::string::npos && !isPositiveNumber(id) && id != "client") {
            throw Exception("Invalid provider id in \"{}\"", spec);
        }
        if (id == "client" && !locator.empty()) {
            throw Exception(
                "Client dependency cannot have a locator (in \"{}\")", spec);
        }
        if (locator.empty()) { // no locator, it's a local provider or a client
            if (id == "client") {
                spdlog::debug("Requesting a client for service {}", name);
                // TODO
            } else {
                uint16_t provider_id = atoi(id.c_str());
                spdlog::debug("Requesting provider {} from service {}",
                              provider_id, name);
                // TODO
            }
        } else {
            if (id.empty()) {
                spdlog::debug(
                    "Requesting provider handle to remote provider with name "
                    "{} at address {}",
                    name, locator);
                // TODO
            } else {
                uint16_t provider_id = atoi(id.c_str());
                spdlog::debug(
                    "Requesting provider handle to remote provider of type {} "
                    "with id {} at address {}",
                    name, provider_id, locator);
                // TODO
            }
        }
    }
    return DependencyWrapper();
}

} // namespace bedrock
