/*
 * (C) 2022 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/MonaManager.hpp"
#include "bedrock/MargoManager.hpp"
#include "Exception.hpp"
#include "MonaManagerImpl.hpp"
#include "JsonUtil.hpp"
#include <margo.h>

namespace tl = thallium;

namespace bedrock {

using nlohmann::json;

MonaManager::MonaManager(const MargoManager& margoCtx,
                         const Jx9Manager& jx9,
                         const json& config,
                         const std::string& defaultAddress)
: self(std::make_shared<MonaManagerImpl>()) {
    self->m_default_address = defaultAddress;
    self->m_margo_manager   = margoCtx;
    self->m_jx9_manager     = jx9;
    if (config.is_null()) return;
#ifndef ENABLE_MONA
    (void)defaultAddress;
    if (!(config.is_array() && config.empty()))
        throw DETAILED_EXCEPTION(
            "Configuration has \"mona\" entry but Bedrock wasn't compiled with MoNA support");
#else
    if (!config.is_array()) {
        throw DETAILED_EXCEPTION("\"mona\" field should be an array");
    }
    for(auto& description : config) {
        addMonaInstanceFromJSON(description);
    }
#endif
}

// LCOV_EXCL_START

MonaManager::MonaManager(const MonaManager&) = default;

MonaManager::MonaManager(MonaManager&&) = default;

MonaManager& MonaManager::operator=(const MonaManager&) = default;

MonaManager& MonaManager::operator=(MonaManager&&) = default;

MonaManager::~MonaManager() = default;

MonaManager::operator bool() const { return static_cast<bool>(self); }

// LCOV_EXCL_STOP

std::shared_ptr<NamedDependency>
MonaManager::getMonaInstance(const std::string& name) const {
#ifndef ENABLE_MONA
    (void)name;
    throw DETAILED_EXCEPTION("Bedrock was not compiled with MoNA support");
#else
    auto guard = std::unique_lock<tl::mutex>{self->m_mtx};
    auto it = std::find_if(self->m_instances.begin(), self->m_instances.end(),
                           [&name](const auto& p) { return p->getName() == name; });
    if (it == self->m_instances.end())
        throw DETAILED_EXCEPTION("Could not find MoNA instance named \"{}\"", name);
    return *it;
#endif
}

std::shared_ptr<NamedDependency> MonaManager::getMonaInstance(size_t index) const {
#ifndef ENABLE_MONA
    (void)index;
    throw DETAILED_EXCEPTION("Bedrock was not compiled with MoNA support");
#else
    auto guard = std::unique_lock<tl::mutex>{self->m_mtx};
    if (index >= self->m_instances.size())
        throw DETAILED_EXCEPTION("Could not find MoNA instance at index {}", index);
    return self->m_instances[index];
#endif
}


size_t MonaManager::numMonaInstances() const {
#ifndef ENABLE_MONA
    return 0;
#else
    auto guard = std::unique_lock<tl::mutex>{self->m_mtx};
    return self->m_instances.size();
#endif
}

std::shared_ptr<NamedDependency>
MonaManager::addMonaInstance(const std::string& name,
                             std::shared_ptr<NamedDependency> pool,
                             const std::string& address) {
#ifndef ENABLE_MONA
    (void)name;
    (void)pool;
    (void)address;
    throw DETAILED_EXCEPTION("Bedrock wasn't compiled with MoNA support");
#else
    auto guard = std::unique_lock<tl::mutex>{self->m_mtx};
    // check if the name doesn't already exist
    auto it = std::find_if(
        self->m_instances.begin(), self->m_instances.end(),
        [&name](const auto& instance) { return instance->getName() == name; });
    if (it != self->m_instances.end()) {
        throw DETAILED_EXCEPTION("Mona instance name \"{}\" already used", name);
    }
    // all good, can instanciate
    mona_instance_t mona = mona_init_pool(
        address.c_str(), true, nullptr, pool->getHandle<ABT_pool>());

    if (!mona) {
        throw DETAILED_EXCEPTION("Could not initialize mona instance");
    }
    auto entry = std::make_shared<MonaEntry>(name, mona, pool);
    self->m_instances.push_back(entry);
    return entry;
#endif
}

std::shared_ptr<NamedDependency>
MonaManager::addMonaInstanceFromJSON(const json& description) {
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
            "address": {"type": "string"}
        },
        "required": ["name"]
    }
    )";
    static const JsonValidator validator{configSchema};
    validator.validate(description, "MonaManager");
    auto name = description["name"].get<std::string>();
    auto address = description.value("address", self->m_default_address);
    std::shared_ptr<NamedDependency> pool;
    // find pool
    if(description.contains("pool") && description["pool"].is_number()) {
        pool = MargoManager(self->m_margo_manager)
            .getPool(description["pool"].get<uint32_t>());
    } else {
        pool = MargoManager(self->m_margo_manager)
            .getPool(description.value("pool", "__primary__"));
    }
    return addMonaInstance(name, pool, address);
}

json MonaManager::getCurrentConfig() const {
#ifndef ENABLE_MONA
    return json::array();
#else
    auto guard = std::unique_lock<tl::mutex>{self->m_mtx};
    return self->makeConfig();
#endif
}

} // namespace bedrock
