/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/Exception.hpp"
#include "bedrock/Client.hpp"
#include "bedrock/ServiceHandle.hpp"
#include "bedrock/ServiceGroupHandle.hpp"
#include "bedrock/RequestResult.hpp"
#include "bedrock/client.h"

#include "ClientImpl.hpp"
#include "ServiceHandleImpl.hpp"
#include "ServiceGroupHandleImpl.hpp"
#include "Exception.hpp"

#include <thallium/serialization/stl/string.hpp>
#ifdef ENABLE_SSG
#include <ssg.h>
#endif

namespace tl = thallium;

namespace bedrock {

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

const tl::engine& Client::engine() const { return self->m_engine; }

Client::operator bool() const { return static_cast<bool>(self); }

ServiceHandle Client::makeServiceHandle(const std::string& address,
                                        uint16_t           provider_id) const {
    auto endpoint = self->m_engine.lookup(address);
    auto ph       = tl::provider_handle(endpoint, provider_id);
    auto service_impl
        = std::make_shared<ServiceHandleImpl>(self, std::move(ph));
    return ServiceHandle(service_impl);
}

ServiceGroupHandle Client::makeServiceGroupHandle(
        const std::string& groupfile,
        uint16_t provider_id) const {
#ifdef ENABLE_SSG
    std::vector<std::string> addresses;
    int num_addrs = SSG_ALL_MEMBERS;
    ssg_group_id_t gid = SSG_GROUP_ID_INVALID;
    int ret = ssg_group_id_load(groupfile.c_str(), &num_addrs, &gid);
    if (ret != SSG_SUCCESS)
        throw DETAILED_EXCEPTION("Could not load group file {} "
            "(ssg_group_id_load returned {})", groupfile, ret);
    auto sg_impl = std::make_shared<ServiceGroupHandleImpl>(self, provider_id);
    sg_impl->m_gid = gid;
    sg_impl->m_owns_gid = true;
    auto result = ServiceGroupHandle(std::move(sg_impl));
    result.refresh();
    return result;
#else
    (void)groupfile;
    (void)provider_id;
    throw DETAILED_EXCEPTION("Bedrock was not built with SSG support");
#endif
}

ServiceGroupHandle Client::makeServiceGroupHandle(
        uint64_t gid,
        uint16_t provider_id) const {
#ifdef ENABLE_SSG
    std::vector<std::string> addresses;
    auto sg_impl = std::make_shared<ServiceGroupHandleImpl>(self, provider_id);
    sg_impl->m_gid = gid;
    auto result = ServiceGroupHandle(std::move(sg_impl));
    result.refresh();
    return result;
#else
    (void)gid;
    (void)provider_id;
    throw DETAILED_EXCEPTION("Bedrock was not built with SSG support");
#endif
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
