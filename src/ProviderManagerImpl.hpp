/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_PROVIDER_MANAGER_IMPL_H
#define __BEDROCK_PROVIDER_MANAGER_IMPL_H

#include "MargoManagerImpl.hpp"
#include "bedrock/DependencyFinder.hpp"
#include "bedrock/DependencyMap.hpp"
#include "bedrock/RequestResult.hpp"
#include "bedrock/AbstractComponent.hpp"
#include "bedrock/ProviderDescriptor.hpp"
#include "bedrock/Jx9Manager.hpp"
#include "bedrock/Exception.hpp"

#include <thallium/serialization/stl/vector.hpp>
#include <thallium/serialization/stl/unordered_map.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thallium.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>

namespace bedrock {

using nlohmann::json;
using namespace std::string_literals;
namespace tl = thallium;

class LocalProvider : public ProviderDependency {

    public:

    std::vector<Dependency>  requested_dependencies;
    ResolvedDependencyMap    resolved_dependencies;
    std::vector<std::string> tags;

    LocalProvider(
            std::string name, std::string type, uint16_t provider_id, ComponentPtr ptr,
            std::vector<Dependency> req_deps, ResolvedDependencyMap res_deps,
            std::vector<std::string> _tags)
        : ProviderDependency(std::move(name), std::move(type), ptr, provider_id)
        , requested_dependencies(std::move(req_deps))
        , resolved_dependencies(std::move(res_deps))
        , tags(std::move(_tags))
    {
    }

    json makeConfig() const {
        ComponentPtr ptr  = getHandle<ComponentPtr>();
        auto c            = json::object();
        c["name"]         = getName();
        c["type"]         = getType();
        c["provider_id"]  = getProviderID();
        c["config"]       = json::parse(ptr->getConfig());
        c["tags"]         = json::array();
        for(auto& t : tags) c["tags"].push_back(t);
        c["dependencies"] = json::object();
        auto& d           = c["dependencies"];
        for (auto& dep : requested_dependencies) {
            auto res_dep = resolved_dependencies.find(dep.name);
            if(res_dep == resolved_dependencies.end()) continue;
            if(dep.is_array) {
                d[dep.name] = json::array();
                for(auto& x : res_dep->second) {
                    d[dep.name].push_back(x->getName());
                }
            } else {
                d[dep.name] = res_dep->second[0]->getName();
            }
        }
        return c;
    }
};

class ProviderManagerImpl
: public tl::provider<ProviderManagerImpl>,
  public std::enable_shared_from_this<ProviderManagerImpl> {

  public:
    std::shared_ptr<DependencyFinderImpl>       m_dependency_finder;
    std::vector<std::shared_ptr<LocalProvider>> m_providers;
    mutable tl::mutex                           m_providers_mtx;
    mutable tl::condition_variable              m_providers_cv;

    std::shared_ptr<MargoManagerImpl> m_margo_manager;
    std::shared_ptr<Jx9ManagerImpl>   m_jx9_manager;

    tl::auto_remote_procedure m_lookup_provider;
    tl::auto_remote_procedure m_load_module;
    tl::auto_remote_procedure m_start_provider;
    tl::auto_remote_procedure m_migrate_provider;
    tl::auto_remote_procedure m_snapshot_provider;
    tl::auto_remote_procedure m_restore_provider;

    ProviderManagerImpl(const tl::engine& engine, uint16_t provider_id,
                        const tl::pool& pool)
    : tl::provider<ProviderManagerImpl>(engine, provider_id),
      m_lookup_provider(define("bedrock_lookup_provider",
                               &ProviderManagerImpl::lookupProviderRPC, pool)),
      m_load_module(define("bedrock_load_module",
                           &ProviderManagerImpl::loadModuleRPC, pool)),
      m_start_provider(define("bedrock_start_provider",
                              &ProviderManagerImpl::startProviderRPC, pool)),
      m_migrate_provider(define("bedrock_migrate_provider",
                                 &ProviderManagerImpl::migrateProviderRPC, pool)),
      m_snapshot_provider(define("bedrock_snapshot_provider",
                                 &ProviderManagerImpl::snapshotProviderRPC, pool)),
      m_restore_provider(define("bedrock_restore_provider",
                                 &ProviderManagerImpl::restoreProviderRPC, pool))
    {
        spdlog::trace("ProviderManagerImpl initialized");
    }

    ~ProviderManagerImpl() {
        spdlog::trace("ProviderManagerImpl destroyed");
    }

    uint16_t getAvailableProviderID() const {
        std::unordered_set<uint16_t> used_provider_ids;
        for(auto& p : m_providers) {
            used_provider_ids.insert(p->getProviderID());
        }
        for(uint16_t i=0; i < std::numeric_limits<uint16_t>::max()-1; ++i) {
            if(used_provider_ids.find(i) == used_provider_ids.end())
                return i;
        }
        return 0;
    }

    auto resolveSpec(const std::string& type, uint16_t provider_id) {
        return std::find_if(m_providers.begin(), m_providers.end(),
                            [&type, &provider_id](const auto& p) {
                                return p->getType() == type
                                    && p->getProviderID() == provider_id;
                            });
    }

    auto resolveSpec(const std::string& spec) {
        auto                          column = spec.find(':');
        decltype(m_providers.begin()) it;
        if (column == std::string::npos) {
            it = std::find_if(m_providers.begin(), m_providers.end(),
                              [&spec](const auto& p) {
                                  return p->getName() == spec;
                              });
        } else {
            auto     type            = spec.substr(0, column);
            auto     provider_id_str = spec.substr(column + 1);
            uint16_t provider_id     = atoi(provider_id_str.c_str());
            it                       = resolveSpec(type, provider_id);
        }
        return it;
    }

    json makeConfig() const {
        auto                       config = json::array();
        std::lock_guard<tl::mutex> lock(m_providers_mtx);
        for (auto& p : m_providers) { config.push_back(p->makeConfig()); }
        return config;
    }

  private:
    void lookupProviderRPC(const tl::request& req, const std::string& spec,
                           double timeout) {
        double  t1 = tl::timer::wtime();
        RequestResult<ProviderDescriptor> result;
        tl::auto_respond<decltype(result)> auto_respond_with{req, result};
        std::unique_lock<tl::mutex> lock(m_providers_mtx);
        auto it = resolveSpec(spec);
        if (it == m_providers.end() && timeout > 0) {
            m_providers_cv.wait(lock, [this, &spec, t1, timeout, &it]() {
                // FIXME doesn't wake up when timeout passes
                double t2 = tl::timer::wtime();
                it = resolveSpec(spec);
                return (t2 - t1 > timeout) || (it != m_providers.end());
            });
        }
        if (it != m_providers.end()) {
            auto& provider = *it;
            result.value().name = provider->getName();
            result.value().provider_id = provider->getProviderID();
        } else {
            result.error()
                = "Could not find provider with spec \""s + spec + "\"";
        }
    }

    void loadModuleRPC(const tl::request& req,
                       const std::string& path) {
        RequestResult<bool> result;
        tl::auto_respond<decltype(result)> auto_respond_with{req, result};
        try {
            ModuleManager::loadModule(path);
            result.success() = true;
            result.value()   = true;
        } catch (const Exception& e) {
            result.success() = false;
            result.error()   = e.what();
        }
    }

    void startProviderRPC(const tl::request& req, const std::string& description) {
        RequestResult<uint16_t> result;
        tl::auto_respond<decltype(result)> auto_respond_with{req, result};
        auto manager = ProviderManager(shared_from_this());
        try {
            auto c = json::parse(description);
            result.value() = manager.addProviderFromJSON(c)->getProviderID();
        } catch (std::exception& ex) {
            result.success() = false;
            result.error()   = ex.what();
        }
    }

    void migrateProviderRPC(const tl::request& req,
                            const std::string& name,
                            const std::string& dest_addr,
                            uint16_t dest_provider_id,
                            const std::string& config,
                            bool remove_source) {
        RequestResult<bool> result;
        tl::auto_respond<decltype(result)> auto_respond_with{req, result};
        auto manager = ProviderManager(shared_from_this());
        try {
            manager.migrateProvider(
                name, dest_addr, dest_provider_id, config, remove_source);
        } catch (Exception& ex) {
            result.success() = false;
            result.error() = ex.what();
        }
    }

    void snapshotProviderRPC(const tl::request& req,
                             const std::string& name,
                             const std::string& dest_path,
                             const std::string& config,
                             bool remove_source) {
        RequestResult<bool> result;
        tl::auto_respond<decltype(result)> auto_respond_with{req, result};
        auto manager = ProviderManager(shared_from_this());
        try {
            manager.snapshotProvider(
                name, dest_path, config, remove_source);
        } catch (Exception& ex) {
            result.success() = false;
            result.error() = ex.what();
        }
    }

    void restoreProviderRPC(const tl::request& req,
                            const std::string& name,
                            const std::string& src_path,
                            const std::string& config) {
        RequestResult<bool> result;
        tl::auto_respond<decltype(result)> auto_respond_with{req, result};
        auto manager = ProviderManager(shared_from_this());
        try {
            manager.restoreProvider(name, src_path, config);
        } catch (Exception& ex) {
            result.success() = false;
            result.error() = ex.what();
        }
    }
};

} // namespace bedrock

#endif
