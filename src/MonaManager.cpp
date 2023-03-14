/*
 * (C) 2022 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/MonaManager.hpp"
#include "bedrock/MargoManager.hpp"
#include "Exception.hpp"
#include "MonaManagerImpl.hpp"
#include <margo.h>

namespace tl = thallium;

namespace bedrock {

using nlohmann::json;

MonaManager::MonaManager(const MargoManager& margoCtx,
                         const std::string& configString,
                         const std::string& defaultAddress)
: self(std::make_shared<MonaManagerImpl>()) {
    self->m_margo_manager = margoCtx;
    auto config           = json::parse(configString);
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
    std::vector<std::string>                      names;
    std::vector<std::shared_ptr<NamedDependency>> pools;
    std::vector<std::string>                      addresses;
    int                                           i = 0;
    for (auto& mona_config : config) {
        if (!mona_config.is_object()) {
            throw DETAILED_EXCEPTION("MoNA instance should be of an object");
        }
        std::string name;       // mona instance name
        int         pool_index; // pool index
        std::shared_ptr<NamedDependency> pool;
        // find the name of the instance or use a default name
        auto name_json_it = mona_config.find("name");
        if (name_json_it == mona_config.end()) {
            name = "__mona_";
            name += std::to_string(i) + "__";
        } else if (not name_json_it->is_string()) {
            throw DETAILED_EXCEPTION("MoNA instance name should be a string");
        } else {
            name = name_json_it->get<std::string>();
        }
        // check if the name doesn't already exist
        auto it = std::find(names.begin(), names.end(), name);
        if (it != names.end()) {
            throw DETAILED_EXCEPTION(
                "MoNA instance name \"{}\" already used", name);
        }
        names.push_back(name);
        // find the pool used by the instance
        auto pool_json_it = mona_config.find("pool");
        if (pool_json_it == mona_config.end()) {
            throw DETAILED_EXCEPTION(
                "\"pool\" field required in MoNA instance configuration");
        } else if (pool_json_it->is_string()) {
            auto pool_str = pool_json_it->get<std::string>();
            pool          = margoCtx.getPool(pool_str);
        } else if (pool_json_it->is_number_integer()) {
            pool_index = pool_json_it->get<int>();
            pool       = margoCtx.getPool(pool_index);
        } else {
            throw DETAILED_EXCEPTION(
                "\"pool\" field in MoNA instance configuration should be a string or "
                "an integer");
        }
        pools.push_back(pool);
        // get the address of this MoNA instance
        auto address_json_it = mona_config.find("address");
        if (address_json_it == mona_config.end()) {
            addresses.push_back(defaultAddress);
        } else if (!address_json_it->is_string()) {
            throw DETAILED_EXCEPTION(
                "\"address\" field in MoNA description should be a string");
        } else {
            addresses.push_back(address_json_it->get<std::string>());
        }
        i += 1;
    }
    // instantiate MoNA instances
    std::vector<std::shared_ptr<MonaEntry>> mona_entries;
    for (unsigned i = 0; i < names.size(); i++) {

        const auto& address = addresses[i];
        mona_instance_t mona = mona_init_pool(address.c_str(), true, nullptr, pools[i]->getHandle<ABT_pool>());

        if (!mona) {
            for (unsigned j = 0; j < mona_entries.size(); j++) {
                mona_finalize(mona_entries[j]->getHandle<mona_instance_t>());
            }
            throw DETAILED_EXCEPTION("Could not initialize mona instance \"{}\"", names[i]);
        }
        auto entry = std::make_shared<MonaEntry>(names[i], mona, pools[i]);
        mona_entries.push_back(std::move(entry));
        spdlog::trace("Added MoNA instance \"{}\"", names[i]);
    }
    // setup self
    self->m_instances = std::move(mona_entries);
#endif
}

MonaManager::MonaManager(const MonaManager&) = default;

MonaManager::MonaManager(MonaManager&&) = default;

MonaManager& MonaManager::operator=(const MonaManager&) = default;

MonaManager& MonaManager::operator=(MonaManager&&) = default;

MonaManager::~MonaManager() = default;

MonaManager::operator bool() const { return static_cast<bool>(self); }

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

std::shared_ptr<NamedDependency> MonaManager::getMonaInstance(int index) const {
#ifndef ENABLE_MONA
    (void)index;
    throw DETAILED_EXCEPTION("Bedrock was not compiled with MoNA support");
#else
    auto guard = std::unique_lock<tl::mutex>{self->m_mtx};
    if (index < 0 || index >= (int)self->m_instances.size())
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
                             const std::string& pool_name,
                             const std::string& address) {
#ifndef ENABLE_MONA
    (void)name;
    (void)pool_name;
    (void)address;
    throw DETAILED_EXCEPTION("Bedrock wasn't compiled with MoNA support");
#else
    auto guard = std::unique_lock<tl::mutex>{self->m_mtx};
    // check if the name doesn't already exist
    auto it = std::find_if(
        self->m_instances.begin(), self->m_instances.end(),
        [&name](const auto& instance) { return instance->getName() == name; });
    if (it != self->m_instances.end()) {
        throw DETAILED_EXCEPTION("Name \"{}\" already used by another MoNA instance");
    }
    // find pool
    auto pool = MargoManager(self->m_margo_manager).getPool(pool_name);
    if (!pool) {
        throw DETAILED_EXCEPTION("Could not find pool \"{}\" in MargoManager",
                        pool_name);
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

std::string MonaManager::getCurrentConfig() const {
#ifndef ENABLE_MONA
    return "[]";
#else
    auto guard = std::unique_lock<tl::mutex>{self->m_mtx};
    return self->makeConfig().dump();
#endif
}

} // namespace bedrock
