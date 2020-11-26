/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_DEPENDENCY_FINDER_IMPL_H
#define __BEDROCK_DEPENDENCY_FINDER_IMPL_H

#include "MargoContextImpl.hpp"
#include "ABTioContextImpl.hpp"
#include "SSGContextImpl.hpp"
#include "ProviderManagerImpl.hpp"
#include "bedrock/VoidPtr.hpp"
#include "bedrock/RequestResult.hpp"
#include "bedrock/ProviderWrapper.hpp"
#include "bedrock/Exception.hpp"
#include <thallium.hpp>
#include <string>
#include <unordered_map>

namespace tl = thallium;

namespace bedrock {

class DependencyFinderImpl {

    using client_type = std::string;

  public:
    tl::engine                               m_engine;
    std::shared_ptr<MargoContextImpl>        m_margo_context;
    std::shared_ptr<ABTioContextImpl>        m_abtio_context;
    std::shared_ptr<SSGContextImpl>          m_ssg_context;
    std::shared_ptr<ProviderManagerImpl>     m_provider_manager;
    std::unordered_map<client_type, VoidPtr> m_cached_clients;

    tl::remote_procedure m_lookup_provider;

    DependencyFinderImpl(const tl::engine& engine)
    : m_engine(engine),
      m_lookup_provider(m_engine.define("bedrock_lookup_provider")) {}

    void lookupRemoteProvider(hg_addr_t addr, uint16_t provider_id,
                              const std::string& spec, double timeout,
                              ProviderDescriptor* desc) {
        auto ph = tl::provider_handle(tl::endpoint(m_engine, addr, false),
                                      provider_id);
        RequestResult<ProviderDescriptor> result
            = m_lookup_provider.on(ph)(spec, timeout);
        if (result.error() != "") throw Exception(result.error());
        if (desc) *desc = result.value();
    }
};

} // namespace bedrock

#endif
