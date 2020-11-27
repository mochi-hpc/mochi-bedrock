/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/MargoContext.hpp"
#include "bedrock/Exception.hpp"
#include "MargoContextImpl.hpp"
#include "MargoLogging.hpp"
#include <margo.h>

namespace tl = thallium;

namespace bedrock {

MargoContext::MargoContext(margo_instance_id mid)
: self(std::make_shared<MargoContextImpl>()) {
    self->m_mid    = mid;
    self->m_engine = tl::engine(mid);
}

MargoContext::MargoContext(const std::string& address,
                           const std::string& configString)
: self(std::make_shared<MargoContextImpl>()) {
    struct margo_init_info args = MARGO_INIT_INFO_INITIALIZER;
    if (!configString.empty() && configString != "null") {
        args.json_config = configString.c_str();
    }
    self->m_mid = margo_init_ext(address.c_str(), MARGO_SERVER_MODE, &args);
    if (self->m_mid == MARGO_INSTANCE_NULL)
        throw Exception("Could not initialize Margo");
    self->m_engine = tl::engine(self->m_mid);
    setupMargoLoggingForInstance(self->m_mid);
}

MargoContext::MargoContext(const MargoContext&) = default;

MargoContext::MargoContext(MargoContext&&) = default;

MargoContext& MargoContext::operator=(const MargoContext&) = default;

MargoContext& MargoContext::operator=(MargoContext&&) = default;

MargoContext::~MargoContext() = default;

MargoContext::operator bool() const { return static_cast<bool>(self); }

margo_instance_id MargoContext::getMargoInstance() const {
    return self ? self->m_mid : MARGO_INSTANCE_NULL;
}

const tl::engine& MargoContext::getThalliumEngine() const {
    return self->m_engine;
}

std::string MargoContext::getCurrentConfig() const {
    return self->makeConfig().dump();
}

ABT_pool MargoContext::getPool(int index) const {
    ABT_pool pool = ABT_POOL_NULL;
    margo_get_pool_by_index(self->m_mid, index, &pool);
    return pool;
}

ABT_pool MargoContext::getPool(const std::string& name) const {
    ABT_pool pool = ABT_POOL_NULL;
    margo_get_pool_by_name(self->m_mid, name.c_str(), &pool);
    return pool;
}

size_t MargoContext::getNumPools() const {
    return margo_get_num_pools(self->m_mid);
}

std::pair<std::string, int> MargoContext::getPoolInfo(ABT_pool pool) const {
    std::pair<std::string, int> result = {"", -1};
    auto numPools = getNumPools();
    for(size_t i=0; i < numPools; i++) {
        ABT_pool p = getPool(i);
        if(p == pool) {
            result.first = margo_get_pool_name(self->m_mid, i);
            result.second = i;
            return result;
        }
    }
    return result;
}

} // namespace bedrock
