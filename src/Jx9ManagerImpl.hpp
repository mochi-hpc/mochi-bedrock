/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_JX9_MANAGER_IMPL_H
#define __BEDROCK_JX9_MANAGER_IMPL_H

#include "jx9/jx9.h"
#include <thallium.hpp>
#include <spdlog/spdlog.h>
#include <memory>
#include <vector>
#include <string>

namespace bedrock {

namespace tl = thallium;

class Jx9ManagerImpl {

  public:

    jx9*      m_engine = nullptr;
    tl::mutex m_mtx;

    Jx9ManagerImpl() {
        jx9_init(&m_engine);
    }

    ~Jx9ManagerImpl() {
        jx9_release(m_engine);
    }

    Jx9ManagerImpl(const Jx9Manager&) = delete;
    Jx9ManagerImpl(Jx9Manager&&) = delete;
    Jx9ManagerImpl& operator=(const Jx9Manager&) = delete;
    Jx9ManagerImpl& operator=(Jx9Manager&&) = delete;
};

} // namespace bedrock

#endif
