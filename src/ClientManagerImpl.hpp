/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_CLIENT_MANAGER_IMPL_H
#define __BEDROCK_CLIENT_MANAGER_IMPL_H

#include "MargoManagerImpl.hpp"
#include "bedrock/RequestResult.hpp"
#include "bedrock/AbstractServiceFactory.hpp"
#include "bedrock/ClientDescriptor.hpp"
#include "bedrock/ClientManager.hpp"
#include "DependencyFinderImpl.hpp"

#include <thallium.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>

namespace bedrock {

using nlohmann::json;
using namespace std::string_literals;
namespace tl = thallium;

class ClientEntry : public NamedDependency {

    public:

    AbstractServiceFactory*  factory;
    ResolvedDependencyMap    dependencies;
    std::vector<std::string> tags;

    ClientEntry(std::string name, std::string type,
                void* handle, AbstractServiceFactory* f,
                ResolvedDependencyMap deps,
                std::vector<std::string> _tags)
    : NamedDependency(
        std::move(name),
        std::move(type),
        handle,
        [f](void* args) {
            if(f) f->finalizeClient(args);
        })
    , factory(f)
    , dependencies(deps)
    , tags(std::move(_tags)) {}

    json makeConfig() const {
        auto c            = json::object();
        c["name"]         = getName();
        c["type"]         = getType();
        c["config"]       = json::parse(factory->getClientConfig(getHandle<void*>()));
        c["tags"]         = json::array();
        for(auto& t : tags) c["tags"].push_back(t);
        c["dependencies"] = json::object();
        auto& d           = c["dependencies"];
        for (auto& p : dependencies) {
            auto& dep_name  = p.first;
            auto& dep_group = p.second;
            if (dep_group.dependencies.size() == 0) continue;
            if (!dep_group.is_array) {
                d[dep_name] = dep_group.dependencies[0]->getName();
            } else {
                d[dep_name] = json::array();
                for (auto& x : dep_group.dependencies) {
                    d[dep_name].push_back(x->getName());
                }
            }
        }
        return c;
    }
};

class ClientManagerImpl
: public tl::provider<ClientManagerImpl>,
  public std::enable_shared_from_this<ClientManagerImpl> {

  public:
    std::shared_ptr<DependencyFinderImpl>     m_dependency_finder;
    std::vector<std::shared_ptr<ClientEntry>> m_clients;
    mutable tl::mutex                         m_clients_mtx;
    mutable tl::condition_variable            m_clients_cv;

    std::shared_ptr<MargoManagerImpl> m_margo_context;

    tl::remote_procedure m_lookup_client;
    tl::remote_procedure m_list_clients;

    ClientManagerImpl(const tl::engine& engine, uint16_t provider_id,
                      const tl::pool& pool)
    : tl::provider<ClientManagerImpl>(engine, provider_id),
      m_lookup_client(define("bedrock_lookup_client",
                             &ClientManagerImpl::lookupClientRPC, pool)),
      m_list_clients(define("bedrock_list_clients",
                            &ClientManagerImpl::listClientsRPC, pool)) {}

    auto resolveSpec(const std::string& spec) {
        auto it = std::find_if(
            m_clients.begin(), m_clients.end(),
            [&spec](const std::shared_ptr<ClientEntry>& item) { return item->getName() == spec; });
        return it;
    }

    json makeConfig() const {
        auto                       config = json::array();
        std::lock_guard<tl::mutex> lock(m_clients_mtx);
        for (auto& p : m_clients) { config.push_back(p->makeConfig()); }
        return config;
    }

  private:
    void lookupClientRPC(const tl::request& req, const std::string& name,
                         double timeout) {
        double        t1      = tl::timer::wtime();
        RequestResult<ClientDescriptor> result;
        std::unique_lock<tl::mutex> lock(m_clients_mtx);
        auto it = resolveSpec(name);
        if (it == m_clients.end() && timeout > 0) {
            m_clients_cv.wait(lock, [this, &name, &it, t1, timeout]() {
                // FIXME will not actually wake up after timeout
                double t2 = tl::timer::wtime();
                it = resolveSpec(name);
                return (t2 - t1 > timeout) || (it != m_clients.end());
            });
        }
        if (it != m_clients.end()) {
            result.value().name = (*it)->getName();
            result.value().type = (*it)->getType();
        } else {
            result.error()
                = "Could not find client with name \""s + name + "\"";
        }
        req.respond(result);
    }

    void listClientsRPC(const tl::request& req) {
        auto manager = ClientManager(shared_from_this());
        RequestResult<std::vector<ClientDescriptor>> result;
        result.value() = manager.listClients();
        req.respond(result);
    }
};

} // namespace bedrock

#endif
