/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_DEPENDENCY_FINDER_IMPL_H
#define __BEDROCK_DEPENDENCY_FINDER_IMPL_H

#include "MargoContextImpl.hpp"
#include "ABTioContextImpl.hpp"
#include "ProviderManagerImpl.hpp"
#include "bedrock/VoidPtr.hpp"
#include <string>
#include <unordered_map>

namespace bedrock {

class DependencyFinderImpl {

    using client_type = std::string;

  public:
    std::shared_ptr<MargoContextImpl>        m_margo_context;
    std::shared_ptr<ABTioContextImpl>        m_abtio_context;
    std::shared_ptr<ProviderManagerImpl>     m_provider_manager;
    std::unordered_map<client_type, VoidPtr> m_cached_clients;
};

} // namespace bedrock

#endif
