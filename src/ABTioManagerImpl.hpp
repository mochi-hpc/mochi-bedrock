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
#include "NamedDependency.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <unordered_map>

#ifdef ENABLE_ABT_IO
#include <abt-io.h>
#else
typedef uint64_t abt_io_instance_id;
inline char* abt_io_get_config(abt_io_instance_id) { return nullptr; }
inline void abt_io_finalize(abt_io_instance_id) {}
#endif


namespace bedrock {

using nlohmann::json;

class ABTioEntry : public NamedDependency {

    public:

    std::string                       instance_name;
    ABT_pool                          pool = ABT_POOL_NULL;
    abt_io_instance_id                abt_io_id = 0;
    std::shared_ptr<MargoManagerImpl> margo_ctx;

    json makeConfig() const {
        json config      = json::object();
        config["name"]   = instance_name;
        config["pool"]   = MargoManager(margo_ctx).getPool(pool).name;
        auto c           = abt_io_get_config(abt_io_id);
        config["config"] = c ? json::parse(c) : json::object();
        free(c);
        return config;
    }

    ABTioEntry(std::string name)
    : instance_name(std::move(name)) {}

    ABTioEntry(const ABTioEntry&) = delete;

    ABTioEntry(ABTioEntry&& other)
    : instance_name(std::move(other.instance_name))
    , pool(other.pool)
    , abt_io_id(other.abt_io_id)
    , margo_ctx(std::move(other.margo_ctx))
    {
        other.abt_io_id = 0;
    }

    ~ABTioEntry() {
        if(!abt_io_id)
            return;
        spdlog::trace("Freeing ABT-IO instance {}", instance_name);
        abt_io_finalize(abt_io_id);
    }

    const std::string& getName() const override {
        return instance_name;
    }
};

class ABTioManagerImpl {

  public:
    std::shared_ptr<MargoManagerImpl>        m_margo_manager;
    std::vector<std::shared_ptr<ABTioEntry>> m_instances;

    json makeConfig() const {
        json config = json::array();
        for (auto& i : m_instances) { config.push_back(i->makeConfig()); }
        return config;
    }
};

} // namespace bedrock

#endif
