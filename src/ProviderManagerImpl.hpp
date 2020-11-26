/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_PROVIDER_IMPL_H
#define __BEDROCK_PROVIDER_IMPL_H

#include "MargoContextImpl.hpp"
#include "bedrock/RequestResult.hpp"
#include "bedrock/ProviderWrapper.hpp"
#include "bedrock/ProviderManager.hpp"

#include <thallium.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>

namespace bedrock {

using namespace std::string_literals;
namespace tl = thallium;

class ProviderManagerImpl
: public tl::provider<ProviderManagerImpl>,
  public std::enable_shared_from_this<ProviderManagerImpl> {

  public:
    std::vector<ProviderWrapper> m_providers;
    tl::mutex                    m_providers_mtx;
    tl::condition_variable       m_providers_cv;

    std::shared_ptr<MargoContextImpl> m_margo_context;

    tl::remote_procedure m_lookup_provider;
    tl::remote_procedure m_list_providers;

    template <typename... Args>
    ProviderManagerImpl(Args&&... args)
    : tl::provider<ProviderManagerImpl>(std::forward<Args>(args)...),
      m_lookup_provider(define("bedrock_lookup_provider",
                               &ProviderManagerImpl::lookupProviderRPC)),
      m_list_providers(define("bedrock_list_providers",
                              &ProviderManagerImpl::listProvidersRPC)) {}

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
