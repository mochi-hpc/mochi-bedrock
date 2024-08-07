/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_SSG_CONTEXT_IMPL_H
#define __BEDROCK_SSG_CONTEXT_IMPL_H

#include <bedrock/MargoManager.hpp>
#include <bedrock/SSGManager.hpp>
#include <bedrock/Jx9Manager.hpp>
#include <bedrock/NamedDependency.hpp>
#include <bedrock/DetailedException.hpp>
#include "MargoManagerImpl.hpp"
#include <spdlog/spdlog.h>
#include <memory>
#include <vector>
#include <string>

#ifdef ENABLE_SSG
#include <ssg.h>

namespace std {

static inline auto to_string(ssg_member_update_type_t t) -> std::string {
    switch(t) {
        case SSG_MEMBER_JOINED: return "SSG_MEMBER_JOINED";
        case SSG_MEMBER_LEFT:   return "SSG_MEMBER_LEFT";
        case SSG_MEMBER_DIED:   return "SSG_MEMBER_DIED";
        default: return "UNKNOWN";
    }
}

}

#endif

namespace bedrock {

class SSGManagerImpl;

class SSGEntry : public NamedDependency {

    public:

#ifdef ENABLE_SSG
    ssg_group_config_t               config;
    std::string                      bootstrap;
    std::string                      group_file;
    std::shared_ptr<NamedDependency> pool;
    std::weak_ptr<SSGManagerImpl>    owner;

    uint64_t                         rank; // current rank of the process in this group

    SSGEntry(std::shared_ptr<SSGManagerImpl> o, std::string name, std::shared_ptr<NamedDependency> p)
    : NamedDependency(
        std::move(name),
        "ssg", SSG_GROUP_ID_INVALID,
        std::function<void(void*)>())
    , pool{std::move(p)}
    , owner(o)
    {}

    SSGEntry(const SSGEntry&) = delete;
    SSGEntry(SSGEntry&&) = delete;

    void setSSGid(ssg_group_id_t gid) {
        m_handle = reinterpret_cast<void*>(gid);
    }

    ssg_group_id_t getSSGid() const {
        return reinterpret_cast<ssg_group_id_t>(m_handle);
    }

    json makeConfig() const {
        json c          = json::object();
        c["name"]       = getName();
        c["bootstrap"]  = bootstrap;
        c["group_file"] = group_file;
        c["pool"]       = pool->getName();
        c["credential"] = config.ssg_credential;
        if(!config.swim_disabled) {
            c["swim"]       = json::object();
            auto& swim      = c["swim"];
            swim["period_length_ms"]        = config.swim_period_length_ms;
            swim["suspect_timeout_periods"] = config.swim_suspect_timeout_periods;
            swim["subgroup_member_count"]   = config.swim_subgroup_member_count;
        }
        return c;
    }

#else

    json makeConfig() const {
        return json::object();
    }

#endif
};

class SSGManagerImpl {

  public:
    std::shared_ptr<MargoManagerImpl>         m_margo_manager;
    std::shared_ptr<Jx9ManagerImpl>           m_jx9_manager;
    std::vector<std::shared_ptr<SSGEntry>>    m_ssg_groups;

    json makeConfig() const {
        auto config = json::array();
        for (const auto& g : m_ssg_groups) {
            config.push_back(g->makeConfig());
        }
        return config;
    }

#ifdef ENABLE_SSG
    SSGManagerImpl() {
        if (SSGManagerImpl::s_num_ssg_init == 0) {
            int ret = ssg_init();
            if (ret != SSG_SUCCESS) {
                throw BEDROCK_DETAILED_EXCEPTION("Could not initialize SSG (ssg_init returned {})",
                        ret);
            }
        }
        SSGManagerImpl::s_num_ssg_init += 1;
    }

    static void releaseSSGid(ssg_group_id_t gid) {
        if (!gid) return;
        int ret = ssg_group_leave(gid);
        // if SWIM is disabled, this function will return SSG_ERR_NOT_SUPPORTED
        if (ret != SSG_SUCCESS && ret != SSG_ERR_NOT_SUPPORTED) {
            spdlog::error(
                    "Could not leave SSG group (ssg_group_leave returned {})",
                    ret);
        }
        ret = ssg_group_destroy(gid);
        if (ret != SSG_SUCCESS) {
            spdlog::error(
                    "Could not destroy SSG group (ssg_group_destroy returned {})",
                    ret);
        }
    }

    ~SSGManagerImpl() {
        spdlog::trace("Destroying SSGManager (count = {})", s_num_ssg_init);
        m_ssg_groups.clear();
        s_num_ssg_init -= 1;
        if (s_num_ssg_init == 0) {
            spdlog::trace("Finalizing SSG");
            ssg_finalize();
        }
    }

    void updateJx9Ranks() {
        json rankMap = json::object();
        for(auto& g : m_ssg_groups) {
            auto& x = rankMap[g->getName()] = json::object();
            x["rank"] = g->rank;
        }
        Jx9Manager(m_jx9_manager).setVariable(
            "__ssg__", rankMap.dump());
    }
#endif

    void clear() {
#ifdef ENABLE_SSG
        for(auto& g : m_ssg_groups) {
            releaseSSGid(g->getHandle<ssg_group_id_t>());
        }
        m_ssg_groups.resize(0);
#endif
    }


    static int s_num_ssg_init;
#ifdef ENABLE_PMIX
    static bool s_initialized_pmix;
#endif

};

} // namespace bedrock

#endif
