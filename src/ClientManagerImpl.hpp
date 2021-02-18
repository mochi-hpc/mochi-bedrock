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
#include "bedrock/ClientWrapper.hpp"
#include "bedrock/ClientManager.hpp"

#include <thallium.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>

namespace bedrock {

using nlohmann::json;
using namespace std::string_literals;
namespace tl = thallium;

class ClientEntry : public ClientWrapper {
  public:
    std::shared_ptr<MargoManagerImpl> margo_ctx;
    ResolvedDependencyMap             dependencies;

    json makeConfig() const {
        auto c            = json::object();
        c["name"]         = name;
        c["type"]         = type;
        c["config"]       = json::parse(factory->getClientConfig(handle));
        c["dependencies"] = json::object();
        auto& d           = c["dependencies"];
        for (auto& p : dependencies) {
            auto& dep_name  = p.first;
            auto& dep_array = p.second;
            if (dep_array.size() == 0) continue;
            if (!(dep_array[0].flags & BEDROCK_ARRAY)) {
                d[dep_name] = dep_array[0].spec;
            } else {
                d[dep_name] = json::array();
                for (auto& x : dep_array) { d[dep_name].push_back(x.spec); }
            }
        }
        return c;
    }
};

class ClientManagerImpl
: public tl::provider<ClientManagerImpl>,
  public std::enable_shared_from_this<ClientManagerImpl> {

  public:
    std::vector<ClientEntry>       m_clients;
    mutable tl::mutex              m_clients_mtx;
    mutable tl::condition_variable m_clients_cv;

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
            [&spec](const ClientWrapper& item) { return item.name == spec; });
        return it;
    }

    json makeConfig() const {
        auto                       config = json::array();
        std::lock_guard<tl::mutex> lock(m_clients_mtx);
        for (auto& p : m_clients) { config.push_back(p.makeConfig()); }
        return config;
    }

  private:
    void lookupClientRPC(const tl::request& req, const std::string& name,
                         double timeout) {
        auto          manager = ClientManager(shared_from_this());
        double        t1      = tl::timer::wtime();
        ClientWrapper wrapper;
        RequestResult<ClientDescriptor> result;
        bool found = manager.lookupClient(name, &wrapper);
        if (!found && timeout > 0) {
            std::unique_lock<tl::mutex> lock(m_clients_mtx);
            m_clients_cv.wait(lock, [this, &name, t1, timeout]() {
                double t2 = tl::timer::wtime();
                return (t2 - t1 > timeout)
                    || (resolveSpec(name) != m_clients.end());
            });
            found = manager.lookupClient(name, &wrapper);
        }
        if (found) {
            result.value() = wrapper;
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
