/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/ServiceHandle.hpp"
#include "bedrock/RequestResult.hpp"
#include "bedrock/Exception.hpp"

#include "AsyncRequestImpl.hpp"
#include "ClientImpl.hpp"
#include "ServiceHandleImpl.hpp"

#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/pair.hpp>

namespace bedrock {

ServiceHandle::ServiceHandle() = default;

ServiceHandle::ServiceHandle(const std::shared_ptr<ServiceHandleImpl>& impl)
: self(impl) {}

ServiceHandle::ServiceHandle(const ServiceHandle&) = default;

ServiceHandle::ServiceHandle(ServiceHandle&&) = default;

ServiceHandle& ServiceHandle::operator=(const ServiceHandle&) = default;

ServiceHandle& ServiceHandle::operator=(ServiceHandle&&) = default;

ServiceHandle::~ServiceHandle() = default;

ServiceHandle::operator bool() const { return static_cast<bool>(self); }

Client ServiceHandle::client() const { return Client(self->m_client); }

void ServiceHandle::sayHello() const {
    if (not self) throw Exception("Invalid bedrock::ServiceHandle object");
    auto& rpc        = self->m_client->m_say_hello;
    auto& ph         = self->m_ph;
    auto& Service_id = self->m_Service_id;
    rpc.on(ph)(Service_id);
}

void ServiceHandle::computeSum(int32_t x, int32_t y, int32_t* result,
                               AsyncRequest* req) const {
    if (not self) throw Exception("Invalid bedrock::ServiceHandle object");
    auto& rpc        = self->m_client->m_compute_sum;
    auto& ph         = self->m_ph;
    auto& Service_id = self->m_Service_id;
    if (req == nullptr) { // synchronous call
        RequestResult<int32_t> response = rpc.on(ph)(Service_id, x, y);
        if (response.success()) {
            if (result) *result = response.value();
        } else {
            throw Exception(response.error());
        }
    } else { // asynchronous call
        auto async_response = rpc.on(ph).async(Service_id, x, y);
        auto async_request_impl
            = std::make_shared<AsyncRequestImpl>(std::move(async_response));
        async_request_impl->m_wait_callback
            = [result](AsyncRequestImpl& async_request_impl) {
                  RequestResult<int32_t> response
                      = async_request_impl.m_async_response.wait();
                  if (response.success()) {
                      if (result) *result = response.value();
                  } else {
                      throw Exception(response.error());
                  }
              };
        *req = AsyncRequest(std::move(async_request_impl));
    }
}

} // namespace bedrock
