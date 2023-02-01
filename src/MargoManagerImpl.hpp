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
#include "NamedDependency.hpp"

namespace tl = thallium;

namespace bedrock {

using nlohmann::json;

class PoolRef : public NamedDependency {

    std::string pool_name;

    public:

    PoolRef(std::string name)
    : pool_name(std::move(name)) {}

    const std::string& getName() const override {
        return pool_name;
    }
};

class MargoManagerImpl {

  public:
    tl::mutex         m_mtx;
    margo_instance_id m_mid;
    tl::engine        m_engine;

    // to keep track of who is using which pool,
    // we keep a shared_ptr to a PoolRef with just
    // the name of the pool.
    std::vector<std::shared_ptr<PoolRef>> m_pools;

    json makeConfig() const {
        char* str    = margo_get_config(m_mid);
        auto  config = json::parse(str);
        free(str);
        return config;
    }
};

} // namespace bedrock

#endif
