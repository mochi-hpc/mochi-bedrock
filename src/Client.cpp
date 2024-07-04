/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <bedrock/DetailedException.hpp>
#include <bedrock/Client.hpp>
#include <bedrock/ServiceHandle.hpp>
#include <bedrock/ServiceGroupHandle.hpp>
#include <bedrock/RequestResult.hpp>

#include "ClientImpl.hpp"
#include "ServiceHandleImpl.hpp"
#include "ServiceGroupHandleImpl.hpp"

#include <thallium/serialization/stl/string.hpp>
#ifdef ENABLE_SSG
#include <ssg.h>
#endif

namespace tl = thallium;

namespace bedrock {

// LCOV_EXCL_START

Client::Client() = default;

Client::Client(const tl::engine& engine)
: self(std::make_shared<ClientImpl>(engine)) {}

Client::Client(margo_instance_id mid)
: self(std::make_shared<ClientImpl>(mid)) {}

Client::Client(const std::shared_ptr<ClientImpl>& impl) : self(impl) {}

Client::Client(Client&& other) = default;

Client& Client::operator=(Client&& other) = default;

Client::Client(const Client& other) = default;

Client& Client::operator=(const Client& other) = default;

Client::~Client() = default;

Client::operator bool() const { return static_cast<bool>(self); }

// LCOV_EXCL_STOP

const tl::engine& Client::engine() const { return self->m_engine; }

ServiceHandle Client::makeServiceHandle(const std::string& address,
                                        uint16_t           provider_id) const {
    auto endpoint = self->m_engine.lookup(address);
    auto ph       = tl::provider_handle(endpoint, provider_id);
    auto service_impl
        = std::make_shared<ServiceHandleImpl>(self, std::move(ph));
    return ServiceHandle(service_impl);
}

ServiceGroupHandle Client::makeServiceGroupHandleFromSSGFile(
        const std::string& groupfile,
        uint16_t provider_id) const {
    auto impl = ServiceGroupHandleImpl::FromSSGfile(self, groupfile, provider_id);
    auto result = ServiceGroupHandle{std::move(impl)};
    result.refresh();
    return result;
}

ServiceGroupHandle Client::makeServiceGroupHandleFromSSGGroup(
        uint64_t gid,
        uint16_t provider_id) const {
    auto impl = ServiceGroupHandleImpl::FromSSGid(self, gid, provider_id);
    auto result = ServiceGroupHandle{std::move(impl)};
    result.refresh();
    return result;
}

ServiceGroupHandle Client::makeServiceGroupHandleFromFlockFile(
        const std::string& groupfile,
        uint16_t provider_id) const {
    auto impl = ServiceGroupHandleImpl::FromFlockFile(self, groupfile, provider_id);
    auto result = ServiceGroupHandle{std::move(impl)};
    result.refresh();
    return result;
}

ServiceGroupHandle Client::makeServiceGroupHandleFromFlockGroup(
        flock_group_handle_t handle,
        uint16_t provider_id) const {
    auto impl = ServiceGroupHandleImpl::FromFlockGroup(self, handle, provider_id);
    auto result = ServiceGroupHandle{std::move(impl)};
    result.refresh();
    return result;
}

ServiceGroupHandle Client::makeServiceGroupHandle(
        const std::vector<std::string>& addresses,
        uint16_t provider_id) const {
    std::vector<std::shared_ptr<ServiceHandleImpl>> shs;
    shs.reserve(addresses.size());
    for(const auto& addr : addresses) {
        auto sh = makeServiceHandle(addr, provider_id);
        shs.push_back(sh.self);
    }
    auto sg_impl = std::make_shared<ServiceGroupHandleImpl>(self, provider_id);
    sg_impl->m_shs = std::move(shs);
    return ServiceGroupHandle(std::move(sg_impl));
}

} // namespace bedrock
