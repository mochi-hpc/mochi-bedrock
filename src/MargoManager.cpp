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

MargoManager::MargoManager(margo_instance_id mid)
: self(std::make_shared<MargoManagerImpl>()) {
    self->m_engine = tl::engine(mid);
}

MargoManager::MargoManager(const std::string& address,
                           const std::string& configString)
: self(std::make_shared<MargoManagerImpl>()) {
    struct margo_init_info args = MARGO_INIT_INFO_INITIALIZER;
    if (!configString.empty() && configString != "null") {
        args.json_config = configString.c_str();
    }
    self->m_engine = tl::engine{address.c_str(), MARGO_SERVER_MODE, &args};
    self->m_engine.enable_remote_shutdown();
    margo_instance_ref_incr(self->m_engine.get_margo_instance());
    setupMargoLoggingForInstance(self->m_engine.get_margo_instance());
}

// LCOV_EXCL_START

MargoManager::MargoManager(const MargoManager&) = default;

MargoManager::MargoManager(MargoManager&&) = default;

MargoManager& MargoManager::operator=(const MargoManager&) = default;

MargoManager& MargoManager::operator=(MargoManager&&) = default;

MargoManager::~MargoManager() = default;

MargoManager::operator bool() const { return static_cast<bool>(self); }

// LCOV_EXCL_STOP

margo_instance_id MargoManager::getMargoInstance() const {
    if(!self) return MARGO_INSTANCE_NULL;
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    return self->m_engine.get_margo_instance();
}

const tl::engine& MargoManager::getThalliumEngine() const {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    return self->m_engine;
}

std::string MargoManager::getCurrentConfig() const {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    return self->makeConfig().dump();
}

std::shared_ptr<NamedDependency> MargoManager::getDefaultHandlerPool() const {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    try {
        auto pool = self->m_engine.get_handler_pool();
        auto name = self->m_engine.pools()[pool].name();
        return std::make_shared<PoolRef>(self->m_engine, name, pool);
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

std::shared_ptr<NamedDependency> MargoManager::getPool(const std::string& name) const {
    try {
        auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
        auto pool = self->m_engine.pools()[name];
        return std::make_shared<PoolRef>(self->m_engine, pool.name(), pool);
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

std::shared_ptr<NamedDependency> MargoManager::getPool(uint32_t index) const {
    try {
        auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
        auto pool = self->m_engine.pools()[index];
        return std::make_shared<PoolRef>(self->m_engine, pool.name(), pool);
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

std::shared_ptr<NamedDependency> MargoManager::getPool(ABT_pool abt_pool) const {
    try {
        auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
        auto pool = self->m_engine.pools()[tl::pool{abt_pool}];
        return std::make_shared<PoolRef>(self->m_engine, pool.name(), pool);
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

size_t MargoManager::getNumPools() const {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    return self->m_engine.pools().size();
}

std::shared_ptr<NamedDependency> MargoManager::addPool(const std::string& config) {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    auto mid = self->m_engine.get_margo_instance();
    margo_pool_info info;
    hg_return_t ret = margo_add_pool_from_json(mid, config.c_str(), &info);
    if (ret != HG_SUCCESS) {
        throw BEDROCK_DETAILED_EXCEPTION(
                "Could not add pool to Margo instance");
    }
    return std::make_shared<PoolRef>(self->m_engine, info.name, tl::pool{info.pool});
}

void MargoManager::removePool(uint32_t index) {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    try {
        self->m_engine.pools().remove(index);
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

void MargoManager::removePool(const std::string& name) {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    try {
        self->m_engine.pools().remove(name);
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

void MargoManager::removePool(ABT_pool pool) {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    try {
        self->m_engine.pools().remove(tl::pool{pool});
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

std::shared_ptr<NamedDependency> MargoManager::getXstream(const std::string& name) const {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    try {
        auto es = self->m_engine.xstreams()[name];
        return std::make_shared<XstreamRef>(self->m_engine, es.name(), es);
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

std::shared_ptr<NamedDependency> MargoManager::getXstream(uint32_t index) const {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    try {
        auto es = self->m_engine.xstreams()[index];
        return std::make_shared<XstreamRef>(self->m_engine, es.name(), es);
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

std::shared_ptr<NamedDependency> MargoManager::getXstream(ABT_xstream abt_es) const {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    try {
        auto es = self->m_engine.xstreams()[tl::xstream{abt_es}];
        return std::make_shared<XstreamRef>(self->m_engine, es.name(), es);
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

size_t MargoManager::getNumXstreams() const {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    try {
        return self->m_engine.xstreams().size();
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

std::shared_ptr<NamedDependency> MargoManager::addXstream(const std::string& config) {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    auto mid = self->m_engine.get_margo_instance();
    margo_xstream_info info;
    hg_return_t ret = margo_add_xstream_from_json(mid, config.c_str(), &info);
    if (ret != HG_SUCCESS) {
        throw BEDROCK_DETAILED_EXCEPTION(
                "Could not add xstream to Margo instance");
    }
    return std::make_shared<XstreamRef>(self->m_engine, info.name, tl::xstream{info.xstream});
}

void MargoManager::removeXstream(uint32_t index) {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    try {
        self->m_engine.xstreams().remove(index);
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

void MargoManager::removeXstream(const std::string& name) {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    try {
        self->m_engine.xstreams().remove(name);
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}

void MargoManager::removeXstream(ABT_xstream es) {
    auto guard = std::unique_lock<tl::mutex>(self->m_mtx);
    try {
        self->m_engine.xstreams().remove(tl::xstream{es});
    } catch(const tl::exception& ex) {
        throw Exception{"{}", ex.what()};
    }
}


} // namespace bedrock
