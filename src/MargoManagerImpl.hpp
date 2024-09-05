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

    tl::engine engine;

    PoolRef(tl::engine e, std::string name, tl::pool pool)
    : NamedDependency(std::move(name), "pool", std::move(pool))
    , engine{std::move(e)} {
        engine.pools().ref_incr(getHandle<tl::pool>());
    }

    PoolRef(const PoolRef&) = delete;
    PoolRef(PoolRef&&) = delete;

    ~PoolRef() {
        engine.pools().release(getHandle<tl::pool>());
    }
};

struct XstreamRef : public NamedDependency {

    tl::engine engine;

    XstreamRef(tl::engine e, std::string name, tl::xstream es)
    : NamedDependency(std::move(name), "xstream", std::move(es))
    , engine{std::move(e)} {
        engine.xstreams().ref_incr(getHandle<tl::xstream>());
    }

    XstreamRef(const XstreamRef&) = delete;
    XstreamRef(XstreamRef&&) = delete;

    ~XstreamRef() {
        engine.xstreams().release(getHandle<tl::xstream>());
    }
};

class MargoManagerImpl {

  public:
    tl::mutex  m_mtx;
    tl::engine m_engine;

    json makeConfig() const {
        auto mid = m_engine.get_margo_instance();
        char* str    = margo_get_config_opt(mid, MARGO_CONFIG_USE_NAMES);
        auto  config = json::parse(str);
        free(str);
        return config;
    }
};

} // namespace bedrock

#endif
