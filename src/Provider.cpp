/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/Provider.hpp"

#include "ProviderImpl.hpp"

#include <thallium/serialization/stl/string.hpp>

namespace tl = thallium;

namespace bedrock {

Provider::Provider(const tl::engine& engine, uint16_t provider_id,
                   const tl::pool& p)
: self(std::make_shared<ProviderImpl>(engine, provider_id, p)) {
    self->get_engine().push_finalize_callback(
        this, [p = this]() { p->self.reset(); });
}

Provider::Provider(margo_instance_id mid, uint16_t provider_id,
                   const tl::pool& p)
: self(std::make_shared<ProviderImpl>(mid, provider_id, p)) {
    self->get_engine().push_finalize_callback(
        this, [p = this]() { p->self.reset(); });
}

Provider::Provider(Provider&& other) {
    other.self->get_engine().pop_finalize_callback(this);
    self = std::move(other.self);
    self->get_engine().push_finalize_callback(
        this, [p = this]() { p->self.reset(); });
}

Provider::~Provider() {
    if (self) { self->get_engine().pop_finalize_callback(this); }
}

Provider::operator bool() const { return static_cast<bool>(self); }

void Provider::setSecurityToken(const std::string& token) {
    if (self) self->m_token = token;
}

} // namespace bedrock
