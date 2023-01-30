/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/MargoManager.hpp"
#include "bedrock/Exception.hpp"
#include "MargoManagerImpl.hpp"
#include "MargoLogging.hpp"
#include <margo.h>

namespace tl = thallium;

namespace bedrock {

MargoManager::MargoManager(margo_instance_id mid)
: self(std::make_shared<MargoManagerImpl>()) {
    self->m_mid    = mid;
    self->m_engine = tl::engine(mid);
}

MargoManager::MargoManager(const std::string& address,
                           const std::string& configString)
: self(std::make_shared<MargoManagerImpl>()) {
    struct margo_init_info args = MARGO_INIT_INFO_INITIALIZER;
    if (!configString.empty() && configString != "null") {
        args.json_config = configString.c_str();
    }
    self->m_mid = margo_init_ext(address.c_str(), MARGO_SERVER_MODE, &args);
    if (self->m_mid == MARGO_INSTANCE_NULL)
        throw Exception("Could not initialize Margo");
    margo_enable_remote_shutdown(self->m_mid);
    self->m_engine = tl::engine(self->m_mid);
    setupMargoLoggingForInstance(self->m_mid);
}

MargoManager::MargoManager(const MargoManager&) = default;

MargoManager::MargoManager(MargoManager&&) = default;

MargoManager& MargoManager::operator=(const MargoManager&) = default;

MargoManager& MargoManager::operator=(MargoManager&&) = default;

MargoManager::~MargoManager() = default;

MargoManager::operator bool() const { return static_cast<bool>(self); }

margo_instance_id MargoManager::getMargoInstance() const {
    return self ? self->m_mid : MARGO_INSTANCE_NULL;
}

const tl::engine& MargoManager::getThalliumEngine() const {
    return self->m_engine;
}

std::string MargoManager::getCurrentConfig() const {
    return self->makeConfig().dump();
}

ABT_pool MargoManager::getDefaultHandlerPool() const {
    ABT_pool p;
    int      ret = margo_get_handler_pool(self->m_mid, &p);
    if (ret != HG_SUCCESS) {
        throw Exception(
            "Could not get handler pool (margo_get_handler_pool returned {})",
            ret);
    }
    return p;
}

MargoManager::PoolInfo MargoManager::getPool(int index) const {
    margo_pool_info info = {ABT_POOL_NULL,"",0};
    hg_return_t ret = margo_find_pool_by_index(self->m_mid, index, &info);
    if (ret != HG_SUCCESS) {
        throw Exception(
            "Could not get pool (margo_find_pool_by_index returned {})",
            ret);
    }
    return {info.pool, info.name, info.index};
}

MargoManager::PoolInfo MargoManager::getPool(const std::string& name) const {
    margo_pool_info info = {ABT_POOL_NULL,"",0};
    hg_return_t ret = margo_find_pool_by_name(self->m_mid, name.c_str(), &info);
    if (ret != HG_SUCCESS) {
        throw Exception(
            "Could not get pool (margo_find_pool_by_name returned {})",
            ret);
    }
    return {info.pool, info.name, info.index};
}

MargoManager::PoolInfo MargoManager::getPool(ABT_pool pool) const {
    margo_pool_info info = {ABT_POOL_NULL,"",0};
    hg_return_t ret = margo_find_pool_by_handle(self->m_mid, pool, &info);
    if (ret != HG_SUCCESS) {
        throw Exception(
            "Could not get pool (margo_find_pool_by_handle returned {})",
            ret);
    }
    return {info.pool, info.name, info.index};
}

size_t MargoManager::getNumPools() const {
    return margo_get_num_pools(self->m_mid);
}

MargoManager::PoolInfo MargoManager::addPool(const std::string& config) {
    margo_pool_info info;
    hg_return_t ret = margo_add_pool_from_json(
        self->m_mid, config.c_str(), &info);
    if (ret != HG_SUCCESS) {
        throw Exception(
            "Could not add pool (margo_add_pool_from_json returned {})",
            ret);
    }
    return {info.pool, info.name, info.index};
}

void MargoManager::removePool(int index) {
    hg_return_t ret = margo_remove_pool_by_index(self->m_mid, index);
    if (ret != HG_SUCCESS) {
        throw Exception(
            "Could not remove pool (margo_remove_pool_by_index returned {})",
            ret);
    }
}

void MargoManager::removePool(const std::string& name) {
    hg_return_t ret = margo_remove_pool_by_name(self->m_mid, name.c_str());
    if (ret != HG_SUCCESS) {
        throw Exception(
            "Could not remove pool (margo_remove_pool_by_name returned {})",
            ret);
    }
}

void MargoManager::removePool(ABT_pool pool) {
    hg_return_t ret = margo_remove_pool_by_handle(self->m_mid, pool);
    if (ret != HG_SUCCESS) {
        throw Exception(
            "Could not remove pool (margo_remove_pool_by_handle returned {})",
            ret);
    }
}

MargoManager::XstreamInfo MargoManager::getXstream(int index) const {
    margo_xstream_info info = {ABT_XSTREAM_NULL,"",0};
    hg_return_t ret = margo_find_xstream_by_index(self->m_mid, index, &info);
    if (ret != 0) {
        throw Exception(
            "Could not get xstream (margo_find_xstream_by_index returned {})",
            ret);
    }
    return {info.xstream, info.name, info.index};
}

MargoManager::XstreamInfo MargoManager::getXstream(const std::string& name) const {
    margo_xstream_info info = {ABT_XSTREAM_NULL,"",0};
    hg_return_t ret = margo_find_xstream_by_name(self->m_mid, name.c_str(), &info);
    if (ret != 0) {
        throw Exception(
            "Could not get xstream (margo_find_xstream_by_name returned {})",
            ret);
    }
    return {info.xstream, info.name, info.index};
}

MargoManager::XstreamInfo MargoManager::getXstream(ABT_xstream xstream) const {
    margo_xstream_info info = {ABT_XSTREAM_NULL,"",0};
    hg_return_t ret = margo_find_xstream_by_handle(self->m_mid, xstream, &info);
    if (ret != 0) {
        throw Exception(
            "Could not get xstream (margo_find_xstream_by_handle returned {})",
            ret);
    }
    return {info.xstream, info.name, info.index};
}

size_t MargoManager::getNumXstreams() const {
    return margo_get_num_xstreams(self->m_mid);
}

MargoManager::XstreamInfo MargoManager::addXstream(const std::string& config) {
    margo_xstream_info info;
    hg_return_t ret = margo_add_xstream_from_json(
        self->m_mid, config.c_str(), &info);
    if (ret != HG_SUCCESS) {
        throw Exception(
            "Could not add ES (margo_add_xstream_from_json returned {})",
            ret);
    }
    return {info.xstream, info.name, info.index};
}

void MargoManager::removeXstream(int index) {
    hg_return_t ret = margo_remove_xstream_by_index(self->m_mid, index);
    if (ret != HG_SUCCESS) {
        throw Exception(
            "Could not remove pool (margo_remove_xstream_by_index returned {})",
            ret);
    }
}

void MargoManager::removeXstream(const std::string& name) {
    hg_return_t ret = margo_remove_xstream_by_name(self->m_mid, name.c_str());
    if (ret != HG_SUCCESS) {
        throw Exception(
            "Could not remove pool (margo_remove_xstream_by_name returned {})",
            ret);
    }
}

void MargoManager::removeXstream(ABT_xstream xstream) {
    hg_return_t ret = margo_remove_xstream_by_handle(self->m_mid, xstream);
    if (ret != HG_SUCCESS) {
        throw Exception(
            "Could not remove xstream (margo_remove_xstream_by_handle returned {})",
            ret);
    }
}

} // namespace bedrock
