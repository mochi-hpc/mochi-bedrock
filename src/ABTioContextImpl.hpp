/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_ABTIO_CONTEXT_IMPL_H
#define __BEDROCK_ABTIO_CONTEXT_IMPL_H

#include <nlohmann/json.hpp>
#include <abt-io.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace bedrock {

using nlohmann::json;

class ABTioContextImpl {

  public:
    std::vector<std::pair<std::string, abt_io_instance_id>> m_abt_io_ids;
    json                                                    m_json_config;
};

} // namespace bedrock

#endif
