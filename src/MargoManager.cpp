/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <bedrock/MargoManager.hpp>
#include <bedrock/DetailedException.hpp>
#include "MargoManagerImpl.hpp"
#include "MargoLogging.hpp"
#include <margo.h>

namespace tl = thallium;

namespace bedrock {

MargoManager::MargoManager(const std::vector<std::string>& addresses,
                           const std::vector<std::string>& configs)
: self(std::make_shared<MargoManagerImpl>()) {
    if (addresses.empty() || configs.empty()) {
        throw Exception{"Addresses and configs vectors must not be empty"};
    }
    if (addresses.size() != configs.size()) {
        throw Exception{"Addresses and configs vectors must have the same size"};
    }
    for (size_t i = 0; i < addresses.size(); i++) {
        std::string resolvedAddress = addresses[i];
        const auto& configString = configs[i];
        if (resolvedAddress.empty()) {
            if (!configString.empty() && configString != "null") {
                try {
                    auto config = json::parse(configString);
                    if (config.contains("mercury") && config["mercury"].is_object()
                        && config["mercury"].contains("address")
                        && config["mercury"]["address"].is_string()) {
                        auto addr = config["mercury"]["address"].get<std::string>();
                        auto colon = addr.find(':');
                        if (colon != std::string::npos) {
                            resolvedAddress = addr.substr(0, colon);
                        }
                    }
                } catch (const json::parse_error&) {}
            }
            if (resolvedAddress.empty()) {
                throw Exception{
                    "Could not infer protocol for engine {}: no address provided and "
                    "no valid mercury.address found in configuration", i};
            }
        }
        struct margo_init_info args = MARGO_INIT_INFO_INITIALIZER;
        if (!configString.empty() && configString != "null") {
            args.json_config = configString.c_str();
        }
        self->m_engines.push_back(
            tl::engine{resolvedAddress.c_str(), MARGO_SERVER_MODE, &args});
        auto& engine = self->m_engines.back();
        engine.enable_remote_shutdown();
        margo_instance_ref_incr(engine.get_margo_instance());
        setupMargoLoggingForInstance(engine.get_margo_instance());
    }
}

// LCOV_EXCL_START

MargoManager::MargoManager(const MargoManager&) = default;

MargoManager::MargoManager(MargoManager&&) = default;

MargoManager& MargoManager::operator=(const MargoManager&) = default;

MargoManager& MargoManager::operator=(MargoManager&&) = default;

MargoManager::~MargoManager() = default;

MargoManager::operator bool() const { return static_cast<bool>(self); }
// LCOV_EXCL_STOP

std::string MargoManager::getCurrentConfig() const {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    return self->makeConfig().dump();
}

std::shared_ptr<NamedDependency> MargoManager::getPool(const std::string& name) const {
    try {
        auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
        auto pool = self->m_engines[0].pools()[name];
        return std::make_shared<PoolRef>(self->m_engines[0], pool.name(), pool);
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

std::shared_ptr<NamedDependency> MargoManager::getPool(uint32_t index) const {
    try {
        auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
        auto pool = self->m_engines[0].pools()[index];
        return std::make_shared<PoolRef>(self->m_engines[0], pool.name(), pool);
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

std::shared_ptr<NamedDependency> MargoManager::getPool(ABT_pool abt_pool) const {
    try {
        auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
        auto pool = self->m_engines[0].pools()[tl::pool{abt_pool}];
        return std::make_shared<PoolRef>(self->m_engines[0], pool.name(), pool);
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

size_t MargoManager::getNumPools() const {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    return self->m_engines[0].pools().size();
}

std::shared_ptr<NamedDependency> MargoManager::addPool(const std::string& config) {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    auto mid = self->m_engines[0].get_margo_instance();
    margo_pool_info info;
    hg_return_t ret = margo_add_pool_from_json(mid, config.c_str(), &info);
    if (ret != HG_SUCCESS) {
        throw BEDROCK_DETAILED_EXCEPTION(
                "Could not add pool to Margo instance");
    }
    return std::make_shared<PoolRef>(self->m_engines[0], info.name, tl::pool{info.pool});
}

void MargoManager::removePool(uint32_t index) {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    try {
        self->m_engines[0].pools().remove(index);
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

void MargoManager::removePool(const std::string& name) {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    try {
        self->m_engines[0].pools().remove(name);
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

void MargoManager::removePool(ABT_pool pool) {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    try {
        self->m_engines[0].pools().remove(tl::pool{pool});
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

std::shared_ptr<NamedDependency> MargoManager::getXstream(const std::string& name) const {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    try {
        auto es = self->m_engines[0].xstreams()[name];
        return std::make_shared<XstreamRef>(self->m_engines[0], es.name(), es);
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

std::shared_ptr<NamedDependency> MargoManager::getXstream(uint32_t index) const {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    try {
        auto es = self->m_engines[0].xstreams()[index];
        return std::make_shared<XstreamRef>(self->m_engines[0], es.name(), es);
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

std::shared_ptr<NamedDependency> MargoManager::getXstream(ABT_xstream abt_es) const {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    try {
        auto es = self->m_engines[0].xstreams()[tl::xstream{abt_es}];
        return std::make_shared<XstreamRef>(self->m_engines[0], es.name(), es);
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

size_t MargoManager::getNumXstreams() const {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    try {
        return self->m_engines[0].xstreams().size();
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

std::shared_ptr<NamedDependency> MargoManager::addXstream(const std::string& config) {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    auto mid = self->m_engines[0].get_margo_instance();
    margo_xstream_info info;
    hg_return_t ret = margo_add_xstream_from_json(mid, config.c_str(), &info);
    if (ret != HG_SUCCESS) {
        throw BEDROCK_DETAILED_EXCEPTION(
                "Could not add xstream to Margo instance");
    }
    return std::make_shared<XstreamRef>(self->m_engines[0], info.name, tl::xstream{info.xstream});
}

void MargoManager::removeXstream(uint32_t index) {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    try {
        self->m_engines[0].xstreams().remove(index);
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

void MargoManager::removeXstream(const std::string& name) {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    try {
        self->m_engines[0].xstreams().remove(name);
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

void MargoManager::removeXstream(ABT_xstream es) {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    try {
        self->m_engines[0].xstreams().remove(tl::xstream{es});
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

// --- Multi-engine methods ---

size_t MargoManager::getNumEngines() const {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    return self->m_engines.size();
}

margo_instance_id MargoManager::getMargoInstance(size_t engineIndex) const {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    if (engineIndex >= self->m_engines.size())
        throw Exception{"Engine index {} out of range (have {} engines)",
                         engineIndex, self->m_engines.size()};
    return self->m_engines[engineIndex].get_margo_instance();
}

const tl::engine& MargoManager::getThalliumEngine(size_t engineIndex) const {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    if (engineIndex >= self->m_engines.size())
        throw Exception{"Engine index {} out of range (have {} engines)",
                         engineIndex, self->m_engines.size()};
    return self->m_engines[engineIndex];
}

std::shared_ptr<NamedDependency> MargoManager::getDefaultHandlerPool(size_t engineIndex) const {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    if (engineIndex >= self->m_engines.size())
        throw Exception{"Engine index {} out of range (have {} engines)",
                         engineIndex, self->m_engines.size()};
    try {
        auto pool = self->m_engines[engineIndex].get_handler_pool();
        auto name = self->m_engines[engineIndex].pools()[pool].name();
        return std::make_shared<PoolRef>(self->m_engines[engineIndex], name, pool);
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

} // namespace bedrock
