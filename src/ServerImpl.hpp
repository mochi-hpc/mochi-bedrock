/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_SERVER_IMPL_H
#define __BEDROCK_SERVER_IMPL_H

#include "MargoManagerImpl.hpp"
#include "ABTioManagerImpl.hpp"
#include "MonaManagerImpl.hpp"
#include "ProviderManagerImpl.hpp"
#include "ClientManagerImpl.hpp"
#include "DependencyFinderImpl.hpp"
#include "SSGManagerImpl.hpp"
#include "Jx9ManagerImpl.hpp"
#include "bedrock/Jx9Manager.hpp"
#include "bedrock/RequestResult.hpp"
#include "bedrock/ModuleContext.hpp"
#include <thallium/serialization/stl/string.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace bedrock {

using nlohmann::json;
using namespace std::string_literals;
namespace tl = thallium;

class ServerImpl : public tl::provider<ServerImpl> {

  public:
    std::shared_ptr<MargoManagerImpl>     m_margo_manager;
    std::shared_ptr<ABTioManagerImpl>     m_abtio_manager;
    std::shared_ptr<MonaManagerImpl>      m_mona_manager;
    std::shared_ptr<SSGManagerImpl>       m_ssg_manager;
    std::shared_ptr<ClientManagerImpl>    m_client_manager;
    std::shared_ptr<ProviderManagerImpl>  m_provider_manager;
    std::shared_ptr<Jx9ManagerImpl>       m_jx9_manager;
    std::shared_ptr<DependencyFinderImpl> m_dependency_finder;
    tl::pool                              m_pool;

    tl::remote_procedure m_get_config_rpc;
    tl::remote_procedure m_query_config_rpc;
    tl::remote_procedure m_create_client_rpc;
    tl::remote_procedure m_create_abtio_rpc;
    tl::remote_procedure m_add_ssg_group_rpc;

    tl::remote_procedure m_add_pool_rpc;
    tl::remote_procedure m_add_xstream_rpc;
    tl::remote_procedure m_remove_pool_rpc;
    tl::remote_procedure m_remove_xstream_rpc;

    ServerImpl(const tl::engine& engine, uint16_t provider_id,
               const tl::pool& pool)
    : tl::provider<ServerImpl>(engine, provider_id), m_pool(pool),
      m_get_config_rpc(
          define("bedrock_get_config", &ServerImpl::getConfigRPC, pool)),
      m_query_config_rpc(
          define("bedrock_query_config", &ServerImpl::queryConfigRPC, pool)),
      m_create_client_rpc(
          define("bedrock_create_client", &ServerImpl::createClientRPC, pool)),
      m_create_abtio_rpc(
          define("bedrock_create_abtio", &ServerImpl::createABTioRPC, pool)),
      m_add_ssg_group_rpc(
          define("bedrock_add_ssg_group", &ServerImpl::addSSGgroupRPC, pool)),
      m_add_pool_rpc(
          define("bedrock_add_pool", &ServerImpl::addPoolRPC, pool)),
      m_add_xstream_rpc(
          define("bedrock_add_xstream", &ServerImpl::addXstreamRPC, pool)),
      m_remove_pool_rpc(
          define("bedrock_remove_pool", &ServerImpl::removePoolRPC, pool)),
      m_remove_xstream_rpc(
          define("bedrock_remove_xstream", &ServerImpl::removeXstreamRPC, pool))
    {}

    ~ServerImpl() {
        m_get_config_rpc.deregister();
        m_query_config_rpc.deregister();
        m_create_client_rpc.deregister();
        m_create_abtio_rpc.deregister();
        m_add_ssg_group_rpc.deregister();
        m_add_pool_rpc.deregister();
        m_add_xstream_rpc.deregister();
        m_remove_pool_rpc.deregister();
        m_remove_xstream_rpc.deregister();
    }

    json makeConfig() const {
        auto config         = json::object();
        config["margo"]     = m_margo_manager->makeConfig();
        config["abt_io"]    = m_abtio_manager->makeConfig();
        config["clients"]   = m_client_manager->makeConfig();
        config["providers"] = m_provider_manager->makeConfig();
        config["ssg"]       = m_ssg_manager->makeConfig();
        config["libraries"] = json::parse(ModuleContext::getCurrentConfig());
        config["bedrock"]   = json::object();
        auto pool_info
            = MargoManager(m_margo_manager).getPool(m_pool.native_handle());
        if (pool_info.pool != ABT_POOL_NULL) config["bedrock"]["pool"] = pool_info.name;
        config["bedrock"]["provider_id"] = get_provider_id();
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

    void createClientRPC(const tl::request& req, const std::string& name,
                         const std::string& type, const std::string& config,
                         const DependencyMap& dependencies) {
        RequestResult<bool> result;
        json                jsonconfig;
        try {
            if (!config.empty())
                jsonconfig = json::parse(config);
            else
                jsonconfig = json::object();
        } catch (...) {
            result.error()   = "Invalid JSON configuration for client";
            result.success() = false;
            req.respond(result);
            return;
        }
        json fullconfig      = json::object();
        fullconfig["name"]   = name;
        fullconfig["type"]   = type;
        fullconfig["config"] = jsonconfig;
        auto& depconfig      = fullconfig["dependencies"];
        for (auto& p : dependencies) {
            auto& name = p.first;
            auto& list = p.second;
            auto  dep  = json::array();
            for (auto& v : list) { dep.push_back(v); }
            depconfig[name] = dep;
        }
        try {
            ClientManager(m_client_manager)
                .addClientFromJSON(fullconfig.dump(),
                                   DependencyFinder(m_dependency_finder));
        } catch (const Exception& ex) {
            result.error()   = ex.what();
            result.success() = false;
        }
        req.respond(result);
    }

    void createABTioRPC(const tl::request& req, const std::string& name,
                        const std::string& pool, const std::string& config) {
        RequestResult<bool> result;
        result.success() = true;
        try {
            ABTioManager(m_abtio_manager).addABTioInstance(name, pool, config);
        } catch (const Exception& ex) {
            result.error()   = ex.what();
            result.success() = false;
        }
        req.respond(result);
    }

    void addSSGgroupRPC(const tl::request& req, const std::string& config) {
        RequestResult<bool> result;
        result.success() = true;
        try {
            SSGManager(m_ssg_manager).createGroupFromConfig(config);
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

} // namespace bedrock

#endif
