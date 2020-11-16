/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_SERVICE_HANDLE_IMPL_H
#define __BEDROCK_SERVICE_HANDLE_IMPL_H

#include <bedrock/UUID.hpp>

namespace bedrock {

class ServiceHandleImpl {

  public:
    UUID                        m_Service_id;
    std::shared_ptr<ClientImpl> m_client;
    tl::provider_handle         m_ph;

    ServiceHandleImpl() = default;

    ServiceHandleImpl(const std::shared_ptr<ClientImpl>& client,
                      tl::provider_handle&& ph, const UUID& Service_id)
    : m_Service_id(Service_id), m_client(client), m_ph(std::move(ph)) {}
};

} // namespace bedrock

#endif
