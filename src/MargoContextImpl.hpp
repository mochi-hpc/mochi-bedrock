/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_MARGO_CONTEXT_IMPL_H
#define __BEDROCK_MARGO_CONTEXT_IMPL_H

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace bedrock {

using namespace std::string_literals;
namespace tl = thallium;

class MargoContextImpl {

  public:
    margo_instance_id m_mid;
};

} // namespace bedrock

#endif
