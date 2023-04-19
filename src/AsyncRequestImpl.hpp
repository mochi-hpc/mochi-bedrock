/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_ASYNC_REQUEST_IMPL_H
#define __BEDROCK_ASYNC_REQUEST_IMPL_H

#include <functional>
#include <thallium.hpp>
#include "bedrock/Exception.hpp"

namespace bedrock {

namespace tl = thallium;

struct AsyncRequestImpl {

    virtual ~AsyncRequestImpl() = default;

    virtual void wait() = 0;

    virtual bool completed() const = 0;

    virtual bool active() const = 0;
};

struct AsyncThalliumResponse : public AsyncRequestImpl {

    AsyncThalliumResponse(tl::async_response&& async_response)
    : m_async_response(std::move(async_response)) {}

    tl::async_response                          m_async_response;
    bool                                        m_waited = false;
    std::function<void(AsyncThalliumResponse&)> m_wait_callback;

    void wait() override {
        if (m_waited) return;
            m_waited = true;
        m_wait_callback(*this);
    }

    bool completed() const override {
        return m_async_response.received();
    }

    bool active() const override {
        return !m_waited;
    }
};

struct MultiAsyncRequest : public AsyncRequestImpl {

    std::vector<std::shared_ptr<AsyncRequestImpl>> m_reqs;
    std::function<void(MultiAsyncRequest&)> m_wait_callback;

    MultiAsyncRequest(std::vector<std::shared_ptr<AsyncRequestImpl>> reqs)
    : m_reqs(std::move(reqs)) {}

    void wait() override {
        Exception first_exception{""};
        bool ok = true;
        for(auto& r : m_reqs) {
            try {
                r->wait();
            } catch(const Exception& ex) {
                if(!ok) continue;
                ok = false;
                first_exception = std::move(ex);
            }
        }
        m_reqs.clear();
        if(!ok) {
            m_wait_callback = std::function<void(MultiAsyncRequest&)>{};
            throw first_exception;
        }
        if(m_wait_callback) m_wait_callback(*this);
        m_wait_callback = std::function<void(MultiAsyncRequest&)>{};
    }

    bool completed() const override {
        for(auto& r : m_reqs) {
            if(!r->completed()) return false;
        }
        return true;
    }

    bool active() const override {
        return !m_reqs.empty();
    }
};

} // namespace bedrock

#endif
