/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_PROVIDER_IMPL_H
#define __BEDROCK_PROVIDER_IMPL_H

#include "MargoContextImpl.hpp"
#include "bedrock/ProviderWrapper.hpp"

#include <thallium.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace bedrock {

using namespace std::string_literals;
namespace tl = thallium;

class ProviderManagerImpl {

  public:
    std::vector<ProviderWrapper> m_providers;
    tl::mutex                    m_providers_mtx;
    tl::condition_variable       m_providers_cv;

    std::shared_ptr<MargoContextImpl> m_margo_context;
};

} // namespace bedrock

#endif
