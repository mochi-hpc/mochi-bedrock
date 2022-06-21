/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/ServiceHandle.hpp"
#include "bedrock/RequestResult.hpp"
#include "bedrock/Exception.hpp"
#include "bedrock/service-handle.h"

#include "AsyncRequestImpl.hpp"
#include "ClientImpl.hpp"
#include "ServiceHandleImpl.hpp"

#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/pair.hpp>
#include <thallium/serialization/stl/unordered_map.hpp>
#include <thallium/serialization/stl/vector.hpp>

#include <cstring>

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

void ServiceHandle::loadModule(const std::string& name, const std::string& path,
                               AsyncRequest* req) const {
    if (not self) throw Exception("Invalid bedrock::ServiceHandle object");
    auto& rpc = self->m_client->m_load_module;
    auto& ph  = self->m_ph;
    if (req == nullptr) { // synchronous call
        RequestResult<bool> response = rpc.on(ph)(name, path);
        if (!response.success()) { throw Exception(response.error()); }
    } else { // asynchronous call
        auto async_response = rpc.on(ph).async(name, path);
        auto async_request_impl
            = std::make_shared<AsyncRequestImpl>(std::move(async_response));
        async_request_impl->m_wait_callback
            = [](AsyncRequestImpl& async_request_impl) {
                  RequestResult<std::string> response
                      = async_request_impl.m_async_response.wait();
                  if (!response.success()) {
                      throw Exception(response.error());
                  }
              };
        *req = AsyncRequest(std::move(async_request_impl));
    }
}

void ServiceHandle::startProvider(const std::string& name,
                                  const std::string& type, uint16_t provider_id,
                                  const std::string&   pool,
                                  const std::string&   config,
                                  const DependencyMap& dependencies,
                                  AsyncRequest*        req) const {
    if (not self) throw Exception("Invalid bedrock::ServiceHandle object");
    auto& rpc = self->m_client->m_start_provider;
    auto& ph  = self->m_ph;
    if (req == nullptr) { // synchronous call
        RequestResult<bool> response
            = rpc.on(ph)(name, type, provider_id, pool, config, dependencies);
        if (!response.success()) { throw Exception(response.error()); }
    } else { // asynchronous call
        auto async_response = rpc.on(ph).async(name, type, provider_id, pool,
                                               config, dependencies);
        auto async_request_impl
            = std::make_shared<AsyncRequestImpl>(std::move(async_response));
        async_request_impl->m_wait_callback
            = [](AsyncRequestImpl& async_request_impl) {
                  RequestResult<std::string> response
                      = async_request_impl.m_async_response.wait();
                  if (!response.success()) {
                      throw Exception(response.error());
                  }
              };
        *req = AsyncRequest(std::move(async_request_impl));
    }
}

void ServiceHandle::createClient(const std::string&   name,
                                 const std::string&   type,
                                 const std::string&   config,
                                 const DependencyMap& dependencies,
                                 AsyncRequest*        req) const {
    if (not self) throw Exception("Invalid bedrock::ServiceHandle object");
    auto& rpc = self->m_client->m_create_client;
    auto& ph  = self->m_ph;
    if (req == nullptr) { // synchronous call
        RequestResult<bool> response
            = rpc.on(ph)(name, type, config, dependencies);
        if (!response.success()) { throw Exception(response.error()); }
    } else { // asynchronous call
        auto async_response
            = rpc.on(ph).async(name, type, config, dependencies);
        auto async_request_impl
            = std::make_shared<AsyncRequestImpl>(std::move(async_response));
        async_request_impl->m_wait_callback
            = [](AsyncRequestImpl& async_request_impl) {
                  RequestResult<std::string> response
                      = async_request_impl.m_async_response.wait();
                  if (!response.success()) {
                      throw Exception(response.error());
                  }
              };
        *req = AsyncRequest(std::move(async_request_impl));
    }
}

void ServiceHandle::createABTioInstance(const std::string& name,
                                        const std::string& pool,
                                        const std::string& config,
                                        AsyncRequest*      req) const {
    if (not self) throw Exception("Invalid bedrock::ServiceHandle object");
    auto& rpc = self->m_client->m_create_abtio;
    auto& ph  = self->m_ph;
    if (req == nullptr) { // synchronous call
        RequestResult<bool> response = rpc.on(ph)(name, pool, config);
        if (!response.success()) { throw Exception(response.error()); }
    } else { // asynchronous call
        auto async_response = rpc.on(ph).async(name, pool, config);
        auto async_request_impl
            = std::make_shared<AsyncRequestImpl>(std::move(async_response));
        async_request_impl->m_wait_callback
            = [](AsyncRequestImpl& async_request_impl) {
                  RequestResult<std::string> response
                      = async_request_impl.m_async_response.wait();
                  if (!response.success()) {
                      throw Exception(response.error());
                  }
              };
        *req = AsyncRequest(std::move(async_request_impl));
    }
}

void ServiceHandle::addSSGgroup(const std::string& config,
                                AsyncRequest*      req) const {
    if (not self) throw Exception("Invalid bedrock::ServiceHandle object");
    auto& rpc = self->m_client->m_add_ssg_group;
    auto& ph  = self->m_ph;
    if (req == nullptr) { // synchronous call
        RequestResult<bool> response = rpc.on(ph)(config);
        if (!response.success()) { throw Exception(response.error()); }
    } else { // asynchronous call
        auto async_response = rpc.on(ph).async(config);
        auto async_request_impl
            = std::make_shared<AsyncRequestImpl>(std::move(async_response));
        async_request_impl->m_wait_callback
            = [](AsyncRequestImpl& async_request_impl) {
                  RequestResult<std::string> response
                      = async_request_impl.m_async_response.wait();
                  if (!response.success()) {
                      throw Exception(response.error());
                  }
              };
        *req = AsyncRequest(std::move(async_request_impl));
    }
}

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

void ServiceHandle::queryConfig(const std::string& script, std::string* result,
                                AsyncRequest* req) const {
    if (not self) throw Exception("Invalid bedrock::ServiceHandle object");
    auto& rpc = self->m_client->m_query_config;
    auto& ph  = self->m_ph;
    if (req == nullptr) { // synchronous call
        RequestResult<std::string> response = rpc.on(ph)(script);
        if (response.success()) {
            if (result) *result = std::move(response.value());
        } else {
            throw Exception(response.error());
        }
    } else { // asynchronous call
        auto async_response = rpc.on(ph).async(script);
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

extern "C"
int bedrock_service_handle_create(
        bedrock_client_t client,
        const char* addr,
        uint16_t provider_id,
        bedrock_service_t* sh) {
    auto c = static_cast<bedrock::Client*>(client);
    *sh = static_cast<void*>(
            new bedrock::ServiceHandle(
                c->makeServiceHandle(addr, provider_id)));
    return 0;
}

extern "C"
int bedrock_service_handle_destroy(
        bedrock_service_t sh) {
    delete static_cast<bedrock::ServiceHandle*>(sh);
    return 0;
}

extern "C"
char* bedrock_service_query_config(
        bedrock_service_t sh,
        const char* script) {
    auto service_handle = static_cast<bedrock::ServiceHandle*>(sh);
    std::string result;
    service_handle->queryConfig(script, &result);
    return strdup(result.c_str());
}
