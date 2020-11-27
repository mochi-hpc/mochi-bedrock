/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_PROVIDER_IMPL_H
#define __BEDROCK_PROVIDER_IMPL_H

#include "MargoContextImpl.hpp"
#include "bedrock/RequestResult.hpp"
#include "bedrock/AbstractServiceFactory.hpp"
#include "bedrock/ProviderWrapper.hpp"
#include "bedrock/ProviderManager.hpp"

#include <thallium.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>

namespace bedrock {

using nlohmann::json;
using namespace std::string_literals;
namespace tl = thallium;

class ProviderEntry : public ProviderWrapper {
  public:
    std::shared_ptr<MargoContextImpl> margo_ctx;
    ABT_pool                          pool;
    // TODO add tracking of dependencies

    json makeConfig() const {
        auto c            = json::object();
        c["name"]         = name;
        c["type"]         = type;
        c["provider_id"]  = provider_id;
        c["pool"]         = MargoContext(margo_ctx).getPoolInfo(pool).first;
        c["config"]       = json::parse(factory->getProviderConfig(handle));
        c["dependencies"] = json::object();
        /* TODO
        auto&d = c["dependencies"];
        for(auto& p : dependencies) {
            c[p.first] = p
        }
        */
        return c;
    }
};

class ProviderManagerImpl
: public tl::provider<ProviderManagerImpl>,
  public std::enable_shared_from_this<ProviderManagerImpl> {

  public:
    std::vector<ProviderEntry>     m_providers;
    mutable tl::mutex              m_providers_mtx;
    mutable tl::condition_variable m_providers_cv;

    std::shared_ptr<MargoContextImpl> m_margo_context;

    tl::remote_procedure m_lookup_provider;
    tl::remote_procedure m_list_providers;

    ProviderManagerImpl(const tl::engine& engine, uint16_t provider_id,
                        const tl::pool& pool)
    : tl::provider<ProviderManagerImpl>(engine, provider_id),
      m_lookup_provider(define("bedrock_lookup_provider",
                               &ProviderManagerImpl::lookupProviderRPC, pool)),
      m_list_providers(define("bedrock_list_providers",
                              &ProviderManagerImpl::listProvidersRPC, pool)) {}

    auto resolveSpec(const std::string& type, uint16_t provider_id) {
        return std::find_if(m_providers.begin(), m_providers.end(),
                            [&type, &provider_id](const ProviderWrapper& item) {
                                return item.type == type
                                    && item.provider_id == provider_id;
                            });
    }

    auto resolveSpec(const std::string& spec) {
        auto                          column = spec.find(':');
        decltype(m_providers.begin()) it;
        if (column == std::string::npos) {
            it = std::find_if(m_providers.begin(), m_providers.end(),
                              [&spec](const ProviderWrapper& item) {
                                  return item.name == spec;
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
        for (auto& p : m_providers) { config.push_back(p.makeConfig()); }
        return config;
    }

  private:
    void lookupProviderRPC(const tl::request& req, const std::string& spec,
                           double timeout) {
        auto            manager = ProviderManager(shared_from_this());
        double          t1      = tl::timer::wtime();
        ProviderWrapper wrapper;
        RequestResult<ProviderDescriptor> result;
        bool found = manager.lookupProvider(spec, &wrapper);
        if (!found && timeout > 0) {
            std::unique_lock<tl::mutex> lock(m_providers_mtx);
            m_providers_cv.wait(lock, [this, &spec, t1, timeout]() {
                double t2 = tl::timer::wtime();
                return (t2 - t1 > timeout)
                    || (resolveSpec(spec) != m_providers.end());
            });
            found = manager.lookupProvider(spec, &wrapper);
        }
        if (found) {
            result.value() = wrapper;
        } else {
            result.error()
                = "Could not find provider with spec \""s + spec + "\"";
        }
        req.respond(result);
    }

    void listProvidersRPC(const tl::request& req) {
        auto manager = ProviderManager(shared_from_this());
        RequestResult<std::vector<ProviderDescriptor>> result;
        result.value() = manager.listProviders();
        req.respond(result);
    }
};

} // namespace bedrock

#endif
