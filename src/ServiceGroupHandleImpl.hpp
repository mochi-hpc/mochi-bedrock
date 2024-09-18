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

#ifdef ENABLE_FLOCK
    flock_client_t                                  m_flock_client = FLOCK_CLIENT_NULL;
    flock_group_handle_t                            m_flock_gh = FLOCK_GROUP_HANDLE_NULL;
#endif

    ServiceGroupHandleImpl() = default;

    ServiceGroupHandleImpl(std::shared_ptr<ClientImpl> client,
                           uint16_t provider_id)
    : m_client(std::move(client)), m_provider_id(provider_id) {}

    ~ServiceGroupHandleImpl() {
#ifdef ENABLE_FLOCK
        if(m_flock_gh) flock_group_handle_release(m_flock_gh);
        if(m_flock_client) flock_client_finalize(m_flock_client);
#endif
    }

    std::vector<std::string> queryAddresses(bool refresh) const {
        std::vector<std::string> addresses;
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
        std::shared_ptr<ServiceGroupHandleImpl> result;
        try {
            result = FromFlockGroup(std::move(client), fgh, provider_id, fclient);
        } catch(...) {
            flock_group_handle_release(fgh);
            throw;
        }
        flock_group_handle_release(fgh);
        return result;
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
