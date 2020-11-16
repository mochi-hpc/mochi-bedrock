/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_ADMIN_IMPL_H
#define __BEDROCK_ADMIN_IMPL_H

#include <thallium.hpp>

namespace bedrock {

namespace tl = thallium;

class AdminImpl {

  public:
    tl::engine           m_engine;
    tl::remote_procedure m_create_Service;
    tl::remote_procedure m_open_Service;
    tl::remote_procedure m_close_Service;
    tl::remote_procedure m_destroy_Service;

    AdminImpl(const tl::engine& engine)
    : m_engine(engine),
      m_create_Service(m_engine.define("bedrock_create_Service")),
      m_open_Service(m_engine.define("bedrock_open_Service")),
      m_close_Service(m_engine.define("bedrock_close_Service")),
      m_destroy_Service(m_engine.define("bedrock_destroy_Service")) {}

    AdminImpl(margo_instance_id mid) : AdminImpl(tl::engine(mid)) {}

    ~AdminImpl() {}
};

} // namespace bedrock

#endif
