/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_DEPENDENCY_FINDER_IMPL_H
#define __BEDROCK_DEPENDENCY_FINDER_IMPL_H

#include "MargoManagerImpl.hpp"
#include "ProviderManagerImpl.hpp"
#include "Formatting.hpp"
#include "MPIEnvImpl.hpp"
#include "bedrock/VoidPtr.hpp"
#include "bedrock/RequestResult.hpp"
#include <bedrock/Exception.hpp>
#include <thallium.hpp>
#include <string>
#include <unordered_map>

namespace tl = thallium;

namespace bedrock {

class DependencyFinderImpl {

    using client_type = std::string;

  public:
    tl::engine                         m_engine;
    std::shared_ptr<MPIEnvImpl>        m_mpi;
    std::shared_ptr<MargoManagerImpl>  m_margo_context;
    std::weak_ptr<ProviderManagerImpl> m_provider_manager;
    double                             m_timeout = 30.0;

    tl::remote_procedure m_lookup_provider;

    DependencyFinderImpl(const tl::engine& engine)
    : m_engine(engine),
      m_lookup_provider(m_engine.define("bedrock_lookup_provider")) {
        spdlog::trace("DependencyFinderImpl initialized");
    }

    ~DependencyFinderImpl() {
        spdlog::trace("DependencyFinderImpl destroyed");
    }

    void lookupRemoteProvider(const tl::endpoint& addr, uint16_t provider_id,
                              const std::string&  spec,
                              ProviderDescriptor* desc) {
        auto ph = tl::provider_handle(addr, provider_id);
        RequestResult<ProviderDescriptor> result
            = m_lookup_provider.on(ph)(spec, m_timeout);
        if (result.error() != "") throw Exception(result.error());
        if (desc) *desc = result.value();
    }
};

} // namespace bedrock

#endif
