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
    margo_instance_id mid = self->m_engine.get_margo_instance();
    int ret = ssg_group_id_load(groupfile.c_str(), &num_addrs, &gid);
    if (ret != SSG_SUCCESS)
        throw Exception("Could not load group file {} "
            "(ssg_group_id_load returned {})", groupfile, ret);
    ret = ssg_group_refresh(mid, gid);
    if (ret != SSG_SUCCESS)
        throw Exception("Could not refresh SSG group view "
            "(ssg_group_refresh returned {})", ret);
    int group_size = 0;
    ret = ssg_get_group_size(gid, &group_size);
    if (ret != SSG_SUCCESS)
        throw Exception("Could not get SSG group size "
            "(ssg_get_group_size returned {})", ret);
    addresses.reserve(group_size);
    for (int i = 0; i < group_size; i++) {
        ssg_member_id_t member_id = SSG_MEMBER_ID_INVALID;
        ret = ssg_get_group_member_id_from_rank(gid, i, &member_id);
        if (member_id == SSG_MEMBER_ID_INVALID || ret != SSG_SUCCESS) {
            throw Exception("Could not get member ID from rank {} "
                "(ssg_get_group_member_id_from_rank returned {})",
                i, ret);
        }
        char* addr = NULL;
        ret        = ssg_get_group_member_addr_str(gid, member_id, &addr);
        if (addr == NULL || ret != SSG_SUCCESS) {
            throw Exception(
                "Could not get address from SSG member {} (rank {}) "
                "(ssg_get_group_member_addr_str returned {})", member_id,
                i, ret);
        }
        addresses.emplace_back(addr);
      }
      auto sgh = makeServiceGroupHandle(addresses, provider_id);
      sgh.self->m_gid = gid;
      return sgh;
#else
    (void)groupfile;
    (void)provider_id;
    throw Exception("Bedrock was not built with SSG support");
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
    auto sg_impl = std::make_shared<ServiceGroupHandleImpl>(self, std::move(shs));
    return ServiceGroupHandle(std::move(sg_impl));
}

} // namespace bedrock

extern "C" int bedrock_client_init(margo_instance_id mid, bedrock_client_t* client) {
    *client = static_cast<void*>(new bedrock::Client(mid));
    return 0;
}

extern "C" int bedrock_client_finalize(bedrock_client_t client) {
    delete static_cast<bedrock::Client*>(client);
    return 0;
}
