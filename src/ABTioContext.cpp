/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/ABTioContext.hpp"
#include "bedrock/Exception.hpp"
#include "bedrock/MargoContext.hpp"
#include "ABTioContextImpl.hpp"
#include <margo.h>

namespace tl = thallium;

namespace bedrock {

using nlohmann::json;

ABTioContext::ABTioContext(const MargoContext& margoCtx,
                           const std::string&  configString)
: self(std::make_shared<ABTioContextImpl>()) {
    self->m_margo_context = margoCtx;
    auto config = json::parse(configString);
    if (config.is_null()) return;
    if (!config.is_array()) {
        throw Exception("\"abt_io\" entry should be an array type");
    }
    std::vector<std::string> names;
    std::vector<ABT_pool>    pools;
    int                      i = 0;
    for (auto& abt_io_config : config) {
        if (!abt_io_config.is_object()) {
            throw Exception("ABT-IO descriptors in JSON should be object type");
        }
        std::string name;       // abt_io instance name
        int         pool_index; // pool index
        ABT_pool    pool;       // pool
        // find the name of the instance or use a default name
        auto name_json_it = abt_io_config.find("name");
        if (name_json_it == abt_io_config.end()) {
            name = "__abt_io_";
            name += std::to_string(i) + "__";
        } else if (not name_json_it->is_string()) {
            throw Exception("ABT-IO instance name should be a string");
        } else {
            name = name_json_it->get<std::string>();
        }
        // check if the name doesn't already exist
        auto it = std::find(names.begin(), names.end(), name);
        if (it != names.end()) {
            throw Exception(
                "Name \"{}\" used multiple times in ABT-IO configuration");
        }
        // find the pool used by the instance
        auto pool_json_it = abt_io_config.find("pool");
        if (pool_json_it == abt_io_config.end()) {
            throw Exception(
                "Could not find \"pool\" entry in ABT-IO descriptor");
        } else if (pool_json_it->is_string()) {
            auto pool_str = pool_json_it->get<std::string>();
            pool          = margoCtx.getPool(pool_str);
            if (pool == ABT_POOL_NULL) {
                throw Exception("Could not find pool \"{}\" in MargoContext",
                                pool_str);
            }
        } else if (pool_json_it->is_number_integer()) {
            pool_index = pool_json_it->get<int>();
            pool       = margoCtx.getPool(pool_index);
            if (pool == ABT_POOL_NULL) {
                throw Exception("Could not find pool {} in MargoContext",
                                pool_index);
            }
        } else {
            throw Exception(
                "\"pool\" field in ABT-IO description should be string or "
                "integer");
        }
        // put the pool and name in vectors
        names.push_back(name);
        pools.push_back(pool);
        i += 1;
    }
    // instantiate ABT-IO instances
    std::vector<ABTioEntry> abt_io_entries;
    for (unsigned i = 0; i < names.size(); i++) {
        abt_io_instance_id abt_io = abt_io_init_pool(pools[i]);
        if (abt_io == ABT_IO_INSTANCE_NULL) {
            for (unsigned j = 0; j < abt_io_entries.size(); j++) {
                abt_io_finalize(abt_io_entries[j].abt_io_id);
            }
            throw Exception("Could not initialized abt-io instance {}", i);
        }
        ABTioEntry entry;
        entry.name = names[i];
        entry.pool = pools[i];
        entry.abt_io_id = abt_io;
        entry.margo_ctx = self->m_margo_context;
        abt_io_entries.push_back(std::move(entry));
    }
    // setup self
    self->m_instances = std::move(abt_io_entries);
}

ABTioContext::ABTioContext(const ABTioContext&) = default;

ABTioContext::ABTioContext(ABTioContext&&) = default;

ABTioContext& ABTioContext::operator=(const ABTioContext&) = default;

ABTioContext& ABTioContext::operator=(ABTioContext&&) = default;

ABTioContext::~ABTioContext() = default;

ABTioContext::operator bool() const { return static_cast<bool>(self); }

abt_io_instance_id
ABTioContext::getABTioInstance(const std::string& name) const {
    auto it = std::find_if(self->m_instances.begin(), self->m_instances.end(),
                           [&name](const auto& p) { return p.name == name; });
    if (it == self->m_instances.end())
        return ABT_IO_INSTANCE_NULL;
    else
        return it->abt_io_id;
}

abt_io_instance_id ABTioContext::getABTioInstance(int index) const {
    if (index < 0 || index >= (int)self->m_instances.size())
        return ABT_IO_INSTANCE_NULL;
    return self->m_instances[index].abt_io_id;
}

const std::string& ABTioContext::getABTioInstanceName(int index) const {
    static const std::string empty = "";
    if (index < 0 || index >= (int)self->m_instances.size()) return empty;
    return self->m_instances[index].name;
}

int ABTioContext::getABTioInstanceIndex(const std::string& name) const {
    auto it = std::find_if(self->m_instances.begin(), self->m_instances.end(),
                           [&name](const auto& p) { return p.name == name; });
    if (it == self->m_instances.end())
        return -1;
    else
        return std::distance(self->m_instances.begin(), it);
}

size_t ABTioContext::numABTioInstances() const {
    return self->m_instances.size();
}

std::string ABTioContext::getCurrentConfig() const {
    return self->makeConfig().dump();
}

} // namespace bedrock
