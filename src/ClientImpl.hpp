/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_CLIENT_IMPL_H
#define __BEDROCK_CLIENT_IMPL_H

#include <thallium.hpp>
#include <thallium/serialization/stl/string.hpp>

namespace bedrock {

namespace tl = thallium;

class ClientImpl {

  public:
    tl::engine           m_engine;
    tl::remote_procedure m_get_config;
    tl::remote_procedure m_load_module;
    tl::remote_procedure m_start_provider;

    ClientImpl(const tl::engine& engine)
    : m_engine(engine), m_get_config(m_engine.define("bedrock_get_config")),
      m_load_module(m_engine.define("bedrock_load_module")),
      m_start_provider(m_engine.define("bedrock_start_provider")) {}

    ClientImpl(margo_instance_id mid) : ClientImpl(tl::engine(mid)) {}

    ~ClientImpl() {}
};

} // namespace bedrock

#endif
