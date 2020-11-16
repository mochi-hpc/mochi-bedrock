/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/Admin.hpp"
#include "bedrock/Exception.hpp"
#include "bedrock/RequestResult.hpp"

#include "AdminImpl.hpp"

#include <thallium/serialization/stl/string.hpp>

namespace tl = thallium;

namespace bedrock {

Admin::Admin() = default;

Admin::Admin(const tl::engine& engine)
: self(std::make_shared<AdminImpl>(engine)) {}

Admin::Admin(margo_instance_id mid) : self(std::make_shared<AdminImpl>(mid)) {}

Admin::Admin(Admin&& other) = default;

Admin& Admin::operator=(Admin&& other) = default;

Admin::Admin(const Admin& other) = default;

Admin& Admin::operator=(const Admin& other) = default;

Admin::~Admin() = default;

Admin::operator bool() const { return static_cast<bool>(self); }

UUID Admin::createService(const std::string& address, uint16_t provider_id,
                          const std::string& Service_type,
                          const std::string& Service_config,
                          const std::string& token) const {
    auto                endpoint = self->m_engine.lookup(address);
    auto                ph       = tl::provider_handle(endpoint, provider_id);
    RequestResult<UUID> result
        = self->m_create_Service.on(ph)(token, Service_type, Service_config);
    if (not result.success()) { throw Exception(result.error()); }
    return result.value();
}

UUID Admin::openService(const std::string& address, uint16_t provider_id,
                        const std::string& Service_type,
                        const std::string& Service_config,
                        const std::string& token) const {
    auto                endpoint = self->m_engine.lookup(address);
    auto                ph       = tl::provider_handle(endpoint, provider_id);
    RequestResult<UUID> result
        = self->m_open_Service.on(ph)(token, Service_type, Service_config);
    if (not result.success()) { throw Exception(result.error()); }
    return result.value();
}

void Admin::closeService(const std::string& address, uint16_t provider_id,
                         const UUID&        Service_id,
                         const std::string& token) const {
    auto                endpoint = self->m_engine.lookup(address);
    auto                ph       = tl::provider_handle(endpoint, provider_id);
    RequestResult<bool> result
        = self->m_close_Service.on(ph)(token, Service_id);
    if (not result.success()) { throw Exception(result.error()); }
}

void Admin::destroyService(const std::string& address, uint16_t provider_id,
                           const UUID&        Service_id,
                           const std::string& token) const {
    auto                endpoint = self->m_engine.lookup(address);
    auto                ph       = tl::provider_handle(endpoint, provider_id);
    RequestResult<bool> result
        = self->m_destroy_Service.on(ph)(token, Service_id);
    if (not result.success()) { throw Exception(result.error()); }
}

void Admin::shutdownServer(const std::string& address) const {
    auto ep = self->m_engine.lookup(address);
    self->m_engine.shutdown_remote_engine(ep);
}

} // namespace bedrock
