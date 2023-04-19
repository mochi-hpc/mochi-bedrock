/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/Exception.hpp"
#include "bedrock/AsyncRequest.hpp"
#include "AsyncRequestImpl.hpp"

namespace bedrock {

AsyncRequest::AsyncRequest() = default;

AsyncRequest::AsyncRequest(const std::shared_ptr<AsyncRequestImpl>& impl)
: self(impl) {}

AsyncRequest::AsyncRequest(const AsyncRequest& other) = default;

AsyncRequest::AsyncRequest(AsyncRequest&& other) {
    self       = std::move(other.self);
    other.self = nullptr;
}

AsyncRequest::~AsyncRequest() {
    if (self && self.unique()) { wait(); }
}

AsyncRequest& AsyncRequest::operator=(const AsyncRequest& other) {
    if (this == &other || self == other.self) return *this;
    if (self && self.unique()) { wait(); }
    self = other.self;
    return *this;
}

AsyncRequest& AsyncRequest::operator=(AsyncRequest&& other) {
    if (this == &other || self == other.self) return *this;
    if (self && self.unique()) { wait(); }
    self       = std::move(other.self);
    other.self = nullptr;
    return *this;
}

void AsyncRequest::wait() const {
    if (not self) throw Exception("Invalid bedrock::AsyncRequest object");
    self->wait();
}

bool AsyncRequest::completed() const {
    if (not self) throw Exception("Invalid bedrock::AsyncRequest object");
    return self->completed();
}

bool AsyncRequest::active() const {
    if (not self) return false;
    return self->active();
}

} // namespace bedrock
