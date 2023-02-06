/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/SSGManager.hpp"
#include "bedrock/Exception.hpp"
#include "bedrock/MargoManager.hpp"
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

#define ASSERT_TYPE(_config, _type, _msg) \
    if (!_config.is_##_type()) { throw Exception(_msg); }

#define ASSERT_TYPE_OR_DEFAULT(_config, _key, _type, _default, _msg) \
    if (!_config.contains(_key)) {                                   \
        _config[_key] = _default;                                    \
    } else if (!_config[_key].is_##_type()) {                        \
        throw Exception(_msg);                                       \
    }

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

#ifdef ENABLE_SSG
static void validateGroupConfig(json&                           config,
                                const std::vector<std::string>& existing_names,
                                const MargoManager&             margo) {
    // validate name field
    auto default_name
        = "__ssg_"s + std::to_string(existing_names.size()) + "__";
    if (!config.contains("name")) { config["name"] = default_name; }
    ASSERT_TYPE(config["name"], string, "SSG group name should be a string");
    auto name = config["name"].get<std::string>();
    if (std::find(existing_names.begin(), existing_names.end(), name)
        != existing_names.end()) {
        throw Exception("An SSG group with name \"{}\" is already defined");
    }
    // validate swim object
    if (config.contains("swim") && !config["swim"].is_object()) {
        throw Exception(
            "\"swim\" entry in SSG group configuration should be an object");
    } else if (!config.contains("swim")) {
        config["swim"] = json::object();
    }
    auto& swim_config = config["swim"];
    // validate period_length_ms
    ASSERT_TYPE_OR_DEFAULT(swim_config, "period_length_ms", number_integer, 0,
                           "\"swim.period_length_ms\" should be an integer");
    // validate suspect_timeout_periods
    ASSERT_TYPE_OR_DEFAULT(
        swim_config, "suspect_timeout_periods", number_integer, -1,
        "\"swim.suspect_timeout_periods\" should be an integer");
    // validate subgroup_member_count
    ASSERT_TYPE_OR_DEFAULT(
        swim_config, "subgroup_member_count", number_integer, -1,
        "\"swim.subgroup_member_count\" should be an integer");
    // validate disabled
    ASSERT_TYPE_OR_DEFAULT(swim_config, "disabled", boolean, false,
                           "\"swim.disabled\" should be a boolean");
    // validate credential
    ASSERT_TYPE_OR_DEFAULT(config, "credential", number_integer, -1,
                           "\"credential\" should be an integer");
    // validate group_file
    ASSERT_TYPE_OR_DEFAULT(config, "group_file", string, "",
                           "\"group_file\" should be a string");
    // validate bootstrap
    if (!config.contains("bootstrap")) config["bootstrap"] = "init";
    ASSERT_TYPE(config["bootstrap"], string,
                "\"bootstrap\" field should be a string");
    auto bootstrap = config["bootstrap"].get<std::string>();
    if (bootstrap != "init" && bootstrap != "join" && bootstrap != "mpi"
        && bootstrap != "pmix" && bootstrap != "init|join"
        && bootstrap != "mpi|join" && bootstrap != "pmix|join") {
        throw Exception(
            "Invalid bootstrap value \"{}\" in SSG group configuration",
            bootstrap);
    }
    if (config.contains("pool")) {
        if (config["pool"].is_string()) {
            auto pool_name = config["pool"].get<std::string>();
            if (!margo.getPool(pool_name)) {
                throw Exception(
                    "Could not find pool \"{}\" in SSG configuration",
                    pool_name);
            }
        } else if (config["pool"].is_number_integer()) {
            auto pool_index = config["pool"].get<int>();
            if (!margo.getPool(pool_index)) {
                throw Exception(
                    "Could not find pool number {} in SSG configuration",
                    pool_index);
            }
        } else {
            throw Exception(
                "Invalid pool type in SSG group configuration (expected int or "
                "string)");
        }
    }
}

static void extractConfigParameters(const json&         config,
                                    const MargoManager& margo,
                                    std::string& name, std::string& bootstrap,
                                    ssg_group_config_t& group_config,
                                    std::string& group_file,
                                    std::shared_ptr<NamedDependency>& pool) {
    name      = config["name"].get<std::string>();
    bootstrap = config["bootstrap"].get<std::string>();
    auto swim = config["swim"];
    group_config.swim_period_length_ms
        = swim["period_length_ms"].get<int32_t>();
    group_config.swim_suspect_timeout_periods
        = swim["suspect_timeout_periods"].get<int32_t>();
    group_config.swim_subgroup_member_count
        = swim["subgroup_member_count"].get<int32_t>();
    group_config.swim_disabled  = swim["disabled"].get<bool>();
    group_config.ssg_credential = config["credential"].get<int64_t>();
    group_file                  = config["group_file"].get<std::string>();
    if (config.contains("pool")) {
        if (config["pool"].is_string()) {
            pool = margo.getPool(config["pool"].get<std::string>());
        } else {
            pool = margo.getPool(config["pool"].get<size_t>());
        }
    }
}
#endif

SSGManager::SSGManager(const MargoManager& margo,
                       const std::string&  configString)
: self(std::make_shared<SSGManagerImpl>()) {
    self->m_margo_manager = margo;
    auto config           = json::parse(configString);

#ifndef ENABLE_SSG
    if (!config.empty()) {
        throw Exception(
            "Configuration has \"ssg\" entry but Bedrock wasn't compiled with SSG support");
    }
#else
    spdlog::trace("Initializing SSG (count = {})", SSGManagerImpl::s_num_ssg_init);
    if (SSGManagerImpl::s_num_ssg_init == 0) {
        int ret = ssg_init();
        if (ret != SSG_SUCCESS) {
            throw Exception("Could not initialize SSG (ssg_init returned {})",
                            ret);
        }
    }
    SSGManagerImpl::s_num_ssg_init += 1;

    auto addGroup = [this, &margo](const auto& config) {
        std::string                      name, bootstrap, group_file;
        ssg_group_config_t               group_config;
        std::shared_ptr<NamedDependency> pool;
        extractConfigParameters(config, margo, name, bootstrap, group_config,
                                group_file, pool);
        createGroup(name, &group_config, pool, bootstrap, group_file);
    };

    std::vector<std::string> existing_names;
    if (config.is_object()) {
        validateGroupConfig(config, existing_names, margo);
        addGroup(config);
    } else if (config.is_array()) {
        for (auto& c : config) {
            validateGroupConfig(c, existing_names, margo);
            existing_names.push_back(c["name"].get<std::string>());
        }
        for (const auto& c : config) { addGroup(c); }
    }
#endif
}

SSGManager::SSGManager(const SSGManager&) = default;

SSGManager::SSGManager(SSGManager&&) = default;

SSGManager& SSGManager::operator=(const SSGManager&) = default;

SSGManager& SSGManager::operator=(SSGManager&&) = default;

SSGManager::~SSGManager() = default;

SSGManager::operator bool() const { return static_cast<bool>(self); }

std::shared_ptr<NamedDependency> SSGManager::getGroup(const std::string& group_name) const {
#ifdef ENABLE_SSG
    auto it = std::find_if(self->m_ssg_groups.begin(), self->m_ssg_groups.end(),
                           [&](auto& g) { return g->getName() == group_name; });
    if (it == self->m_ssg_groups.end())
        return nullptr;
    else
        return *it;
#else
    (void)group_name;
    return nullptr;
#endif
}

std::shared_ptr<NamedDependency>
SSGManager::createGroup(const std::string&                      name,
                        const ssg_group_config_t*               config,
                        const std::shared_ptr<NamedDependency>& pool,
                        const std::string& bootstrap_method,
                        const std::string& group_file) {
#ifndef ENABLE_SSG
    (void)name;
    (void)config;
    (void)pool;
    (void)bootstrap_method;
    (void)group_file;
    throw Exception("Bedrock wasn't compiled with SSG support");
    return nullptr;
#else // ENABLE_SSG
    int            ret;
    ssg_group_id_t gid = SSG_GROUP_ID_INVALID;
    auto           mid = MargoManager(self->m_margo_manager).getMargoInstance();

    // The inner data of the ssg_entry will be set later.
    // The ssg_entry needs to be created here because the
    // membership callback needs it.
    auto ssg_entry = std::make_shared<SSGEntry>(name);

    spdlog::trace("Creating SSG group {} with bootstrap method {}", name,
                  bootstrap_method);

    (void)pool; // TODO add support for specifying the pool

    auto method = bootstrap_method;

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

        hg_addr_t   addr;
        hg_return_t hret = margo_addr_self(mid, &addr);
        if (hret != HG_SUCCESS) {
            throw Exception(
                "Could not get process address "
                "(margo_addr_self returned {})",
                hret);
        }
        std::string addr_str(256, '\0');
        hg_size_t   addr_str_size = 256;
        hret = margo_addr_to_string(mid, const_cast<char*>(addr_str.data()),
                                    &addr_str_size, addr);
        if (hret != HG_SUCCESS) {
            margo_addr_free(mid, addr);
            throw Exception(
                "Could not convert address into string "
                "(margo_addr_to_string returned {}",
                hret);
        }
        addr_str.resize(addr_str_size);
        std::vector<const char*> addresses = {addr_str.c_str()};
        margo_addr_free(mid, addr);

        ret = ssg_group_create(mid, name.c_str(), addresses.data(), 1,
                               const_cast<ssg_group_config_t*>(config),
                               SSGUpdateHandler::membershipUpdate, ssg_entry.get(),
                               &gid);
        if (ret != SSG_SUCCESS) {
            throw Exception("ssg_group_create failed with error code {}", ret);
        }

    } else if (method == "join") {

        if (group_file.empty()) {
            throw Exception(
                "SSG \"join\" bootstrapping mode requires a group file to be "
                "specified");
        }
        int num_addrs = 32; // XXX make that configurable?
        ret           = ssg_group_id_load(group_file.c_str(), &num_addrs, &gid);
        if (ret != SSG_SUCCESS) {
            throw Exception(
                "Failed to load SSG group from file {}"
                " (ssg_group_id_load returned {}",
                group_file, ret);
        }
        ret = ssg_group_join(mid, gid, SSGUpdateHandler::membershipUpdate,
                             ssg_entry.get());
        if (ret != SSG_SUCCESS) {
            throw Exception(
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
            self->m_margo_manager->m_mid, name.c_str(), MPI_COMM_WORLD,
            const_cast<ssg_group_config_t*>(config),
            SSGUpdateHandler::membershipUpdate, ssg_entry.get(), &gid);
        if (ret != SSG_SUCCESS) {
            throw Exception(
                "Failed to create SSG group {} "
                "(ssg_group_create_mpi returned {})",
                name, ret);
        }
#else // ENABLE_MPI
        throw Exception("Bedrock was not compiled with MPI support");
#endif // ENABLE_MPI

    } else if (method == "pmix") {
#ifdef ENABLE_PMIX
        pmix_proc_t proc;
        auto        ret = PMIx_Init(&proc, NULL, 0);
        if (ret != PMIX_SUCCESS) {
            throw Exception("Could not initialize PMIx (PMIx_Init returned {})",
                            ret);
        }
        gid = ssg_group_create_pmix(
            self->m_margo_manager->m_mid, name.c_str(), proc,
            const_cast<ssg_group_config_t*>(config),
            SSGUpdateHandler::membershipUpdate, group_data.get());
#else // ENABLE_PMIX
        throw Exception("Bedrock was not compiled with PMIx support");
#endif // ENABLE_PMIX
    } else {
        throw Exception("Invalid SSG bootstrapping method {}",
                        bootstrap_method);
    }
    if (!group_file.empty()
        && (method == "init" || method == "mpi"
            || method == "pmix")) {
        int rank = -1;
        ret      = ssg_get_group_self_rank(gid, &rank);
        if (rank == -1 || ret != SSG_SUCCESS) {
            throw Exception("Could not get SSG group rank from group {}", name);
        }
        if (rank == 0) {
            int ret
                = ssg_group_id_store(group_file.c_str(), gid, SSG_ALL_MEMBERS);
            if (ret != SSG_SUCCESS) {
                throw Exception(
                    "Could not write SSG group file {}"
                    " (ssg_group_id_store return {}",
                    group_file, ret);
            }
        }
    }
    ssg_entry->setSSGid(gid);
    ssg_entry->config        = *config;
    ssg_entry->bootstrap     = bootstrap_method;
    ssg_entry->group_file    = group_file;
    ssg_entry->pool          = pool;
    self->m_ssg_groups.push_back(std::move(ssg_entry));
    return ssg_entry;
#endif // ENABLE_SSSG
}

std::shared_ptr<NamedDependency>
SSGManager::createGroupFromConfig(const std::string& configString) {
#ifndef ENABLE_SSG
    (void)configString;
    throw Exception("Bedrock wasn't compiled with SSG support");
    return 0;
#else // ENABLE_SSG
    auto config = json::parse(configString);
    if (!config.is_object()) {
        throw Exception("SSG group config should be an object");
    }

    std::vector<std::string> existing_names;
    for (const auto& g : self->m_ssg_groups) {
        existing_names.push_back(g->getName());
    }

    auto margo = MargoManager(self->m_margo_manager);
    validateGroupConfig(config, existing_names, margo);

    if (SSGManagerImpl::s_num_ssg_init == 0) {
        int ret = ssg_init();
        if (ret != SSG_SUCCESS) {
            throw Exception("Could not initialize SSG (ssg_init returned {})",
                            ret);
        }
    }
    SSGManagerImpl::s_num_ssg_init += 1;

    std::string                      name, bootstrap, group_file;
    ssg_group_config_t               group_config;
    std::shared_ptr<NamedDependency> pool;
    extractConfigParameters(config, margo, name, bootstrap, group_config,
                            group_file, pool);
    return createGroup(name, &group_config, pool, bootstrap, group_file);
#endif // ENABLE_SSG
}

#ifdef ENABLE_SSG
void SSGUpdateHandler::membershipUpdate(void* group_data, ssg_member_id_t member_id,
                                  ssg_member_update_type_t update_type) {
    SSGEntry* gdata = reinterpret_cast<SSGEntry*>(group_data);
    (void)gdata;
    spdlog::trace("SSG membership updated: member_id={}, update_type={}",
                  member_id, update_type);
}
#endif

hg_addr_t SSGManager::resolveAddress(const std::string& address) const {
#ifndef ENABLE_SSG
    (void)address;
    throw Exception("Bedrock wasn't compiled with SSG support");
    return HG_ADDR_NULL;
#else // ENABLE_SSG
    std::regex  re("ssg:\\/\\/([a-zA-Z_][a-zA-Z0-9_]*)\\/(#?)([0-9][0-9]*)$");
    std::smatch match;
    if (std::regex_search(address, match, re)) {
        auto group_name        = match.str(1);
        auto is_member_id      = match.str(2) == "#";
        auto member_id_or_rank = atol(match.str(3).c_str());
        auto ssg_entry         = getGroup(group_name);
        if (!ssg_entry) {
            throw Exception("Could not resolve \"{}\" to a valid SSG group",
                            group_name);
        }
        ssg_member_id_t member_id;
        auto gid = ssg_entry->getHandle<ssg_group_id_t>();
        if (!is_member_id) {
            int ret = ssg_get_group_member_id_from_rank(gid, member_id_or_rank,
                                                        &member_id);
            if (member_id == SSG_MEMBER_ID_INVALID || ret != SSG_SUCCESS) {
                throw Exception("Invalid rank {} in group {}",
                                member_id_or_rank, group_name);
            }
        } else {
            member_id = member_id_or_rank;
        }
        hg_addr_t addr = HG_ADDR_NULL;
        int       ret  = ssg_get_group_member_addr(gid, member_id, &addr);
        if (addr == HG_ADDR_NULL || ret != HG_SUCCESS) {
            throw Exception("Invalid member id {} in group {}", member_id,
                            group_name);
        }
        return addr;
    } else {
        throw Exception("Invalid SSG address specification \"{}\"", address);
    }
    return HG_ADDR_NULL;
#endif // ENABLE_SSG
}

std::string SSGManager::getCurrentConfig() const {
#ifndef ENABLE_SSG
    return "[]";
#else
    return self->makeConfig().dump();
#endif
}

} // namespace bedrock
