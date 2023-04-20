/*
 * (C) 2023 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __ALPHA_SERVICE_GROUP_HANDLE_IMPL_H
#define __ALPHA_SERVICE_GROUP_HANDLE_IMPL_H

#include "ClientImpl.hpp"
#include "ServiceHandleImpl.hpp"
#ifdef ENABLE_SSG
#include <ssg.h>
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

    ServiceGroupHandleImpl() = default;

    ServiceGroupHandleImpl(std::shared_ptr<ClientImpl> client,
                           uint16_t provider_id)
    : m_client(std::move(client)), m_provider_id(provider_id) {}

    ~ServiceGroupHandleImpl() {
#ifdef ENABLE_SSG
        if(m_owns_gid)
            ssg_group_destroy(m_gid);
#endif
    }
};

} // namespace bedrock

#endif
