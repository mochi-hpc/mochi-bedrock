/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __ALPHA_SERVICE_HANDLE_IMPL_H
#define __ALPHA_SERVICE_HANDLE_IMPL_H

#include "ClientImpl.hpp"

namespace bedrock {

class ServiceHandleImpl {

  public:
    std::shared_ptr<ClientImpl> m_client;
    tl::provider_handle         m_ph;

    ServiceHandleImpl() = default;

    ServiceHandleImpl(const std::shared_ptr<ClientImpl>& client,
                      tl::provider_handle&&              ph)
    : m_client(client), m_ph(std::move(ph)) {}
};

} // namespace bedrock

#endif
