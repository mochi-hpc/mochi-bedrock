/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/ServiceHandle.hpp"
#include "bedrock/ServiceGroupHandle.hpp"
#include "bedrock/RequestResult.hpp"
#include "bedrock/service-handle.h"

#include "Exception.hpp"
#include "AsyncRequestImpl.hpp"
#include "ClientImpl.hpp"
#include "ServiceGroupHandleImpl.hpp"

#include <cstring>

namespace bedrock {

using json = nlohmann::json;

ServiceGroupHandle::ServiceGroupHandle() = default;

ServiceGroupHandle::ServiceGroupHandle(const std::shared_ptr<ServiceGroupHandleImpl>& impl)
: self(impl) {}

ServiceGroupHandle::ServiceGroupHandle(const ServiceGroupHandle&) = default;

ServiceGroupHandle::ServiceGroupHandle(ServiceGroupHandle&&) = default;

ServiceGroupHandle& ServiceGroupHandle::operator=(const ServiceGroupHandle&) = default;

ServiceGroupHandle& ServiceGroupHandle::operator=(ServiceGroupHandle&&) = default;

ServiceGroupHandle::~ServiceGroupHandle() = default;

ServiceGroupHandle::operator bool() const { return static_cast<bool>(self); }

Client ServiceGroupHandle::client() const { return Client(self->m_client); }

void ServiceGroupHandle::refresh() const {
#ifdef ENABLE_SSG
    if (!self) throw DETAILED_EXCEPTION("Invalid bedrock::ServiceGroupHandle object");
    if (self->m_gid == SSG_GROUP_ID_INVALID)
        throw DETAILED_EXCEPTION("ServiceGroupHandle not associated with an SSG group");
    std::vector<std::string> addresses;
    margo_instance_id mid = self->m_client->m_engine.get_margo_instance();
    int ret = ssg_group_refresh(mid, self->m_gid);
    if (ret != SSG_SUCCESS)
        throw DETAILED_EXCEPTION("Could not refresh SSG group view "
                "(ssg_group_refresh returned {})", ret);
    int group_size = 0;
    ret = ssg_get_group_size(self->m_gid, &group_size);
    if (ret != SSG_SUCCESS)
        throw DETAILED_EXCEPTION("Could not get SSG group size "
                "(ssg_get_group_size returned {})", ret);
    addresses.reserve(group_size);
    for (int i = 0; i < group_size; i++) {
        ssg_member_id_t member_id = SSG_MEMBER_ID_INVALID;
        ret = ssg_get_group_member_id_from_rank(self->m_gid, i, &member_id);
        if (member_id == SSG_MEMBER_ID_INVALID || ret != SSG_SUCCESS) {
            throw DETAILED_EXCEPTION("Could not get member ID from rank {} "
                    "(ssg_get_group_member_id_from_rank returned {})",
                    i, ret);
        }
        char* addr = NULL;
        ret        = ssg_get_group_member_addr_str(self->m_gid, member_id, &addr);
        if (addr == NULL || ret != SSG_SUCCESS) {
            throw DETAILED_EXCEPTION(
                    "Could not get address from SSG member {} (rank {}) "
                    "(ssg_get_group_member_addr_str returned {})", member_id,
                    i, ret);
        }
        addresses.emplace_back(addr);
    }
    std::vector<std::shared_ptr<ServiceHandleImpl>> shs;
    shs.reserve(addresses.size());
    for(const auto& addr : addresses) {
        auto sh = Client(self->m_client).makeServiceHandle(addr, self->m_provider_id);
        shs.push_back(sh.self);
    }
    self->m_shs = std::move(shs);
#endif
}

#define CALL_ON_EACH_SERVICE_HANDLE(__f__, ...) do { \
    if (not self) throw DETAILED_EXCEPTION("Invalid bedrock::ServiceGroupHandle object"); \
    if (req && req->active()) throw DETAILED_EXCEPTION("AsyncRequest object passed is already in use"); \
    std::vector<std::shared_ptr<AsyncRequestImpl>> reqs(self->m_shs.size()); \
    for(unsigned i=0; i < self->m_shs.size(); i++) { \
        AsyncRequest r; \
        ServiceHandle(self->m_shs[i]).__f__(__VA_ARGS__, &r); \
        reqs[i] = std::move(r.self); \
    } \
    auto req_impl = std::make_shared<MultiAsyncRequest>(std::move(reqs)); \
    if(!req) req_impl->wait(); \
    else req->self = std::move(req_impl); \
} while(0)

void ServiceGroupHandle::loadModule(const std::string& name, const std::string& path, AsyncRequest* req) const {
    CALL_ON_EACH_SERVICE_HANDLE(loadModule, name, path);
}

void ServiceGroupHandle::startProvider(const std::string& name,
                                  const std::string& type, uint16_t provider_id,
                                  const std::string&   pool,
                                  const std::string&   config,
                                  const DependencyMap& dependencies,
                                  AsyncRequest*        req) const {
    CALL_ON_EACH_SERVICE_HANDLE(startProvider, name, type, provider_id, pool, config, dependencies);
}

void ServiceGroupHandle::changeProviderPool(const std::string& provider_name,
                                       const std::string&   pool,
                                       AsyncRequest*        req) const {
    CALL_ON_EACH_SERVICE_HANDLE(changeProviderPool, provider_name, pool);
}

void ServiceGroupHandle::addClient(const std::string&   name,
                                 const std::string&   type,
                                 const std::string&   config,
                                 const DependencyMap& dependencies,
                                 AsyncRequest*        req) const {
    CALL_ON_EACH_SERVICE_HANDLE(addClient, name, type, config, dependencies);
}

void ServiceGroupHandle::addABTioInstance(const std::string& name,
                                        const std::string& pool,
                                        const std::string& config,
                                        AsyncRequest*      req) const {
    CALL_ON_EACH_SERVICE_HANDLE(addABTioInstance, name, pool, config);
}

void ServiceGroupHandle::addSSGgroup(const std::string& config,
                                AsyncRequest*      req) const {
    CALL_ON_EACH_SERVICE_HANDLE(addSSGgroup, config);
}

void ServiceGroupHandle::addPool(const std::string& config,
                            AsyncRequest*      req) const {
    CALL_ON_EACH_SERVICE_HANDLE(addPool, config);
}

void ServiceGroupHandle::addXstream(const std::string& config,
                               AsyncRequest*      req) const {
    CALL_ON_EACH_SERVICE_HANDLE(addXstream, config);
}

void ServiceGroupHandle::removePool(const std::string& name,
                               AsyncRequest*      req) const {
    CALL_ON_EACH_SERVICE_HANDLE(removePool, name);
}

void ServiceGroupHandle::removeXstream(const std::string& name,
                                  AsyncRequest*      req) const {
    CALL_ON_EACH_SERVICE_HANDLE(removeXstream, name);
}

void ServiceGroupHandle::getConfig(std::string* result, AsyncRequest* req) const {
    if (not self) throw DETAILED_EXCEPTION("Invalid bedrock::ServiceGroupHandle object");
    if (req && req->active()) throw DETAILED_EXCEPTION("AsyncRequest object passed is already in use");
    const auto n = self->m_shs.size();
    std::vector<std::shared_ptr<AsyncRequestImpl>> reqs(n);
    std::vector<std::string> results(n);
    for(unsigned i=0; i < n; i++) {
        AsyncRequest r;
        ServiceHandle(self->m_shs[i]).getConfig(&results[i], &r);
        reqs[i] = std::move(r.self);
    }
    auto req_impl = std::make_shared<MultiAsyncRequest>(std::move(reqs));
    req_impl->m_wait_callback = [n, self=this->self, results=std::move(results), result](MultiAsyncRequest&) {
        auto obj = json::object();
        for(unsigned i = 0; i < n; i++) {
            auto addr = static_cast<std::string>(self->m_shs[i]->m_ph);
            obj[addr] = results[i].empty() ? json() : json::parse(results[i]);
        }
        if(result) *result = obj.dump();
    };
    if(!req) req_impl->wait();
    else req->self = std::move(req_impl);
}

void ServiceGroupHandle::queryConfig(const std::string& script, std::string* result,
                                     AsyncRequest* req) const {
    if (not self) throw DETAILED_EXCEPTION("Invalid bedrock::ServiceGroupHandle object");
    if (req && req->active()) throw DETAILED_EXCEPTION("AsyncRequest object passed is already in use");
    const auto n = self->m_shs.size();
    std::vector<std::shared_ptr<AsyncRequestImpl>> reqs(n);
    std::vector<std::string> results(n);
    for(unsigned i=0; i < n; i++) {
        AsyncRequest r;
        ServiceHandle(self->m_shs[i]).queryConfig(script, &results[i], &r);
        reqs[i] = std::move(r.self);
    }
    auto req_impl = std::make_shared<MultiAsyncRequest>(std::move(reqs));
    req_impl->m_wait_callback = [n, self=this->self, results=std::move(results), result](MultiAsyncRequest&) {
        auto obj = json::object();
        for(unsigned i = 0; i < n; i++) {
            auto addr = static_cast<std::string>(self->m_shs[i]->m_ph);
            obj[addr] = results[i].empty() ? json() : json::parse(results[i]);
        }
        if(result) *result = obj.dump();
    };
    if(!req) req_impl->wait();
    else req->self = std::move(req_impl);
}

} // namespace bedrock
