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
    tl::remote_procedure m_query_config;
    tl::remote_procedure m_load_module;
    tl::remote_procedure m_start_provider;
    tl::remote_procedure m_change_provider_pool;
    tl::remote_procedure m_migrate_provider;
    tl::remote_procedure m_snapshot_provider;
    tl::remote_procedure m_restore_provider;
    tl::remote_procedure m_add_client;
    tl::remote_procedure m_add_abtio;
    tl::remote_procedure m_add_pool;
    tl::remote_procedure m_add_xstream;
    tl::remote_procedure m_remove_pool;
    tl::remote_procedure m_remove_xstream;

    ClientImpl(const tl::engine& engine)
    : m_engine(engine), m_get_config(m_engine.define("bedrock_get_config")),
      m_query_config(m_engine.define("bedrock_query_config")),
      m_load_module(m_engine.define("bedrock_load_module")),
      m_start_provider(m_engine.define("bedrock_start_provider")),
      m_change_provider_pool(m_engine.define("bedrock_change_provider_pool")),
      m_migrate_provider(m_engine.define("bedrock_migrate_provider")),
      m_snapshot_provider(m_engine.define("bedrock_snapshot_provider")),
      m_restore_provider(m_engine.define("bedrock_restore_provider")),
      m_add_client(m_engine.define("bedrock_add_client")),
      m_add_abtio(m_engine.define("bedrock_add_abtio")),
      m_add_pool(m_engine.define("bedrock_add_pool")),
      m_add_xstream(m_engine.define("bedrock_add_xstream")),
      m_remove_pool(m_engine.define("bedrock_remove_pool")),
      m_remove_xstream(m_engine.define("bedrock_remove_xstream"))
    {}

    ClientImpl(margo_instance_id mid) : ClientImpl(tl::engine(mid)) {}

    ~ClientImpl() {}
};

} // namespace bedrock

#endif
