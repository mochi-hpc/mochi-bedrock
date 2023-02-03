/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/ABTioManager.hpp"
#include "bedrock/Exception.hpp"
#include "bedrock/MargoManager.hpp"
#include "ABTioManagerImpl.hpp"
#include <margo.h>

namespace tl = thallium;

namespace bedrock {

using nlohmann::json;

ABTioManager::ABTioManager(const MargoManager& margoCtx,
                           const std::string&  configString)
: self(std::make_shared<ABTioManagerImpl>()) {
    self->m_margo_manager = margoCtx;
    auto config           = json::parse(configString);
    if (config.is_null()) return;
#ifndef ENABLE_ABT_IO
    if (!(config.is_array() && config.empty()))
        throw Exception(
            "Configuration has \"abt_io\" entry but Bedrock wasn't compiled with ABT-IO support");
#else
    if (!config.is_array()) {
        throw Exception("\"abt_io\" entry should be an array type");
    }
    std::vector<std::string>                      names;
    std::vector<std::shared_ptr<NamedDependency>> pools;
    std::vector<std::string>                      configs;
    int                      i = 0;
    for (auto& abt_io_config : config) {
        if (!abt_io_config.is_object()) {
            throw Exception("ABT-IO descriptors in JSON should be object type");
        }
        std::string name;                      // abt_io instance name
        int         pool_index;                // pool index
        std::shared_ptr<NamedDependency> pool; // pool
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
            if (!pool) {
                throw Exception("Could not find pool \"{}\" in MargoManager",
                                pool_str);
            }
        } else if (pool_json_it->is_number_integer()) {
            pool_index = pool_json_it->get<int>();
            pool       = margoCtx.getPool(pool_index);
            if (!pool) {
                throw Exception("Could not find pool {} in MargoManager",
                                pool_index);
            }
        } else {
            throw Exception(
                "\"pool\" field in ABT-IO description should be string or "
                "integer");
        }
        // get the config of this ABT-IO instance
        auto config_json_it = abt_io_config.find("config");
        if (config_json_it == abt_io_config.end()) {
            configs.push_back("{}");
        } else if (!config_json_it->is_object()) {
            throw Exception(
                "\"config\" field in ABT-IO description should be an object");
        } else {
            configs.push_back(config_json_it->dump());
        }
        // put the pool and name in vectors
        names.push_back(name);
        pools.push_back(pool);
        i += 1;
    }
    // instantiate ABT-IO instances
    std::vector<std::shared_ptr<ABTioEntry>> abt_io_entries;
    for (unsigned i = 0; i < names.size(); i++) {
        abt_io_init_info abt_io_info;
        abt_io_info.json_config   = configs[i].c_str();
        abt_io_info.progress_pool = pools[i]->getHandle<ABT_pool>();
        abt_io_instance_id abt_io = abt_io_init_ext(&abt_io_info);
        spdlog::trace("Created ABT-IO instance {}", names[i]);
        if (abt_io == ABT_IO_INSTANCE_NULL) {
            abt_io_entries.clear();
            throw Exception("Could not initialized abt-io instance {}", i);
        }
        auto entry = std::make_shared<ABTioEntry>(names[i], abt_io, pools[i]);
        abt_io_entries.push_back(std::move(entry));
    }
    // setup self
    self->m_instances = std::move(abt_io_entries);
#endif
}

ABTioManager::ABTioManager(const ABTioManager&) = default;

ABTioManager::ABTioManager(ABTioManager&&) = default;

ABTioManager& ABTioManager::operator=(const ABTioManager&) = default;

ABTioManager& ABTioManager::operator=(ABTioManager&&) = default;

ABTioManager::~ABTioManager() = default;

ABTioManager::operator bool() const { return static_cast<bool>(self); }

std::shared_ptr<NamedDependency>
ABTioManager::getABTioInstance(const std::string& name) const {
#ifndef ENABLE_ABT_IO
    (void)name;
    return nullptr;
#else
    auto it = std::find_if(self->m_instances.begin(), self->m_instances.end(),
                           [&name](const auto& p) { return p->getName() == name; });
    if (it == self->m_instances.end())
        return nullptr;
    else
        return *it;
#endif
}

std::shared_ptr<NamedDependency>
ABTioManager::getABTioInstance(int index) const {
#ifndef ENABLE_ABT_IO
    (void)index;
    return nullptr;
#else
    if (index < 0 || index >= (int)self->m_instances.size())
        return nullptr;
    return self->m_instances[index];
#endif
}

size_t ABTioManager::numABTioInstances() const {
#ifndef ENABLE_ABT_IO
    return 0;
#else
    return self->m_instances.size();
#endif
}

std::shared_ptr<NamedDependency>
ABTioManager::addABTioInstance(const std::string& name,
                               const std::string& pool_name,
                               const std::string& config) {
#ifndef ENABLE_ABT_IO
    (void)name;
    (void)pool_name;
    (void)config;
    throw Exception("Bedrock wasn't compiled with ABT-IO support");
#else
    std::shared_ptr<NamedDependency> pool;
    json     abt_io_config;
    // parse configuration
    try {
        abt_io_config = json::parse(config);
    } catch (...) {
        throw Exception(
            "Could not parse ABT-IO JSON configuration "
            "for instance {}",
            name);
    }
    // check if the name doesn't already exist
    auto it = std::find_if(
        self->m_instances.begin(), self->m_instances.end(),
        [&name](const auto& instance) { return instance->getName() == name; });
    if (it != self->m_instances.end()) {
        throw Exception("Name \"{}\" already used by another ABT-IO instance");
    }
    // find pool
    pool = MargoManager(self->m_margo_manager).getPool(pool_name);
    if (!pool) {
        throw Exception("Could not find pool \"{}\" in MargoManager",
                        pool_name);
    }
    // get the config of this ABT-IO instance
    if (!abt_io_config.is_object()) {
        throw Exception(
            "\"config\" field in ABT-IO description should be an object");
    }
    // all good, can instanciate
    abt_io_init_info abt_io_info;
    abt_io_info.json_config   = config.c_str();
    abt_io_info.progress_pool = pool->getHandle<ABT_pool>();
    abt_io_instance_id abt_io = abt_io_init_ext(&abt_io_info);
    if (abt_io == ABT_IO_INSTANCE_NULL) {
        throw Exception("Could not initialized abt-io instance {}", name);
    }
    auto entry = std::make_shared<ABTioEntry>(name, abt_io, pool);
    self->m_instances.push_back(entry);
    return entry;
#endif
}

std::string ABTioManager::getCurrentConfig() const {
#ifndef ENABLE_ABT_IO
    return "[]";
#else
    return self->makeConfig().dump();
#endif
}

} // namespace bedrock
