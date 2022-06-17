/*
 * (C) 2022 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_MONA_CONTEXT_IMPL_H
#define __BEDROCK_MONA_CONTEXT_IMPL_H

#include "bedrock/MargoManager.hpp"
#include "bedrock/MonaManager.hpp"
#include "MargoManagerImpl.hpp"
#include <nlohmann/json.hpp>
#ifdef ENABLE_MONA
#include <mona.h>
#endif
#include <string>
#include <vector>
#include <unordered_map>

namespace bedrock {

using nlohmann::json;

class MonaEntry {
  public:
#ifdef ENABLE_MONA
    std::string                       name;
    ABT_pool                          pool;
    mona_instance_t                   mona;
    std::shared_ptr<MargoManagerImpl> margo_ctx;
#endif

    json makeConfig() const {
        json config      = json::object();
#ifdef ENABLE_MONA
        config["name"]   = name;
        config["pool"]   = MargoManager(margo_ctx).getPoolInfo(pool).first;
        na_addr_t self_addr;
        na_return_t ret = mona_addr_self(mona, &self_addr);
        if(ret != NA_SUCCESS) {
            config["address"] = nullptr;
        } else {
            char self_addr_str[256];
            size_t self_addr_size = 256;
            ret = mona_addr_to_string(mona, self_addr_str, &self_addr_size, self_addr);
            if(ret != NA_SUCCESS)
                config["address"] = nullptr;
            else
                config["address"] = std::string{self_addr_str};
        }
#endif
        return config;
    }
};

class MonaManagerImpl {

    friend class MonaManager;

  public:
    std::shared_ptr<MargoManagerImpl> m_margo_manager;
    std::vector<MonaEntry>            m_instances;

    json makeConfig() const {
        json config = json::array();
        for (auto& i : m_instances) { config.push_back(i.makeConfig()); }
        return config;
    }
};

} // namespace bedrock

#endif
