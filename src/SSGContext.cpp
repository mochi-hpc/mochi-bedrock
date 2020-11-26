/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/SSGContext.hpp"
#include "bedrock/Exception.hpp"
#include "bedrock/MargoContext.hpp"
#include "SSGContextImpl.hpp"
#include <margo.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#ifdef ENABLE_MPI
    #include <ssg-mpi.h>
#endif
#ifdef ENABLE_PMIX
    #include <ssg-pmix.h>
#endif
#include <regex>

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

static int s_num_ssg_init = 0;
#ifdef ENABLE_MPI
static bool s_initialized_mpi = false;
#endif
#ifdef ENABLE_PMIX
static bool s_initialized_pmix = false;
#endif

static void validateGroupConfig(json&                           config,
                                const std::vector<std::string>& existing_names,
                                const MargoContext&             margo) {
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
        && bootstrap != "pmix") {
        throw Exception(
            "Invalid bootstrap value \"{}\" in SSG group configuration",
            bootstrap);
    }
    if (config.contains("pool")) {
        if (config["pool"].is_string()) {
            auto pool_name = config["pool"].get<std::string>();
            if (ABT_POOL_NULL == margo.getPool(pool_name)) {
                throw Exception(
                    "Could not find pool \"{}\" in SSG configuration",
                    pool_name);
            }
        } else if (config["pool"].is_number_integer()) {
            auto pool_index = config["pool"].get<int>();
            if (ABT_POOL_NULL == margo.getPool(pool_index)) {
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
                                    const MargoContext& margo,
                                    std::string& name, std::string& bootstrap,
                                    ssg_group_config_t& group_config,
                                    std::string& group_file, ABT_pool& pool) {
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

SSGContext::SSGContext(const MargoContext& margo,
                       const std::string&  configString)
: self(std::make_shared<SSGContextImpl>()) {
    self->m_margo_context = margo;
    auto config           = json::parse(configString);
    if (config.is_null()) return;

    if (s_num_ssg_init == 0) {
        int ret = ssg_init();
        if (ret != SSG_SUCCESS) {
            throw Exception("Could not initialize SSG (ssg_init returned {})",
                            ret);
        }
    }
    s_num_ssg_init += 1;

    auto addGroup = [this, &margo](const auto& config) {
        std::string        name, bootstrap, group_file;
        ssg_group_config_t group_config;
        ABT_pool           pool;
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
}

SSGContext::SSGContext(const SSGContext&) = default;

SSGContext::SSGContext(SSGContext&&) = default;

SSGContext& SSGContext::operator=(const SSGContext&) = default;

SSGContext& SSGContext::operator=(SSGContext&&) = default;

SSGContext::~SSGContext() {
    if (self.use_count() == 1) {
        s_num_ssg_init -= 1;
        if (s_num_ssg_init == 0) ssg_finalize();
        for (auto& g : self->m_ssg_groups) {
            int ret = ssg_group_leave(g.second->m_gid);
            if (ret != SSG_SUCCESS) {
                spdlog::error(
                    "Could not leave SSG group \"{}\" "
                    "(ssg_group_leave returned {})",
                    g.first, ret);
            }
        }
    }
}

SSGContext::operator bool() const { return static_cast<bool>(self); }

ssg_group_id_t SSGContext::getGroup(const std::string& group_name) const {
    auto it = self->m_ssg_groups.find(group_name);
    if (it == self->m_ssg_groups.end())
        return SSG_GROUP_ID_INVALID;
    else
        return it->second->m_gid;
}

ssg_group_id_t SSGContext::createGroup(const std::string&        name,
                                       const ssg_group_config_t* config,
                                       ABT_pool                  pool,
                                       const std::string& bootstrap_method,
                                       const std::string& group_file) {
    ssg_group_id_t gid        = SSG_GROUP_ID_INVALID;
    auto           mid        = self->m_margo_context->m_mid;
    auto           group_data = std::make_unique<SSGData>();
    group_data->m_context     = self;

    spdlog::trace("Creating SSG group {} with bootstrap method {}", name,
                  bootstrap_method);

    (void)pool; // TODO add support for specifying the pool

    if (bootstrap_method == "init") {

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

        gid = ssg_group_create(mid, name.c_str(), addresses.data(), 1,
                               const_cast<ssg_group_config_t*>(config),
                               SSGContext::membershipUpdate, group_data.get());

    } else if (bootstrap_method == "join") {

        if (group_file.empty()) {
            throw Exception(
                "SSG \"join\" bootstrapping mode requires a group file to be "
                "specified");
        }
        int num_addrs = 32; // XXX make that configurable?
        int ret       = ssg_group_id_load(group_file.c_str(), &num_addrs, &gid);
        if (ret != SSG_SUCCESS) {
            throw Exception(
                "Failed to load SSG group from file {}"
                " (ssg_group_id_load returned {}",
                group_file, ret);
        }
        ret = ssg_group_join(mid, gid, SSGContext::membershipUpdate,
                             group_data.get());
        if (ret != SSG_SUCCESS) {
            throw Exception(
                "Failed to join SSG group {} "
                "(ssg_group_join returned {})",
                name, ret);
        }

    } else if (bootstrap_method == "mpi") {
#ifdef ENABLE_MPI
        int flag;
        MPI_Initialized(&flag);
        if (!flag) {
            MPI_Init(NULL, NULL);
            s_initialized_mpi = true;
        }
        gid = ssg_group_create_mpi(
            self->m_margo_context->m_mid, name.c_str(), MPI_COMM_WORLD,
            const_cast<ssg_group_config_t*>(config),
            SSGContext::membershipUpdate, group_data.get());
#else
        throw Exception("Bedrock was not compiled with MPI support");
#endif

    } else if (bootstrap_method == "pmix") {
#ifdef ENABLE_PMIX
        pmix_proc_t proc;
        auto        ret = PMIx_Init(&proc, NULL, 0);
        if (ret != PMIX_SUCCESS) {
            throw Exception("Could not initialize PMIx (PMIx_Init returned {})",
                            ret);
        }
        gid = ssg_group_create_pmix(
            self->m_margo_context->m_mid, name.c_str(), proc,
            const_cast<ssg_group_config_t*>(config),
            SSGContext::membershipUpdate, group_data.get());
#else
        throw Exception("Bedrock was not compiled with PMIx support");
#endif
    } else {
        throw Exception("Invalid SSG bootstrapping method {}",
                        bootstrap_method);
    }
    if (!group_file.empty()
        && (bootstrap_method == "init" || bootstrap_method == "mpi"
            || bootstrap_method == "pmix")) {
        int rank = ssg_get_group_self_rank(gid);
        if (rank == -1) {
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
    group_data->m_gid        = gid;
    self->m_ssg_groups[name] = std::move(group_data);
    return gid;
}

void SSGContext::membershipUpdate(void* group_data, ssg_member_id_t member_id,
                                  ssg_member_update_type_t update_type) {
    SSGData* gdata = reinterpret_cast<SSGData*>(group_data);
    (void)gdata;
    spdlog::trace("SSG membership updated: member_id={}, update_type={}",
                  member_id, update_type);
}

hg_addr_t SSGContext::resolveAddress(const std::string& address) const {
    std::regex re(
            "ssg:\\/\\/([a-zA-Z_][a-zA-Z0-9_]*)\\/(#?)([0-9][0-9]*)$");
    std::smatch match;
    if (std::regex_search(address, match, re)) {
        auto group_name   = match.str(1);
        auto is_member_id = match.str(2) == "#";
        auto member_id_or_rank = atol(match.str(3).c_str());
        auto gid = getGroup(group_name);
        if(gid == SSG_GROUP_ID_INVALID) {
            throw Exception("Could not resolve \"{}\" to a valid SSG group");
        }
        ssg_member_id_t member_id;
        if(!is_member_id) {
            member_id = ssg_get_group_member_id_from_rank(gid, member_id_or_rank);
            if(member_id == SSG_MEMBER_ID_INVALID) {
                throw Exception("Invalid rank {} in group {}", member_id_or_rank, group_name);
            }
        } else {
            member_id = member_id_or_rank;
        }
        hg_addr_t addr = ssg_get_group_member_addr(gid, member_id);
        if(addr == HG_ADDR_NULL) {
            throw Exception("Invalid member id {} in group {}", member_id, group_name);
        }
        return addr;
    } else {
        throw Exception("Invalid SSG address specification \"{}\"", address);
    }
    return HG_ADDR_NULL;
}

} // namespace bedrock
