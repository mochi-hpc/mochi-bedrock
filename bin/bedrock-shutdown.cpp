#include <bedrock/Client.hpp>
#include <ssg.h>
#include <spdlog/spdlog.h>
#include <tclap/CmdLine.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <istream>
#include <ostream>
#include <iterator>
#include <fstream>
#include <streambuf>

using nlohmann::json;

static std::string              g_protocol;
static std::vector<std::string> g_addresses;
static std::string              g_log_level;
static std::string              g_ssg_file;

static void        parseCommandLine(int argc, char** argv);
static void        resolveSSGAddresses(thallium::engine& engine);

int main(int argc, char** argv) {
    parseCommandLine(argc, argv);
    spdlog::set_level(spdlog::level::from_str(g_log_level));

    try {
        auto engine = thallium::engine(g_protocol, THALLIUM_CLIENT_MODE);
        resolveSSGAddresses(engine);
        std::vector<thallium::managed<thallium::thread>> ults;
        for (unsigned i = 0; i < g_addresses.size(); i++) {
            ults.push_back(
                thallium::xstream::self().make_thread([i, &engine]() {
                    try {
                        auto ep = engine.lookup(g_addresses[i]);
                        engine.shutdown_remote_engine(ep);
                    } catch(const std::exception& ex) {
                        spdlog::error("Could not shutdown {}: {}", g_addresses[i], ex.what());
                    }
                }));
        }
        for (auto& ult : ults) { ult->join(); }
        ults.clear();
    } catch (const std::exception& e) { spdlog::critical(e.what()); }
    return 0;
}

static void parseCommandLine(int argc, char** argv) {
    try {
        TCLAP::CmdLine cmd("Query the configuration from Bedrock daemons", ' ',
                           "0.3");
        TCLAP::UnlabeledValueArg<std::string> protocol(
            "protocol", "Protocol (e.g. ofi+tcp)", true, "na+sm", "protocol");
        TCLAP::ValueArg<std::string> logLevel(
            "v", "verbose",
            "Log level (trace, debug, info, warning, error, critical, off)",
            false, "info", "level");
        TCLAP::ValueArg<std::string> ssgFile(
            "s", "ssg-file",
            "SSG file from which to read addresses of Bedrock daemons", false,
            "", "filename");
        TCLAP::MultiArg<std::string> addresses(
            "a", "addresses", "Address of a Bedrock daemon", false, "address");
        TCLAP::SwitchArg prettyJSON("p", "pretty", "Print human-readable JSON",
                                    false);
        cmd.add(protocol);
        cmd.add(logLevel);
        cmd.add(ssgFile);
        cmd.add(addresses);
        cmd.parse(argc, argv);
        g_addresses   = addresses.getValue();
        g_log_level   = logLevel.getValue();
        g_ssg_file    = ssgFile.getValue();
        g_protocol    = protocol.getValue();
    } catch (TCLAP::ArgException& e) {
        std::cerr << "error: " << e.error() << " for arg " << e.argId()
                  << std::endl;
        exit(-1);
    }
}

static void resolveSSGAddresses(thallium::engine& engine) {
    if (g_ssg_file.empty()) return;
    margo_instance_id mid = engine.get_margo_instance();
    int               ret = ssg_init();
    if (ret != SSG_SUCCESS) {
        spdlog::critical("Could not initialize SSG");
        exit(-1);
    }
    int            num_addrs = SSG_ALL_MEMBERS;
    ssg_group_id_t gid       = SSG_GROUP_ID_INVALID;
    ret = ssg_group_id_load(g_ssg_file.c_str(), &num_addrs, &gid);
    if (ret != SSG_SUCCESS) {
        spdlog::critical("Could not load SSG file {}", g_ssg_file);
        exit(-1);
    }
    ret = ssg_group_observe(mid, gid);
    if (ret != SSG_SUCCESS) {
        spdlog::critical("Could not observe SSG group");
        exit(-1);
    }
    int group_size = 0;
    ret            = ssg_get_group_size(gid, &group_size);
    if (ret != SSG_SUCCESS) {
        spdlog::critical("Could not get SSG group size");
        exit(-1);
    }
    for (int i = 0; i < group_size; i++) {
        ssg_member_id_t member_id = SSG_MEMBER_ID_INVALID;
        ret = ssg_get_group_member_id_from_rank(gid, i, &member_id);
        if (member_id == SSG_MEMBER_ID_INVALID || ret != SSG_SUCCESS) {
            spdlog::critical(
                "ssg_get_group_member_id_from_rank (rank={}) returned "
                "invalid member id",
                i);
            exit(-1);
        }
        hg_addr_t addr = HG_ADDR_NULL;
        ret            = ssg_get_group_member_addr(gid, member_id, &addr);
        if (addr == HG_ADDR_NULL || ret != SSG_SUCCESS) {
            spdlog::critical(
                "Could not get address from SSG member {} (rank {})", member_id,
                i);
            exit(-1);
        }
        g_addresses.emplace_back(
            static_cast<std::string>(thallium::endpoint(engine, addr, false)));
    }
    ssg_group_unobserve(gid);
    ssg_finalize();
}
