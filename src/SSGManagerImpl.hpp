/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_SSG_CONTEXT_IMPL_H
#define __BEDROCK_SSG_CONTEXT_IMPL_H

#include "bedrock/MargoManager.hpp"
#include "MargoManagerImpl.hpp"
#include <spdlog/spdlog.h>
#include <ssg.h>
#include <memory>
#include <vector>
#include <string>

namespace bedrock {

class SSGManagerImpl;

class SSGData {
  public:
    std::string                       name;
    ssg_group_config_t                config;
    std::string                       bootstrap;
    std::string                       group_file;
    ABT_pool                          pool;
    std::shared_ptr<MargoManagerImpl> margo_ctx;

    ssg_group_id_t gid = SSG_GROUP_ID_INVALID;

    json makeConfig() const {
        json c          = json::object();
        c["name"]       = name;
        c["bootstrap"]  = bootstrap;
        c["group_file"] = group_file;
        c["pool"]       = MargoManager(margo_ctx).getPoolInfo(pool).first;
        c["credential"] = config.ssg_credential;
        c["swim"]       = json::object();
        auto& swim      = c["swim"];
        swim["period_length_ms"]        = config.swim_period_length_ms;
        swim["suspect_timeout_periods"] = config.swim_suspect_timeout_periods;
        swim["subgroup_member_count"]   = config.swim_subgroup_member_count;
        swim["disabled"]                = config.swim_disabled ? true : false;
        return c;
    }

    ~SSGData() {
        spdlog::trace("Leaving and destroying SSG group {}", name);
        if (gid) {
            int ret = ssg_group_leave(gid);
            if (ret != SSG_SUCCESS) {
                spdlog::error(
                    "Could not leave SSG group \"{}\" "
                    "(ssg_group_leave returned {})",
                    name, ret);
            }
            ret = ssg_group_destroy(gid);
            if (ret != SSG_SUCCESS) {
                spdlog::error(
                    "Could not destroy SSG group \"{}\" "
                    "(ssg_group_destroy returned {})",
                    name, ret);
            }
        }
    }
};

class SSGManagerImpl {

  public:
    std::shared_ptr<MargoManagerImpl>     m_margo_manager;
    std::vector<std::unique_ptr<SSGData>> m_ssg_groups;
    // note: we need a vector of unique_ptr because
    // membership callbacks are being passed the internal pointer

    json makeConfig() const {
        auto config = json::array();
        for (const auto& g : m_ssg_groups) {
            config.push_back(g->makeConfig());
        }
        return config;
    }

    void clear() {
        m_ssg_groups.resize(0);
    }

    ~SSGManagerImpl() {
        spdlog::trace("Destroying SSGManager");
        s_num_ssg_init -= 1;
        if (s_num_ssg_init == 0) {
            spdlog::trace("Finalizing SSG");
            ssg_finalize();
        }
    }

    static int s_num_ssg_init;
#ifdef ENABLE_MPI
    static bool s_initialized_mpi;
#endif
#ifdef ENABLE_PMIX
    static bool s_initialized_pmix;
#endif

};

} // namespace bedrock

#endif
