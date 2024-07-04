/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <bedrock/ServiceHandle.hpp>
#include <bedrock/ServiceGroupHandle.hpp>
#include <bedrock/RequestResult.hpp>
#include <bedrock/DetailedException.hpp>

#include "AsyncRequestImpl.hpp"
#include "ClientImpl.hpp"
#include "ServiceGroupHandleImpl.hpp"

#include <cstring>

namespace bedrock {

using json = nlohmann::json;

// LCOV_EXCL_START

ServiceGroupHandle::ServiceGroupHandle() = default;

ServiceGroupHandle::ServiceGroupHandle(const std::shared_ptr<ServiceGroupHandleImpl>& impl)
: self(impl) {}

ServiceGroupHandle::ServiceGroupHandle(const ServiceGroupHandle&) = default;

ServiceGroupHandle::ServiceGroupHandle(ServiceGroupHandle&&) = default;

ServiceGroupHandle& ServiceGroupHandle::operator=(const ServiceGroupHandle&) = default;

ServiceGroupHandle& ServiceGroupHandle::operator=(ServiceGroupHandle&&) = default;

ServiceGroupHandle::~ServiceGroupHandle() = default;

ServiceGroupHandle::operator bool() const { return static_cast<bool>(self); }

// LCOV_EXCL_STOP

Client ServiceGroupHandle::client() const { return Client(self->m_client); }

size_t ServiceGroupHandle::size() const {
    if (!self) throw BEDROCK_DETAILED_EXCEPTION("Invalid bedrock::ServiceGroupHandle object");
    return self->m_shs.size();
}

ServiceHandle ServiceGroupHandle::operator[](size_t i) const {
    if (!self) throw BEDROCK_DETAILED_EXCEPTION("Invalid bedrock::ServiceGroupHandle object");
    if (i >= self->m_shs.size()) throw BEDROCK_DETAILED_EXCEPTION("Invalid index {}", i);
    return self->m_shs[i];
}

void ServiceGroupHandle::refresh() const {
    if (!self) throw BEDROCK_DETAILED_EXCEPTION("Invalid bedrock::ServiceGroupHandle object");
    auto addresses = self->queryAddresses(true);
    std::vector<std::shared_ptr<ServiceHandleImpl>> shs;
    shs.reserve(addresses.size());
    for(const auto& addr : addresses) {
        auto sh = Client(self->m_client).makeServiceHandle(addr, self->m_provider_id);
        shs.push_back(sh.self);
    }
    self->m_shs = std::move(shs);
}

void ServiceGroupHandle::getConfig(std::string* result, AsyncRequest* req) const {
    if (not self) throw BEDROCK_DETAILED_EXCEPTION("Invalid bedrock::ServiceGroupHandle object");
    if (req && req->active()) throw BEDROCK_DETAILED_EXCEPTION("AsyncRequest object passed is already in use");
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
    if (not self) throw BEDROCK_DETAILED_EXCEPTION("Invalid bedrock::ServiceGroupHandle object");
    if (req && req->active()) throw BEDROCK_DETAILED_EXCEPTION("AsyncRequest object passed is already in use");
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
