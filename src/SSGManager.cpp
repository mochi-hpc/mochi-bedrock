/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/SSGManager.hpp"
#include "bedrock/MargoManager.hpp"
#include "JsonUtil.hpp"
#include "Exception.hpp"
#include "SSGManagerImpl.hpp"
#include <margo.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#ifdef ENABLE_SSG
  #include <ssg.h>
  #ifdef ENABLE_MPI
    #include <ssg-mpi.h>
  #endif
  #ifdef ENABLE_PMIX
    #include <ssg-pmix.h>
  #endif
#endif
#include <regex>
#include <fstream>

namespace bedrock {

using nlohmann::json;
using namespace std::string_literals;

int SSGManagerImpl::s_num_ssg_init = 0;
#ifdef ENABLE_MPI
bool SSGManagerImpl::s_initialized_mpi = false;
#endif
#ifdef ENABLE_PMIX
bool SSGManagerImpl::s_initialized_pmix = false;
#endif

class SSGUpdateHandler {
#ifdef ENABLE_SSG
    public:
    static void membershipUpdate(void* group_data, ssg_member_id_t member_id,
                                 ssg_member_update_type_t update_type);
#endif
};

SSGManager::SSGManager(const MargoManager& margo,
                       const Jx9Manager& jx9,
                       const json&  config)
: self(std::make_shared<SSGManagerImpl>()) {
    self->m_margo_manager = margo;
    self->m_jx9_manager   = jx9;

    if(config.is_null()) return;
    if(!(config.is_array() || config.is_object()))
        throw DETAILED_EXCEPTION("\"ssg\" field must be an object or an array");

#ifndef ENABLE_SSG
    if (!config.empty()) {
        throw DETAILED_EXCEPTION(
            "Configuration has \"ssg\" entry but Bedrock wasn't compiled with SSG support");
    }
#else
    if (config.is_object()) {
        addGroupFromJSON(config);
    } else if (config.is_array()) {
        for (const auto& c : config) {
            addGroupFromJSON(c);
        }
    }

    self->updateJx9Ranks();
#endif
}

// LCOV_EXCL_START

SSGManager::SSGManager(const SSGManager&) = default;

SSGManager::SSGManager(SSGManager&&) = default;

SSGManager& SSGManager::operator=(const SSGManager&) = default;

SSGManager& SSGManager::operator=(SSGManager&&) = default;

SSGManager::~SSGManager() = default;

SSGManager::operator bool() const { return static_cast<bool>(self); }

// LCOV_EXCL_STOP

std::shared_ptr<NamedDependency> SSGManager::getGroup(const std::string& group_name) const {
#ifdef ENABLE_SSG
    auto it = std::find_if(self->m_ssg_groups.begin(), self->m_ssg_groups.end(),
                           [&](auto& g) { return g->getName() == group_name; });
    if (it == self->m_ssg_groups.end()) {
        throw DETAILED_EXCEPTION("Could not find SSG group with name \"{}\"", group_name);
        return nullptr;
    } else {
        return *it;
    }
#else
    (void)group_name;
    throw DETAILED_EXCEPTION("Bedrock was not compiler with SSG support");
    return nullptr;
#endif
}

std::shared_ptr<NamedDependency> SSGManager::getGroup(size_t group_index) const {
#ifdef ENABLE_SSG
    if(group_index >= self->m_ssg_groups.size()) {
        throw DETAILED_EXCEPTION("Could not find SSG group at index {}", group_index);
        return nullptr;
    }
    return self->m_ssg_groups[group_index];
#else
    (void)group_index;
    throw DETAILED_EXCEPTION("Bedrock was not compiler with SSG support");
    return nullptr;
#endif
}

size_t SSGManager::getNumGroups() const {
#ifdef ENABLE_SSG
    return self->m_ssg_groups.size();
#else
    return 0;
#endif
}

std::shared_ptr<NamedDependency>
SSGManager::addGroup(const std::string&                      name,
                     const ssg_group_config_t&               config,
                     const std::shared_ptr<NamedDependency>& pool,
                     const std::string&                      bootstrap,
                     const std::string&                      group_file) {
#ifndef ENABLE_SSG
    (void)name;
    (void)config;
    (void)pool;
    (void)bootstrap_method;
    (void)group_file;
    throw DETAILED_EXCEPTION("Bedrock wasn't compiled with SSG support");
    return nullptr;
#else // ENABLE_SSG
    int            ret;
    ssg_group_id_t gid = SSG_GROUP_ID_INVALID;
    auto           margo = MargoManager(self->m_margo_manager);
    auto           mid = margo.getMargoInstance();

    auto it = std::find_if(self->m_ssg_groups.begin(), self->m_ssg_groups.end(),
                           [&](auto& g) { return g->getName() == name; });
    if (it != self->m_ssg_groups.end()) {
        throw DETAILED_EXCEPTION("SSG group name \"{}\" already used", name);
    }

    if (SSGManagerImpl::s_num_ssg_init == 0) {
        int ret = ssg_init();
        if (ret != SSG_SUCCESS) {
            throw DETAILED_EXCEPTION(
                "Could not initialize SSG (ssg_init returned {})", ret);
        }
    }
    SSGManagerImpl::s_num_ssg_init += 1;

    // The inner data of the ssg_entry will be set later.
    // The ssg_entry needs to be created here because the
    // membership callback needs it.
    auto ssg_entry = std::make_shared<SSGEntry>(self, name, pool);

    spdlog::trace("Creating SSG group {} with bootstrap method {}", name,
                  bootstrap);

    (void)pool; // TODO add support for specifying the pool

    auto method = bootstrap;

    if (method == "init|join"
    ||  method == "mpi|join"
    ||  method == "pmix|join") {
        auto sep = method.find('|');
        if(group_file.empty()) {
            auto new_method = method.substr(0, sep);
            spdlog::trace("Group file not specified, changing bootstrap method from \"{}\" to \"{}\"",
                method, new_method);
            method = std::move(new_method);
        } else {
            std::ifstream f(group_file.c_str());
            if(f.good()) {
                auto new_method = method.substr(sep+1);
                spdlog::trace("File {} found, changing bootstrap method from \"{}\" to \"{}\"",
                    group_file, method, new_method);
                method = std::move(new_method);
            } else {
                auto new_method = method.substr(0, sep);
                spdlog::trace("File {} not found, changing bootstrap method from \"{}\" to \"{}\"",
                    group_file, method, new_method);
                method = std::move(new_method);
            }
        }
    }

    if (method == "init") {

        std::string addr_str = margo.getThalliumEngine().self();
        std::vector<const char*> addresses = {addr_str.c_str()};

        ret = ssg_group_create(mid, name.c_str(), addresses.data(), 1,
                               const_cast<ssg_group_config_t*>(&config),
                               SSGUpdateHandler::membershipUpdate, ssg_entry.get(),
                               &gid);
        if (ret != SSG_SUCCESS) {
            throw DETAILED_EXCEPTION("ssg_group_create failed with error code {}", ret);
        }

    } else if (method == "join") {

        if (group_file.empty()) {
            throw DETAILED_EXCEPTION(
                "SSG \"join\" bootstrapping mode requires a group file to be "
                "specified");
        }
        int num_addrs = 32; // XXX make that configurable?
        ret           = ssg_group_id_load(group_file.c_str(), &num_addrs, &gid);
        if (ret != SSG_SUCCESS) {
            throw DETAILED_EXCEPTION(
                "Failed to load SSG group from file {}"
                " (ssg_group_id_load returned {}",
                group_file, ret);
        }
        ret = ssg_group_join(mid, gid, SSGUpdateHandler::membershipUpdate, ssg_entry.get());
        if (ret != SSG_SUCCESS) {
            throw DETAILED_EXCEPTION(
                "Failed to join SSG group {} "
                "(ssg_group_join returned {})",
                name, ret);
        }

    } else if (method == "mpi") {
#ifdef ENABLE_MPI
        int flag;
        MPI_Initialized(&flag);
        if (!flag) {
            MPI_Init(NULL, NULL);
            SSGManagerImpl::s_initialized_mpi = true;
        }
        ret = ssg_group_create_mpi(
            mid, name.c_str(), MPI_COMM_WORLD,
            const_cast<ssg_group_config_t*>(&config),
            SSGUpdateHandler::membershipUpdate, ssg_entry.get(), &gid);
        if (ret != SSG_SUCCESS) {
            throw DETAILED_EXCEPTION(
                "Failed to create SSG group {} "
                "(ssg_group_create_mpi returned {})",
                name, ret);
        }
#else // ENABLE_MPI
        throw DETAILED_EXCEPTION("Bedrock was not compiled with MPI support");
#endif // ENABLE_MPI

    } else if (method == "pmix") {
#ifdef ENABLE_PMIX
        pmix_proc_t proc;
        auto        ret = PMIx_Init(&proc, NULL, 0);
        if (ret != PMIX_SUCCESS) {
            throw DETAILED_EXCEPTION("Could not initialize PMIx (PMIx_Init returned {})",
                            ret);
        }
        gid = ssg_group_create_pmix(
            mid, name.c_str(), proc,
            const_cast<ssg_group_config_t*>(config),
            SSGUpdateHandler::membershipUpdate, group_data.get());
#else // ENABLE_PMIX
        throw DETAILED_EXCEPTION("Bedrock was not compiled with PMIx support");
#endif // ENABLE_PMIX
    } else {
        throw DETAILED_EXCEPTION("Invalid SSG bootstrapping method {}", bootstrap);
    }
    int rank = -1;
    ret      = ssg_get_group_self_rank(gid, &rank);
    if (rank == -1 || ret != SSG_SUCCESS) {
        throw DETAILED_EXCEPTION("Could not get SSG group rank from group {}", name);
    }
    if (!group_file.empty() && (rank == 0)
        && (method == "init" || method == "mpi"
            || method == "pmix")) {
        ret = ssg_group_id_store(group_file.c_str(), gid, SSG_ALL_MEMBERS);
        if (ret != SSG_SUCCESS) {
            throw DETAILED_EXCEPTION(
                    "Could not write SSG group file {}"
                    " (ssg_group_id_store return {}",
                    group_file, ret);
        }
    }
    ssg_entry->setSSGid(gid);
    ssg_entry->config        = config;
    ssg_entry->bootstrap     = bootstrap;
    ssg_entry->group_file    = group_file;
    ssg_entry->pool          = pool;
    ssg_entry->rank          = rank;
    self->m_ssg_groups.push_back(std::move(ssg_entry));

    self->updateJx9Ranks();

    return ssg_entry;
#endif // ENABLE_SSSG
}

std::shared_ptr<NamedDependency>
SSGManager::addGroupFromJSON(const json& description) {
#ifndef ENABLE_SSG
    (void)config;
    throw DETAILED_EXCEPTION("Bedrock wasn't compiled with SSG support");
    return 0;
#else // ENABLE_SSG

    static constexpr const char* configSchema = R"(
    {
        "$schema": "https://json-schema.org/draft/2019-09/schema",
        "type": "object",
        "properties": {
            "name": {"type": "string", "pattern": "^[a-zA-Z_][a-zA-Z0-9_]*$" },
            "pool": {"oneOf": [
                {"type": "string", "pattern": "^[a-zA-Z_][a-zA-Z0-9_]*$" },
                {"type": "integer", "minimum": 0 }
            ]},
            "credential": {"type": "integer"},
            "group_file": {"type": "string"},
            "bootstrap": {"enum": ["init", "mpi", "pmix", "join", "init|join", "mpi|join", "pmix|join"]},
            "swim": {
                "type": "object",
                "properties": {
                    "period_length_ms": {"type": "integer"},
                    "suspect_timeout_periods": {"type": "integer"},
                    "subgroup_member_count": {"type": "integer"},
                    "disabled": {"type": "boolean"}
                }
            }
        },
        "required": ["name"]
    }
    )";
    static const JsonValidator validator{configSchema};
    validator.validate(description, "SSGManager");
    auto name = description["name"].get<std::string>();
    std::shared_ptr<NamedDependency> pool;
    if(description.contains("pool") && description["pool"].is_number()) {
        pool = MargoManager(self->m_margo_manager)
            .getPool(description["pool"].get<uint32_t>());
    } else {
        pool = MargoManager(self->m_margo_manager)
            .getPool(description.value("pool", "__primary__"));
    }
    auto bootstrap = description.value("bootstrap", "init");
    auto group_file = description.value("group_file", "");
    ssg_group_config_t config = SSG_GROUP_CONFIG_INITIALIZER;
    if(description.contains("credential"))
        config.ssg_credential = description["credential"].get<int64_t>();
    if(description.contains("swim") && !description["swim"].value("disabled", false)) {
        auto& swim = description["swim"];
        config.swim_disabled                = 0;
        config.swim_period_length_ms        = swim.value("period_length_ms", 0);
        config.swim_suspect_timeout_periods = swim.value("suspect_timeout_periods", -1);
        config.swim_subgroup_member_count   = swim.value("subgroup_member_count", -1);
    } else {
        config.swim_disabled = 1;
    }
    return addGroup(name, config, pool, bootstrap, group_file);
#endif // ENABLE_SSG
}

#ifdef ENABLE_SSG
void SSGUpdateHandler::membershipUpdate(void* group_data, ssg_member_id_t member_id,
                                        ssg_member_update_type_t update_type) {
    SSGEntry* gdata = reinterpret_cast<SSGEntry*>(group_data);
    (void)gdata;
    spdlog::trace("SSG membership updated: member_id={}, update_type={}",
                  member_id, std::to_string(update_type));
    int self_rank;
    ssg_get_group_self_rank(gdata->getHandle<ssg_group_id_t>(), &self_rank);
    gdata->rank = (uint64_t)self_rank;
    auto manager = gdata->owner.lock();
    if(manager) manager->updateJx9Ranks();
}
#endif

hg_addr_t SSGManager::resolveAddress(const std::string& address) const {
#ifndef ENABLE_SSG
    (void)address;
    throw DETAILED_EXCEPTION("Bedrock wasn't compiled with SSG support");
#else // ENABLE_SSG
    std::regex  re("ssg:\\/\\/([a-zA-Z_][a-zA-Z0-9_]*)\\/(#?)([0-9][0-9]*)$");
    std::smatch match;
    if (std::regex_search(address, match, re)) {
        auto group_name        = match.str(1);
        auto is_member_id      = match.str(2) == "#";
        auto member_id_or_rank = atol(match.str(3).c_str());
        auto ssg_entry         = getGroup(group_name);
        if (!ssg_entry) {
            throw DETAILED_EXCEPTION("Could not resolve \"{}\" to a valid SSG group",
                            group_name);
        }
        ssg_member_id_t member_id;
        auto gid = ssg_entry->getHandle<ssg_group_id_t>();
        if (!is_member_id) {
            int ret = ssg_get_group_member_id_from_rank(gid, member_id_or_rank,
                                                        &member_id);
            if (member_id == SSG_MEMBER_ID_INVALID || ret != SSG_SUCCESS) {
                throw DETAILED_EXCEPTION("Invalid rank {} in group {}",
                                member_id_or_rank, group_name);
            }
        } else {
            member_id = member_id_or_rank;
        }
        hg_addr_t addr = HG_ADDR_NULL;
        int       ret  = ssg_get_group_member_addr(gid, member_id, &addr);
        if (addr == HG_ADDR_NULL || ret != HG_SUCCESS) {
            throw DETAILED_EXCEPTION("Invalid member id {} in group {}", member_id,
                            group_name);
        }
        return addr;
    } else {
        throw DETAILED_EXCEPTION("Invalid SSG address specification \"{}\"", address);
    }
    return HG_ADDR_NULL;
#endif // ENABLE_SSG
}

json SSGManager::getCurrentConfig() const {
#ifndef ENABLE_SSG
    return json::array();
#else
    return self->makeConfig();
#endif
}

} // namespace bedrock
