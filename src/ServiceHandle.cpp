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

void ServiceHandle::getConfig(std::string* result, AsyncRequest* req) const {
    if (not self) throw Exception("Invalid bedrock::ServiceHandle object");
    auto& rpc = self->m_client->m_get_config;
    auto& ph  = self->m_ph;
    if (req == nullptr) { // synchronous call
        RequestResult<std::string> response = rpc.on(ph)();
        if (response.success()) {
            if (result) *result = std::move(response.value());
        } else {
            throw Exception(response.error());
        }
    } else { // asynchronous call
        auto async_response = rpc.on(ph).async();
        auto async_request_impl
            = std::make_shared<AsyncRequestImpl>(std::move(async_response));
        async_request_impl->m_wait_callback
            = [result](AsyncRequestImpl& async_request_impl) {
                  RequestResult<std::string> response
                      = async_request_impl.m_async_response.wait();
                  if (response.success()) {
                      if (result) *result = std::move(response.value());
                  } else {
                      throw Exception(response.error());
                  }
              };
        *req = AsyncRequest(std::move(async_request_impl));
    }
}

} // namespace bedrock
