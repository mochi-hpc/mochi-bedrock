/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_SERVER_IMPL_H
#define __BEDROCK_SERVER_IMPL_H

#include "MargoManagerImpl.hpp"
#include "ABTioManagerImpl.hpp"
#include "ProviderManagerImpl.hpp"
#include "DependencyFinderImpl.hpp"
#include "SSGManagerImpl.hpp"
#include "bedrock/ModuleContext.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace bedrock {

using nlohmann::json;
using namespace std::string_literals;
namespace tl = thallium;

class ServerImpl {

  public:
    std::shared_ptr<MargoManagerImpl>     m_margo_manager;
    std::shared_ptr<ABTioManagerImpl>     m_abtio_manager;
    std::shared_ptr<ProviderManagerImpl>  m_provider_manager;
    std::shared_ptr<DependencyFinderImpl> m_dependency_finder;
    std::shared_ptr<SSGManagerImpl>       m_ssg_manager;

    json makeConfig() const {
        auto config         = json::object();
        config["margo"]     = m_margo_manager->makeConfig();
        config["abt_io"]    = m_abtio_manager->makeConfig();
        config["providers"] = m_provider_manager->makeConfig();
        config["ssg"]       = m_ssg_manager->makeConfig();
        config["libraries"] = json::parse(ModuleContext::getCurrentConfig());
        return config;
    }
};

} // namespace bedrock

#endif
