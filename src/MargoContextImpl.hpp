/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_MARGO_CONTEXT_IMPL_H
#define __BEDROCK_MARGO_CONTEXT_IMPL_H

#include <nlohmann/json.hpp>
#include <margo.h>
#include <thallium.hpp>

namespace tl = thallium;

namespace bedrock {

using nlohmann::json;

class MargoContextImpl {

  public:
    margo_instance_id m_mid;
    tl::engine        m_engine;

    json makeConfig() const {
        char* str    = margo_get_config(m_mid);
        auto  config = json::parse(str);
        free(str);
        return config;
    }
};

} // namespace bedrock

#endif
