#include <bedrock/Client.hpp>
#include <bedrock/ServiceGroupHandle.hpp>
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
static std::string              g_flock_file;
static std::string              g_jx9_file;
static std::string              g_jx9_script_content;
static uint16_t                 g_provider_id;
static bool                     g_pretty;

static void parseCommandLine(int argc, char** argv);

int main(int argc, char** argv) {
    parseCommandLine(argc, argv);
    spdlog::set_level(spdlog::level::from_str(g_log_level));

    if (!g_jx9_file.empty()) {
        std::ifstream t(g_jx9_file.c_str());
        if(not t.good()) {
            spdlog::critical("Could not read jx9 file {}", g_jx9_file);
            exit(-1);
        }
        std::string   str((std::istreambuf_iterator<char>(t)),
                std::istreambuf_iterator<char>());
        g_jx9_script_content = std::move(str);
    }

    try {
        auto engine = thallium::engine(g_protocol, THALLIUM_CLIENT_MODE);

        if(!g_flock_file.empty()) {
            json flock_file_content;
            std::ifstream flock_file{g_flock_file};
            if(!flock_file.good()) {
                spdlog::critical("Could not open flock file {}", g_flock_file);
                exit(-1);
            }
            flock_file >> flock_file_content;
            std::unordered_set<std::string> addresses;
            for(auto& member : flock_file_content["members"]) {
                auto& address = member["address"].get_ref<const std::string&>();
                if(addresses.count(address)) continue;
                g_addresses.push_back(address);
                addresses.insert(address);
            }
        }

        bedrock::Client client(engine);

        auto sgh = client.makeServiceGroupHandle(g_addresses, g_provider_id);

        std::string result;
        if (g_jx9_script_content.empty())
            sgh.getConfig(&result);
        else
            sgh.queryConfig(g_jx9_script_content, &result);
        std::cout << (g_pretty ? json::parse(result).dump(4) : result) << std::endl;
    } catch (const std::exception& e) { spdlog::critical(e.what()); exit(-1); }
    return 0;
}

static void parseCommandLine(int argc, char** argv) {
    #define VALUE(string) #string
    #define TO_LITERAL(string) VALUE(string)
    try {
        TCLAP::CmdLine cmd("Query the configuration from Bedrock daemons", ' ',
                           TO_LITERAL(BEDROCK_VERSION));
        TCLAP::UnlabeledValueArg<std::string> protocol(
            "protocol", "Protocol (e.g. ofi+tcp)", true, "na+sm", "protocol");
        TCLAP::ValueArg<std::string> logLevel(
            "v", "verbose",
            "Log level (trace, debug, info, warning, error, critical, off)",
            false, "info", "level");
        TCLAP::ValueArg<uint16_t> providerID(
            "i", "provider-id",
            "Provider id to use when contacting Bedrock daemons", false, 0,
            "int");
        TCLAP::ValueArg<std::string> flockFile(
            "f", "flock-file",
            "Flock file from which to read addresses of Bedrock daemons", false,
            "", "filename");
        TCLAP::ValueArg<std::string> jx9File(
            "j", "jx9-file", "Jx9 file to send to processes and execute", false,
            "", "filename");
        TCLAP::MultiArg<std::string> addresses(
            "a", "addresses", "Address of a Bedrock daemon", false, "address");
        TCLAP::SwitchArg prettyJSON("p", "pretty", "Print human-readable JSON",
                                    false);
        cmd.add(protocol);
        cmd.add(logLevel);
        cmd.add(flockFile);
        cmd.add(providerID);
        cmd.add(addresses);
        cmd.add(prettyJSON);
        cmd.add(jx9File);
        cmd.parse(argc, argv);
        g_addresses   = addresses.getValue();
        g_log_level   = logLevel.getValue();
        g_flock_file  = flockFile.getValue();
        g_protocol    = protocol.getValue();
        g_provider_id = providerID.getValue();
        g_pretty      = prettyJSON.getValue();
        g_jx9_file    = jx9File.getValue();
        if(g_addresses.empty()) {
            std::cerr << "error: no address specified" << std::endl;
            exit(-1);
        }
    } catch (TCLAP::ArgException& e) {
        std::cerr << "error: " << e.error() << " for arg " << e.argId()
                  << std::endl;
        exit(-1);
    }
}
