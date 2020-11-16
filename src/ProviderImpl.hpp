/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_PROVIDER_IMPL_H
#define __BEDROCK_PROVIDER_IMPL_H

#include "bedrock/Backend.hpp"
#include "bedrock/UUID.hpp"

#include <thallium.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/vector.hpp>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <tuple>

#define FIND_SERVICE(__var__)                                                 \
    std::shared_ptr<Backend> __var__;                                         \
    do {                                                                      \
        std::lock_guard<tl::mutex> lock(m_backends_mtx);                      \
        auto                       it = m_backends.find(Service_id);          \
        if (it == m_backends.end()) {                                         \
            result.success() = false;                                         \
            result.error()   = "Service with UUID "s + Service_id.to_string() \
                           + " not found";                                    \
            req.respond(result);                                              \
            spdlog::error("[provider:{}] Service {} not found", id(),         \
                          Service_id.to_string());                            \
            return;                                                           \
        }                                                                     \
        __var__ = it->second;                                                 \
    } while (0)

namespace bedrock {

using namespace std::string_literals;
namespace tl = thallium;

class ProviderImpl : public tl::provider<ProviderImpl> {

    auto id() const { return get_provider_id(); } // for convenience

    using json = nlohmann::json;

  public:
    std::string m_token;
    tl::pool    m_pool;
    // Admin RPC
    tl::remote_procedure m_create_Service;
    tl::remote_procedure m_open_Service;
    tl::remote_procedure m_close_Service;
    tl::remote_procedure m_destroy_Service;
    // Client RPC
    tl::remote_procedure m_check_Service;
    tl::remote_procedure m_say_hello;
    tl::remote_procedure m_compute_sum;
    // Backends
    std::unordered_map<UUID, std::shared_ptr<Backend>> m_backends;
    tl::mutex                                          m_backends_mtx;

    ProviderImpl(const tl::engine& engine, uint16_t provider_id,
                 const tl::pool& pool)
    : tl::provider<ProviderImpl>(engine, provider_id), m_pool(pool),
      m_create_Service(
          define("bedrock_create_Service", &ProviderImpl::createService, pool)),
      m_open_Service(
          define("bedrock_open_Service", &ProviderImpl::openService, pool)),
      m_close_Service(
          define("bedrock_close_Service", &ProviderImpl::closeService, pool)),
      m_destroy_Service(define("bedrock_destroy_Service",
                               &ProviderImpl::destroyService, pool)),
      m_check_Service(
          define("bedrock_check_Service", &ProviderImpl::checkService, pool)),
      m_say_hello(define("bedrock_say_hello", &ProviderImpl::sayHello, pool)),
      m_compute_sum(
          define("bedrock_compute_sum", &ProviderImpl::computeSum, pool)) {
        spdlog::trace("[provider:{0}] Registered provider with id {0}", id());
    }

    ~ProviderImpl() {
        spdlog::trace("[provider:{}] Deregistering provider", id());
        m_create_Service.deregister();
        m_open_Service.deregister();
        m_close_Service.deregister();
        m_destroy_Service.deregister();
        m_check_Service.deregister();
        m_say_hello.deregister();
        m_compute_sum.deregister();
        spdlog::trace("[provider:{}]    => done!", id());
    }

    void createService(const tl::request& req, const std::string& token,
                       const std::string& Service_type,
                       const std::string& Service_config) {

        spdlog::trace("[provider:{}] Received createService request", id());
        spdlog::trace("[provider:{}]    => type = {}", id(), Service_type);
        spdlog::trace("[provider:{}]    => config = {}", id(), Service_config);

        auto                Service_id = UUID::generate();
        RequestResult<UUID> result;

        if (m_token.size() > 0 && m_token != token) {
            result.success() = false;
            result.error()   = "Invalid security token";
            req.respond(result);
            spdlog::error("[provider:{}] Invalid security token {}", id(),
                          token);
            return;
        }

        json json_config;
        try {
            json_config = json::parse(Service_config);
        } catch (json::parse_error& e) {
            result.error()   = e.what();
            result.success() = false;
            spdlog::error(
                "[provider:{}] Could not parse Service configuration for "
                "Service {}",
                id(), Service_id.to_string());
            req.respond(result);
            return;
        }

        std::unique_ptr<Backend> backend;
        try {
            backend = ServiceFactory::createService(Service_type, get_engine(),
                                                    json_config);
        } catch (const std::exception& ex) {
            result.success() = false;
            result.error()   = ex.what();
            spdlog::error(
                "[provider:{}] Error when creating Service {} of type {}:",
                id(), Service_id.to_string(), Service_type);
            spdlog::error("[provider:{}]    => {}", id(), result.error());
            req.respond(result);
            return;
        }

        if (not backend) {
            result.success() = false;
            result.error()   = "Unknown Service type "s + Service_type;
            spdlog::error(
                "[provider:{}] Unknown Service type {} for Service {}", id(),
                Service_type, Service_id.to_string());
            req.respond(result);
            return;
        } else {
            std::lock_guard<tl::mutex> lock(m_backends_mtx);
            m_backends[Service_id] = std::move(backend);
            result.value()         = Service_id;
        }

        req.respond(result);
        spdlog::trace(
            "[provider:{}] Successfully created Service {} of type {}", id(),
            Service_id.to_string(), Service_type);
    }

    void openService(const tl::request& req, const std::string& token,
                     const std::string& Service_type,
                     const std::string& Service_config) {

        spdlog::trace("[provider:{}] Received openService request", id());
        spdlog::trace("[provider:{}]    => type = {}", id(), Service_type);
        spdlog::trace("[provider:{}]    => config = {}", id(), Service_config);

        auto                Service_id = UUID::generate();
        RequestResult<UUID> result;

        if (m_token.size() > 0 && m_token != token) {
            result.success() = false;
            result.error()   = "Invalid security token";
            req.respond(result);
            spdlog::error("[provider:{}] Invalid security token {}", id(),
                          token);
            return;
        }

        json json_config;
        try {
            json_config = json::parse(Service_config);
        } catch (json::parse_error& e) {
            result.error()   = e.what();
            result.success() = false;
            spdlog::error(
                "[provider:{}] Could not parse Service configuration for "
                "Service {}",
                id(), Service_id.to_string());
            req.respond(result);
            return;
        }

        std::unique_ptr<Backend> backend;
        try {
            backend = ServiceFactory::openService(Service_type, get_engine(),
                                                  json_config);
        } catch (const std::exception& ex) {
            result.success() = false;
            result.error()   = ex.what();
            spdlog::error(
                "[provider:{}] Error when opening Service {} of type {}:", id(),
                Service_id.to_string(), Service_type);
            spdlog::error("[provider:{}]    => {}", id(), result.error());
            req.respond(result);
            return;
        }

        if (not backend) {
            result.success() = false;
            result.error()   = "Unknown Service type "s + Service_type;
            spdlog::error(
                "[provider:{}] Unknown Service type {} for Service {}", id(),
                Service_type, Service_id.to_string());
            req.respond(result);
            return;
        } else {
            std::lock_guard<tl::mutex> lock(m_backends_mtx);
            m_backends[Service_id] = std::move(backend);
            result.value()         = Service_id;
        }

        req.respond(result);
        spdlog::trace(
            "[provider:{}] Successfully created Service {} of type {}", id(),
            Service_id.to_string(), Service_type);
    }

    void closeService(const tl::request& req, const std::string& token,
                      const UUID& Service_id) {
        spdlog::trace(
            "[provider:{}] Received closeService request for Service {}", id(),
            Service_id.to_string());

        RequestResult<bool> result;

        if (m_token.size() > 0 && m_token != token) {
            result.success() = false;
            result.error()   = "Invalid security token";
            req.respond(result);
            spdlog::error("[provider:{}] Invalid security token {}", id(),
                          token);
            return;
        }

        {
            std::lock_guard<tl::mutex> lock(m_backends_mtx);

            if (m_backends.count(Service_id) == 0) {
                result.success() = false;
                result.error()
                    = "Service "s + Service_id.to_string() + " not found";
                req.respond(result);
                spdlog::error("[provider:{}] Service {} not found", id(),
                              Service_id.to_string());
                return;
            }

            m_backends.erase(Service_id);
        }
        req.respond(result);
        spdlog::trace("[provider:{}] Service {} successfully closed", id(),
                      Service_id.to_string());
    }

    void destroyService(const tl::request& req, const std::string& token,
                        const UUID& Service_id) {
        RequestResult<bool> result;
        spdlog::trace(
            "[provider:{}] Received destroyService request for Service {}",
            id(), Service_id.to_string());

        if (m_token.size() > 0 && m_token != token) {
            result.success() = false;
            result.error()   = "Invalid security token";
            req.respond(result);
            spdlog::error("[provider:{}] Invalid security token {}", id(),
                          token);
            return;
        }

        {
            std::lock_guard<tl::mutex> lock(m_backends_mtx);

            if (m_backends.count(Service_id) == 0) {
                result.success() = false;
                result.error()
                    = "Service "s + Service_id.to_string() + " not found";
                req.respond(result);
                spdlog::error("[provider:{}] Service {} not found", id(),
                              Service_id.to_string());
                return;
            }

            result = m_backends[Service_id]->destroy();
            m_backends.erase(Service_id);
        }

        req.respond(result);
        spdlog::trace("[provider:{}] Service {} successfully destroyed", id(),
                      Service_id.to_string());
    }

    void checkService(const tl::request& req, const UUID& Service_id) {
        spdlog::trace(
            "[provider:{}] Received checkService request for Service {}", id(),
            Service_id.to_string());
        RequestResult<bool> result;
        FIND_SERVICE(Service);
        result.success() = true;
        req.respond(result);
        spdlog::trace("[provider:{}] Code successfully executed on Service {}",
                      id(), Service_id.to_string());
    }

    void sayHello(const tl::request& req, const UUID& Service_id) {
        spdlog::trace("[provider:{}] Received sayHello request for Service {}",
                      id(), Service_id.to_string());
        RequestResult<bool> result;
        FIND_SERVICE(Service);
        Service->sayHello();
        spdlog::trace(
            "[provider:{}] Successfully executed sayHello on Service {}", id(),
            Service_id.to_string());
    }

    void computeSum(const tl::request& req, const UUID& Service_id, int32_t x,
                    int32_t y) {
        spdlog::trace("[provider:{}] Received sayHello request for Service {}",
                      id(), Service_id.to_string());
        RequestResult<int32_t> result;
        FIND_SERVICE(Service);
        result = Service->computeSum(x, y);
        req.respond(result);
        spdlog::trace(
            "[provider:{}] Successfully executed computeSum on Service {}",
            id(), Service_id.to_string());
    }
};

} // namespace bedrock

#endif
