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
#include <spdlog/spdlog.h>
#ifdef ENABLE_MONA
#include <mona.h>
#endif
#include <string>
#include <vector>
#include <unordered_map>

namespace bedrock {

using nlohmann::json;

class MonaEntry : public NamedDependency {
  public:
    std::string                       name;
#ifdef ENABLE_MONA
    ABT_pool                          pool = ABT_POOL_NULL;
    mona_instance_t                   mona = nullptr;
    std::shared_ptr<MargoManagerImpl> margo_ctx;

    MonaEntry(const MonaEntry&) = delete;

    MonaEntry(MonaEntry&& other)
    : name(std::move(other.name))
    , pool(other.pool)
    , mona(other.mona)
    , margo_ctx(std::move(other.margo_ctx)) {
        other.mona = nullptr;
    }
#endif

    MonaEntry() = default;

    const std::string& getName() const {
        return name;
    }

    json makeConfig() const {
        json config      = json::object();
#ifdef ENABLE_MONA
        config["name"]   = name;
        config["pool"]   = MargoManager(margo_ctx).getPool(pool).name;
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

#ifdef ENABLE_MONA
    ~MonaEntry() {
        if(mona) {
            spdlog::trace("Finalizing MoNA instance {}", name);
            mona_finalize(mona);
            spdlog::trace("Done finalizing MoNA instance {}", name);
        }
    }
#endif
};

class MonaManagerImpl {

    friend class MonaManager;

  public:
    std::shared_ptr<MargoManagerImpl>       m_margo_manager;
    std::vector<std::shared_ptr<MonaEntry>> m_instances;

    json makeConfig() const {
        json config = json::array();
        for (auto& i : m_instances) { config.push_back(i->makeConfig()); }
        return config;
    }

    ~MonaManagerImpl() {
        spdlog::trace("Finalizing MoNA");
    }
};

} // namespace bedrock

#endif
