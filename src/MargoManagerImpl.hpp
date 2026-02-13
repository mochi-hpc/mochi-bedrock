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
#include <vector>
#include <atomic>

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
    tl::mutex                  m_mtx;
    std::vector<tl::engine>    m_engines;
    std::atomic<bool>          m_shutting_down{false};

    json makeConfig() const {
        if (m_engines.size() == 1) {
            auto mid = m_engines[0].get_margo_instance();
            char* str    = margo_get_config_opt(mid, MARGO_CONFIG_USE_NAMES);
            auto  config = json::parse(str);
            free(str);
            return config;
        }
        auto configs = json::array();
        for (auto& engine : m_engines) {
            auto mid = engine.get_margo_instance();
            char* str    = margo_get_config_opt(mid, MARGO_CONFIG_USE_NAMES);
            auto  config = json::parse(str);
            free(str);
            configs.push_back(std::move(config));
        }
        return configs;
    }
};

} // namespace bedrock

#endif
