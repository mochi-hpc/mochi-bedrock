/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_MARGO_CONTEXT_IMPL_H
#define __BEDROCK_MARGO_CONTEXT_IMPL_H

#include <nlohmann/json.hpp>
#include <margo.h>
#include <thallium.hpp>
#include "bedrock/NamedDependency.hpp"
#include "Formatting.hpp"

namespace tl = thallium;

namespace bedrock {

using nlohmann::json;

struct PoolRef : public NamedDependency {

    PoolRef(std::string name, ABT_pool pool)
    : NamedDependency(
        std::move(name),
        "pool", pool,
        std::function<void(void*)>()) {}

};

struct XstreamRef : public NamedDependency {

    XstreamRef(std::string name, ABT_xstream xstream)
    : NamedDependency(
        std::move(name),
        "xstream", xstream,
        std::function<void(void*)>()) {}

};

class MargoManagerImpl {

  public:
    tl::mutex         m_mtx;
    margo_instance_id m_mid;
    tl::engine        m_engine;

    // to keep track of who is using which pool and xstream,
    // we keep a shared_ptr to a PoolRef/XstreamRef with just
    // the name of the pool/xstream and its handle.
    std::vector<std::shared_ptr<XstreamRef>> m_xstreams;
    std::vector<std::shared_ptr<PoolRef>> m_pools;

    json makeConfig() const {
        char* str    = margo_get_config_opt(m_mid, MARGO_CONFIG_USE_NAMES);
        auto  config = json::parse(str);
        free(str);
        return config;
    }
};

} // namespace bedrock

#endif
