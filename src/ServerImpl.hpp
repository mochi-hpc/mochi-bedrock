/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_SERVER_IMPL_H
#define __BEDROCK_SERVER_IMPL_H

#include "MargoContextImpl.hpp"
#include "ABTioContextImpl.hpp"
#include "ProviderManagerImpl.hpp"
#include "DependencyFinderImpl.hpp"
#include "SSGContextImpl.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace bedrock {

using namespace std::string_literals;
namespace tl = thallium;

class ServerImpl {

  public:
    std::shared_ptr<MargoContextImpl>     m_margo_context;
    std::shared_ptr<ABTioContextImpl>     m_abtio_context;
    std::shared_ptr<ProviderManagerImpl>  m_provider_manager;
    std::shared_ptr<DependencyFinderImpl> m_dependency_finder;
    std::shared_ptr<SSGContextImpl>       m_ssg_context;
};

} // namespace bedrock

#endif
