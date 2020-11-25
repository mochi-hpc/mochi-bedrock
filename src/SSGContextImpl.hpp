/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_SSG_CONTEXT_IMPL_H
#define __BEDROCK_SSG_CONTEXT_IMPL_H

#include "MargoContextImpl.hpp"
#include <ssg.h>
#include <memory>
#include <unordered_map>
#include <string>

namespace bedrock {

class SSGData {
  public:
    ssg_group_id_t                m_gid;
    std::weak_ptr<SSGContextImpl> m_context;
};

class SSGContextImpl {

    using group_name_t = std::string;

  public:
    std::shared_ptr<MargoContextImpl>                          m_margo_context;
    std::unordered_map<group_name_t, std::unique_ptr<SSGData>> m_ssg_groups;
};

} // namespace bedrock

#endif
