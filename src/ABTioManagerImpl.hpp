/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_ABTIO_CONTEXT_IMPL_H
#define __BEDROCK_ABTIO_CONTEXT_IMPL_H

#include "bedrock/MargoManager.hpp"
#include "bedrock/ABTioManager.hpp"
#include "bedrock/NamedDependency.hpp"
#include "MargoManagerImpl.hpp"
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

    std::shared_ptr<NamedDependency> pool;

    json makeConfig() const {
        json config      = json::object();
        config["name"]   = getName();
        config["pool"]   = pool->getName();
        auto c           = abt_io_get_config(getHandle<abt_io_instance_id>());
        config["config"] = c ? json::parse(c) : json::object();
        free(c);
        return config;
    }

    template<typename S>
    ABTioEntry(
        S&& name,
        abt_io_instance_id abt_io_id,
        std::shared_ptr<NamedDependency> p)
    : NamedDependency(
        std::forward<S>(name),
        "abt_io",
        abt_io_id,
        releaseABTioEntry)
    , pool(std::move(p))
    {}

    ABTioEntry(const ABTioEntry&) = delete;
    ABTioEntry(ABTioEntry&& other) = delete;

    static void releaseABTioEntry(void* args) {
        auto abt_io_id = static_cast<abt_io_instance_id>(args);
        if(!abt_io_id) return;
        abt_io_finalize(abt_io_id);
    }

    ~ABTioEntry() {
        spdlog::trace("Freeing ABT-IO instance {}", getName());
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
