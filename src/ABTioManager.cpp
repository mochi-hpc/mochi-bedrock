/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/ABTioManager.hpp"
#include "bedrock/MargoManager.hpp"
#include "bedrock/Jx9Manager.hpp"
#include "JsonUtil.hpp"
#include "Exception.hpp"
#include "ABTioManagerImpl.hpp"
#include <margo.h>

namespace tl = thallium;

namespace bedrock {

using nlohmann::json;

ABTioManager::ABTioManager(const MargoManager& margoCtx,
                           const Jx9Manager& jx9,
                           const json& config)
: self(std::make_shared<ABTioManagerImpl>()) {
    self->m_margo_manager = margoCtx;
    self->m_jx9_manager   = jx9;
    if (config.is_null()) return;
#ifndef ENABLE_ABT_IO
    if (!(config.is_array() && config.empty()))
        throw DETAILED_EXCEPTION(
            "Configuration has an \"abt_io\" field but Bedrock wasn't compiled with ABT-IO support");
#else
    if (!config.is_array()) {
        throw DETAILED_EXCEPTION("\"abt_io\" field in configuration should be an array");
    }
    for(auto& description : config) {
        addABTioInstanceFromJSON(description);
    }
#endif
}

// LCOV_EXCL_START

ABTioManager::ABTioManager(const ABTioManager&) = default;

ABTioManager::ABTioManager(ABTioManager&&) = default;

ABTioManager& ABTioManager::operator=(const ABTioManager&) = default;

ABTioManager& ABTioManager::operator=(ABTioManager&&) = default;

ABTioManager::~ABTioManager() = default;

ABTioManager::operator bool() const { return static_cast<bool>(self); }

// LCOV_EXCL_STOP

std::shared_ptr<NamedDependency>
ABTioManager::getABTioInstance(const std::string& name) const {
#ifndef ENABLE_ABT_IO
    (void)name;
    throw DETAILED_EXCEPTION("Bedrock was not compiler with ABT-IO support");
#else
    auto it = std::find_if(self->m_instances.begin(), self->m_instances.end(),
                           [&name](const auto& p) { return p->getName() == name; });
    if (it == self->m_instances.end())
        throw DETAILED_EXCEPTION("Could not find ABT-IO instance \"{}\"", name);
    return *it;
#endif
}

std::shared_ptr<NamedDependency>
ABTioManager::getABTioInstance(size_t index) const {
#ifndef ENABLE_ABT_IO
    (void)index;
    throw DETAILED_EXCEPTION("Bedrock was not compiler with ABT-IO support");
#else
    if (index >= self->m_instances.size())
        throw DETAILED_EXCEPTION("Could not find ABT-IO instance at index {}", index);
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
ABTioManager::addABTioInstance(const std::string&                      name,
                               const std::shared_ptr<NamedDependency>& pool,
                               const json&                             config) {
#ifndef ENABLE_ABT_IO
    (void)name;
    (void)pool;
    (void)config;
    throw DETAILED_EXCEPTION("Bedrock was not compiled with ABT-IO support");
#else
    json     abt_io_config;
    // check if the name doesn't already exist
    auto it = std::find_if(
        self->m_instances.begin(), self->m_instances.end(),
        [&name](const auto& instance) { return instance->getName() == name; });
    if (it != self->m_instances.end()) {
        throw DETAILED_EXCEPTION(
            "ABT-IO instance name \"{}\" already used",
            name);
    }
    // get the config of this ABT-IO instance
    if (!config.is_object() && !config.is_null()) {
        throw DETAILED_EXCEPTION(
            "\"config\" field in ABT-IO instance configuration should be an object");
    }
    // all good, can instanciate
    abt_io_init_info abt_io_info;
    auto configStr            = config.is_null() ? std::string{"{}"} : config.dump();
    abt_io_info.json_config   = configStr.c_str();
    abt_io_info.progress_pool = pool->getHandle<ABT_pool>();
    abt_io_instance_id abt_io = abt_io_init_ext(&abt_io_info);
    if (abt_io == ABT_IO_INSTANCE_NULL) {
        throw DETAILED_EXCEPTION("Could not initialize ABT-IO instance \"{}\"", name);
    }
    auto entry = std::make_shared<ABTioEntry>(name, abt_io, pool);
    self->m_instances.push_back(entry);
    return entry;
#endif
}

std::shared_ptr<NamedDependency>
ABTioManager::addABTioInstanceFromJSON(const json& description) {
    static constexpr const char* configSchema = R"(
    {
        "$schema": "https://json-schema.org/draft/2019-09/schema",
        "type": "object",
        "properties": {
            "name": {"type": "string", "pattern": "^[a-zA-Z_][a-zA-Z0-9_]*$" },
            "pool": {"oneOf": [
                {"type": "string", "pattern": "^[a-zA-Z_][a-zA-Z0-9_]*$" },
                {"type": "integer", "minimum": 0 }
            ]},
            "config": {"type": "object"}
        },
        "required": ["name"]
    }
    )";
    static const JsonValidator validator{configSchema};
    validator.validate(description, "ABTioManager");
    auto name = description["name"].get<std::string>();
    auto config = description.value("config", json::object());
    std::shared_ptr<NamedDependency> pool;
    // find pool
    if(description.contains("pool") && description["pool"].is_number()) {
        pool = MargoManager(self->m_margo_manager)
                .getPool(description["pool"].get<uint32_t>());
    } else {
        pool = MargoManager(self->m_margo_manager)
                .getPool(description.value("pool", "__primary__"));
    }
    return addABTioInstance(name, pool, config);
}

json ABTioManager::getCurrentConfig() const {
#ifndef ENABLE_ABT_IO
    return json::array();
#else
    return self->makeConfig();
#endif
}

} // namespace bedrock
