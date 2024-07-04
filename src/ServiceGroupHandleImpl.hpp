/*
 * (C) 2023 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __ALPHA_SERVICE_GROUP_HANDLE_IMPL_H
#define __ALPHA_SERVICE_GROUP_HANDLE_IMPL_H

#include <bedrock/Client.hpp>
#include <bedrock/DetailedException.hpp>
#include "ClientImpl.hpp"
#include "ServiceHandleImpl.hpp"
#include "Formatting.hpp"
#ifdef ENABLE_SSG
#include <ssg.h>
#endif
#ifdef ENABLE_FLOCK
#include <flock/flock-client.h>
#include <flock/flock-group.h>
#endif

namespace bedrock {



class ServiceGroupHandleImpl {

  public:

    std::shared_ptr<ClientImpl>                     m_client;
    uint16_t                                        m_provider_id;
    std::vector<std::shared_ptr<ServiceHandleImpl>> m_shs;

#ifdef ENABLE_SSG
    ssg_group_id_t                                  m_gid = SSG_GROUP_ID_INVALID;
    bool                                            m_owns_gid = false;
#endif
#ifdef ENABLE_FLOCK
    flock_client_t                                  m_flock_client = FLOCK_CLIENT_NULL;
    flock_group_handle_t                            m_flock_gh = FLOCK_GROUP_HANDLE_NULL;
#endif

    ServiceGroupHandleImpl() = default;

    ServiceGroupHandleImpl(std::shared_ptr<ClientImpl> client,
                           uint16_t provider_id)
    : m_client(std::move(client)), m_provider_id(provider_id) {}

    ~ServiceGroupHandleImpl() {
#ifdef ENABLE_SSG
        if(m_owns_gid)
            ssg_group_destroy(m_gid);
#endif
#ifdef ENABLE_FLOCK
        flock_group_handle_release(m_flock_gh);
        flock_client_finalize(m_flock_client);
#endif
    }

    std::vector<std::string> queryAddresses(bool refresh) const {
        std::vector<std::string> addresses;
#ifdef ENABLE_SSG
        if(m_gid) {
            auto mid = m_client->m_engine.get_margo_instance();
            int ret = SSG_SUCCESS;
            if(refresh)
                ret = ssg_group_refresh(mid, m_gid);
            if (ret != SSG_SUCCESS)
                throw BEDROCK_DETAILED_EXCEPTION(
                    "Could not refresh SSG group view "
                    "(ssg_group_refresh returned {})", ret);
            int group_size = 0;
            ret = ssg_get_group_size(m_gid, &group_size);
            if (ret != SSG_SUCCESS)
                throw BEDROCK_DETAILED_EXCEPTION(
                    "Could not get SSG group size "
                    "(ssg_get_group_size returned {})", ret);
            addresses.reserve(group_size);
            for (int i = 0; i < group_size; i++) {
                ssg_member_id_t member_id = SSG_MEMBER_ID_INVALID;
                ret = ssg_get_group_member_id_from_rank(m_gid, i, &member_id);
                if (member_id == SSG_MEMBER_ID_INVALID || ret != SSG_SUCCESS) {
                    throw BEDROCK_DETAILED_EXCEPTION(
                            "Could not get member ID from rank {} "
                            "(ssg_get_group_member_id_from_rank returned {})",
                            i, ret);
                }
                char* addr = NULL;
                ret        = ssg_get_group_member_addr_str(m_gid, member_id, &addr);
                if (addr == NULL || ret != SSG_SUCCESS) {
                    throw BEDROCK_DETAILED_EXCEPTION(
                            "Could not get address from SSG member {} (rank {}) "
                            "(ssg_get_group_member_addr_str returned {})", member_id,
                            i, ret);
                }
                addresses.emplace_back(addr);
            }
            return addresses;
        }
#endif
#if ENABLE_FLOCK
        if(m_flock_gh) {
            flock_return_t ret = flock_group_access_view(m_flock_gh,
                [](void* uargs, const flock_group_view_t* view) {
                    auto addresses_ptr = static_cast<decltype(&addresses)>(uargs);
                    for(size_t i = 0; i < view->members.size; ++i) {
                        if(!addresses_ptr->empty()
                        && addresses_ptr->back() == view->members.data[i].address)
                            continue;
                        addresses_ptr->emplace_back(view->members.data[i].address);
                    }
                }, static_cast<void*>(&addresses));
            if(ret != FLOCK_SUCCESS)
                throw BEDROCK_DETAILED_EXCEPTION(
                    "Could not get view from flock group handle: {}", std::to_string(ret));
            return addresses;
        }
#endif
        throw Exception{"ServiceGroupHandle not associated with an SSG or Flock group"};
    }

    static std::shared_ptr<ServiceGroupHandleImpl> FromSSGfile(
            std::shared_ptr<ClientImpl> client,
            const std::string& groupfile,
            uint16_t provider_id) {
#ifdef ENABLE_SSG
        int num_addrs = SSG_ALL_MEMBERS;
        ssg_group_id_t gid = SSG_GROUP_ID_INVALID;
        int ret = ssg_group_id_load(groupfile.c_str(), &num_addrs, &gid);
        if (ret != SSG_SUCCESS)
            throw BEDROCK_DETAILED_EXCEPTION("Could not load group file {} "
                    "(ssg_group_id_load returned {})", groupfile, ret);
        auto sg_impl = std::make_shared<ServiceGroupHandleImpl>(std::move(client), provider_id);
        sg_impl->m_gid = gid;
        sg_impl->m_owns_gid = true;
        return sg_impl;
#else
        (void)client;
        (void)groupfile;
        (void)provider_id;
        throw BEDROCK_DETAILED_EXCEPTION("Bedrock was not built with SSG support");
#endif
    }

    static std::shared_ptr<ServiceGroupHandleImpl> FromSSGid(
            std::shared_ptr<ClientImpl> client,
            uint64_t gid,
            uint16_t provider_id) {
#ifdef ENABLE_SSG
        std::vector<std::string> addresses;
        auto sg_impl = std::make_shared<ServiceGroupHandleImpl>(client, provider_id);
        sg_impl->m_gid = gid;
        return sg_impl;
#else
        (void)client;
        (void)gid;
        (void)provider_id;
        throw BEDROCK_DETAILED_EXCEPTION("Bedrock was not built with SSG support");
#endif
    }

    static std::shared_ptr<ServiceGroupHandleImpl> FromFlockFile(
            std::shared_ptr<ClientImpl> client,
            const std::string& groupfile,
            uint16_t provider_id) {
#ifdef ENABLE_FLOCK
        flock_return_t ret = FLOCK_SUCCESS;
        flock_client_t fclient = FLOCK_CLIENT_NULL;
        flock_group_handle_t fgh = FLOCK_GROUP_HANDLE_NULL;
        auto mid = client->m_engine.get_margo_instance();
        ret = flock_client_init(mid, ABT_POOL_NULL, &fclient);
        if(ret != FLOCK_SUCCESS)
            throw BEDROCK_DETAILED_EXCEPTION(
                "Could not create flock client: {}", std::to_string(ret));
        ret = flock_group_handle_create_from_file(
                fclient, groupfile.c_str(), 0, &fgh);
        if(ret != FLOCK_SUCCESS) {
            flock_client_finalize(fclient);
            throw BEDROCK_DETAILED_EXCEPTION(
                "Could not create flock group handle: {}", std::to_string(ret));
        }
        return FromFlockGroup(std::move(client), fgh, provider_id, fclient);
#else
        (void)client;
        (void)groupfile;
        (void)provider_id;
        throw BEDROCK_DETAILED_EXCEPTION("Bedrock was not built with Flock support");
#endif
    }

    static std::shared_ptr<ServiceGroupHandleImpl> FromFlockGroup(
            std::shared_ptr<ClientImpl> client,
            flock_group_handle_t gh,
            uint16_t provider_id,
            flock_client_t fc = nullptr) {
#ifdef ENABLE_FLOCK
        flock_return_t ret = FLOCK_SUCCESS;
        auto sg_impl = std::make_shared<ServiceGroupHandleImpl>(client, provider_id);
        ret = flock_group_handle_ref_incr(gh);
        if(ret != FLOCK_SUCCESS)
            throw BEDROCK_DETAILED_EXCEPTION(
                "Could not create ServiceGroupHandle from flock_group_handle_t: {} ",
                std::to_string(ret));
        sg_impl->m_flock_client = fc;
        sg_impl->m_flock_gh = gh;
        return sg_impl;
#else
        (void)client;
        (void)gh;
        (void)provider_id;
        (void)fc;
        throw BEDROCK_DETAILED_EXCEPTION("Bedrock was not built with Flock support");
#endif
    }
};

} // namespace bedrock

#endif
