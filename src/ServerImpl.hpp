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
#include "bedrock/ModuleContext.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace bedrock {

using nlohmann::json;
using namespace std::string_literals;
namespace tl = thallium;

class ServerImpl {

  public:
    std::shared_ptr<MargoContextImpl>     m_margo_context;
    std::shared_ptr<ABTioContextImpl>     m_abtio_context;
    std::shared_ptr<ProviderManagerImpl>  m_provider_manager;
    std::shared_ptr<DependencyFinderImpl> m_dependency_finder;
    std::shared_ptr<SSGContextImpl>       m_ssg_context;

    json makeConfig() const {
        auto config         = json::object();
        config["margo"]     = m_margo_context->makeConfig();
        config["abt_io"]    = m_abtio_context->makeConfig();
        config["providers"] = m_provider_manager->makeConfig();
        config["ssg"]       = m_ssg_context->makeConfig();
        config["libraries"] = json::parse(ModuleContext::getCurrentConfig());
        return config;
    }
};

} // namespace bedrock

#endif
