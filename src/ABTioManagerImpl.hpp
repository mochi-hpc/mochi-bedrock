/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_ABTIO_CONTEXT_IMPL_H
#define __BEDROCK_ABTIO_CONTEXT_IMPL_H

#include "bedrock/MargoManager.hpp"
#include "bedrock/ABTioManager.hpp"
#include "MargoManagerImpl.hpp"
#include <nlohmann/json.hpp>
#ifdef ENABLE_ABT_IO
#include <abt-io.h>
#endif
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace bedrock {

using nlohmann::json;

class ABTioEntry {
  public:
#ifdef ENABLE_ABT_IO
    std::string                       name;
    ABT_pool                          pool = ABT_POOL_NULL;
    abt_io_instance_id                abt_io_id = 0;
    std::shared_ptr<MargoManagerImpl> margo_ctx;
#endif

    json makeConfig() const {
        json config      = json::object();
#ifdef ENABLE_ABT_IO
        config["name"]   = name;
        config["pool"]   = MargoManager(margo_ctx).getPool(pool).name;
        auto c           = abt_io_get_config(abt_io_id);
        config["config"] = c ? json::parse(c) : json::object();
        free(c);
#endif
        return config;
    }

    ABTioEntry() = default;

    ABTioEntry(const ABTioEntry&) = delete;

    ABTioEntry(ABTioEntry&& other)
    : name(std::move(other.name))
    , pool(other.pool)
    , abt_io_id(other.abt_io_id)
    , margo_ctx(std::move(other.margo_ctx))
    {
        other.abt_io_id = 0;
    }

    ~ABTioEntry() {
#ifdef ENABLE_ABT_IO
        if(!abt_io_id)
            return;
        spdlog::trace("Freeing ABT-IO instance {}", name);
        abt_io_finalize(abt_io_id);
#endif
    }
};

class ABTioManagerImpl {

  public:
    std::shared_ptr<MargoManagerImpl> m_margo_manager;
    std::vector<ABTioEntry>           m_instances;

    json makeConfig() const {
        json config = json::array();
        for (auto& i : m_instances) { config.push_back(i.makeConfig()); }
        return config;
    }
};

} // namespace bedrock

#endif
