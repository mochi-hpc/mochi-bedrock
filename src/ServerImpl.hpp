/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_SERVER_IMPL_H
#define __BEDROCK_SERVER_IMPL_H

#include "MargoManagerImpl.hpp"
#include "ProviderManagerImpl.hpp"
#include "DependencyFinderImpl.hpp"
#include "Jx9ManagerImpl.hpp"
#include "MPIEnvImpl.hpp"
#include "bedrock/Jx9Manager.hpp"
#include "bedrock/RequestResult.hpp"
#include "bedrock/ModuleManager.hpp"
#include <thallium/serialization/stl/string.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace bedrock {

using nlohmann::json;
using namespace std::string_literals;
namespace tl = thallium;

class ServerImpl;

class BedrockProviderImpl : public tl::provider<BedrockProviderImpl> {
    ServerImpl* m_impl;
public:
    tl::remote_procedure m_get_config_rpc;
    tl::remote_procedure m_query_config_rpc;
    tl::remote_procedure m_add_pool_rpc;
    tl::remote_procedure m_add_xstream_rpc;
    tl::remote_procedure m_remove_pool_rpc;
    tl::remote_procedure m_remove_xstream_rpc;

    BedrockProviderImpl(tl::engine& engine, uint16_t provider_id,
                      tl::pool pool, ServerImpl* impl);

    void getConfigRPC(const tl::request& req);
    void queryConfigRPC(const tl::request& req, const std::string& s);
    void addPoolRPC(const tl::request& req, const std::string& config);
    void removePoolRPC(const tl::request& req, const std::string& name);
    void addXstreamRPC(const tl::request& req, const std::string& config);
    void removeXstreamRPC(const tl::request& req, const std::string& name);

    ~BedrockProviderImpl() = default;
};

class ServerImpl {

  public:
    std::shared_ptr<MPIEnvImpl>           m_mpi;
    std::shared_ptr<Jx9ManagerImpl>       m_jx9_manager;
    std::shared_ptr<MargoManagerImpl>     m_margo_manager;
    std::shared_ptr<ProviderManagerImpl>  m_provider_manager;
    std::shared_ptr<DependencyFinderImpl> m_dependency_finder;
    std::shared_ptr<NamedDependency>      m_pool;
    uint16_t                              m_provider_id;

    std::vector<std::unique_ptr<BedrockProviderImpl>> m_providers;

    ServerImpl(std::shared_ptr<MargoManagerImpl> margo, uint16_t provider_id,
               std::shared_ptr<NamedDependency> pool)
    : m_margo_manager(std::move(margo)),
      m_pool(pool),
      m_provider_id(provider_id)
    {
        auto mgr = MargoManager(m_margo_manager);
        for (size_t i = 0; i < mgr.getNumEngines(); ++i) {
            auto& engine = const_cast<tl::engine&>(mgr.getThalliumEngine(i));
            auto handler_pool = mgr.getDefaultHandlerPool(i);
            m_providers.push_back(std::make_unique<BedrockProviderImpl>(
                engine, provider_id, tl::pool(handler_pool->getHandle<tl::pool>()), this));
        }
    }

    json makeConfig() const {
        auto config         = json::object();
        config["margo"]     = m_margo_manager->makeConfig();
        config["providers"] = m_provider_manager->makeConfig();
        config["libraries"] = json::parse(ModuleManager::getCurrentConfig());
        config["bedrock"]   = json::object();
        config["bedrock"]["pool"] = m_pool->getName();
        config["bedrock"]["provider_id"] = m_provider_id;
        return config;
    }

    void getConfigRPC(const tl::request& req) {
        RequestResult<std::string> result;
        result.value() = makeConfig().dump();
        req.respond(result);
    }

    void queryConfigRPC(const tl::request& req, const std::string& script) {
        RequestResult<std::string> result;
        try {
            std::unordered_map<std::string, std::string> args;
            args["__config__"] = makeConfig().dump();
            result.value()
                = Jx9Manager(m_jx9_manager).executeQuery(script, args);
            result.success() = true;
        } catch (const Exception& ex) {
            result.error()   = ex.what();
            result.success() = false;
        }
        req.respond(result);
    }

    void addPoolRPC(const tl::request& req, const std::string& config) {
        RequestResult<bool> result;
        result.success() = true;
        try {
            MargoManager(m_margo_manager).addPool(config);
        } catch (const Exception& ex) {
            result.error() = ex.what();
            result.success() = false;
        }
        req.respond(result);
    }

    void removePoolRPC(const tl::request& req, const std::string& name) {
        RequestResult<bool> result;
        result.success() = true;
        try {
            MargoManager(m_margo_manager).removePool(name);
        } catch (const Exception& ex) {
            result.error() = ex.what();
            result.success() = false;
        }
        req.respond(result);
    }

    void addXstreamRPC(const tl::request& req, const std::string& config) {
        RequestResult<bool> result;
        result.success() = true;
        try {
            MargoManager(m_margo_manager).addXstream(config);
        } catch (const Exception& ex) {
            result.error() = ex.what();
            result.success() = false;
        }
        req.respond(result);
    }

    void removeXstreamRPC(const tl::request& req, const std::string& name) {
        RequestResult<bool> result;
        result.success() = true;
        try {
            MargoManager(m_margo_manager).removeXstream(name);
        } catch (const Exception& ex) {
            result.error() = ex.what();
            result.success() = false;
        }
        req.respond(result);
    }
};

// --- BedrockProviderImpl implementation ---

inline BedrockProviderImpl::BedrockProviderImpl(tl::engine& engine, uint16_t provider_id,
                                            tl::pool pool, ServerImpl* impl)
: tl::provider<BedrockProviderImpl>(engine, provider_id, "bedrock"),
  m_impl(impl),
  m_get_config_rpc(
      define("bedrock_get_config", &BedrockProviderImpl::getConfigRPC, pool)),
  m_query_config_rpc(
      define("bedrock_query_config", &BedrockProviderImpl::queryConfigRPC, pool)),
  m_add_pool_rpc(
      define("bedrock_add_pool", &BedrockProviderImpl::addPoolRPC, pool)),
  m_add_xstream_rpc(
      define("bedrock_add_xstream", &BedrockProviderImpl::addXstreamRPC, pool)),
  m_remove_pool_rpc(
      define("bedrock_remove_pool", &BedrockProviderImpl::removePoolRPC, pool)),
  m_remove_xstream_rpc(
      define("bedrock_remove_xstream", &BedrockProviderImpl::removeXstreamRPC, pool))
{}

inline void BedrockProviderImpl::getConfigRPC(const tl::request& req) {
    m_impl->getConfigRPC(req);
}

inline void BedrockProviderImpl::queryConfigRPC(const tl::request& req, const std::string& s) {
    m_impl->queryConfigRPC(req, s);
}

inline void BedrockProviderImpl::addPoolRPC(const tl::request& req, const std::string& config) {
    m_impl->addPoolRPC(req, config);
}

inline void BedrockProviderImpl::removePoolRPC(const tl::request& req, const std::string& name) {
    m_impl->removePoolRPC(req, name);
}

inline void BedrockProviderImpl::addXstreamRPC(const tl::request& req, const std::string& config) {
    m_impl->addXstreamRPC(req, config);
}

inline void BedrockProviderImpl::removeXstreamRPC(const tl::request& req, const std::string& name) {
    m_impl->removeXstreamRPC(req, name);
}

} // namespace bedrock

#endif
