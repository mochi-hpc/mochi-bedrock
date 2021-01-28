/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_ABTIO_CONTEXT_IMPL_H
#define __BEDROCK_ABTIO_CONTEXT_IMPL_H

#include "bedrock/MargoManager.hpp"
#include "MargoManagerImpl.hpp"
#include <nlohmann/json.hpp>
#include <abt-io.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace bedrock {

using nlohmann::json;

class ABTioEntry {
  public:
    std::string                       name;
    ABT_pool                          pool;
    abt_io_instance_id                abt_io_id;
    std::shared_ptr<MargoManagerImpl> margo_ctx;

    json makeConfig() const {
        json config      = json::object();
        config["name"]   = name;
        config["pool"]   = MargoManager(margo_ctx).getPoolInfo(pool).first;
        auto c           = abt_io_get_config(abt_io_id);
        config["config"] = c ? json::parse(c) : json::object();
        free(c);
        return config;
    }
};

class ABTioManagerImpl {

  public:
    std::shared_ptr<MargoManagerImpl> m_margo_context;
    std::vector<ABTioEntry>           m_instances;

    json makeConfig() const {
        json config = json::array();
        for (auto& i : m_instances) { config.push_back(i.makeConfig()); }
        return config;
    }
};

} // namespace bedrock

#endif
