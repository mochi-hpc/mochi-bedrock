/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/Exception.hpp"
#include "bedrock/Client.hpp"
#include "bedrock/ServiceHandle.hpp"
#include "bedrock/RequestResult.hpp"
#include "bedrock/client.h"

#include "ClientImpl.hpp"
#include "ServiceHandleImpl.hpp"

#include <thallium/serialization/stl/string.hpp>

namespace tl = thallium;

namespace bedrock {

Client::Client() = default;

Client::Client(const tl::engine& engine)
: self(std::make_shared<ClientImpl>(engine)) {}

Client::Client(margo_instance_id mid)
: self(std::make_shared<ClientImpl>(mid)) {}

Client::Client(const std::shared_ptr<ClientImpl>& impl) : self(impl) {}

Client::Client(Client&& other) = default;

Client& Client::operator=(Client&& other) = default;

Client::Client(const Client& other) = default;

Client& Client::operator=(const Client& other) = default;

Client::~Client() = default;

const tl::engine& Client::engine() const { return self->m_engine; }

Client::operator bool() const { return static_cast<bool>(self); }

ServiceHandle Client::makeServiceHandle(const std::string& address,
                                        uint16_t           provider_id) const {
    auto endpoint = self->m_engine.lookup(address);
    auto ph       = tl::provider_handle(endpoint, provider_id);
    auto service_impl
        = std::make_shared<ServiceHandleImpl>(self, std::move(ph));
    return ServiceHandle(service_impl);
}

} // namespace bedrock

extern "C" int bedrock_client_init(margo_instance_id mid, bedrock_client_t* client) {
    *client = static_cast<void*>(new bedrock::Client(mid));
    return 0;
}

extern "C" int bedrock_client_finalize(bedrock_client_t client) {
    delete static_cast<bedrock::Client*>(client);
    return 0;
}
