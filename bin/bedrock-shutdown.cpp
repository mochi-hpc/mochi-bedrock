#include <bedrock/Client.hpp>
#ifdef ENABLE_SSG
#include <ssg.h>
#endif
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

static void parseCommandLine(int argc, char** argv);
static void resolveFlockAddresses();

int main(int argc, char** argv) {
    parseCommandLine(argc, argv);
    spdlog::set_level(spdlog::level::from_str(g_log_level));

    try {
        auto engine = thallium::engine(g_protocol, THALLIUM_CLIENT_MODE);
        resolveFlockAddresses();
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
        TCLAP::ValueArg<std::string> flockFile(
            "f", "flock-file",
            "Flock file from which to read addresses of Bedrock daemons", false,
            "", "filename");
        TCLAP::MultiArg<std::string> addresses(
            "a", "addresses", "Address of a Bedrock daemon", false, "address");
        TCLAP::SwitchArg prettyJSON("p", "pretty", "Print human-readable JSON",
                                    false);
        cmd.add(protocol);
        cmd.add(logLevel);
        cmd.add(flockFile);
        cmd.add(addresses);
        cmd.parse(argc, argv);
        g_addresses   = addresses.getValue();
        g_log_level   = logLevel.getValue();
        g_flock_file  = flockFile.getValue();
        g_protocol    = protocol.getValue();
    } catch (TCLAP::ArgException& e) {
        std::cerr << "error: " << e.error() << " for arg " << e.argId()
                  << std::endl;
        exit(-1);
    }
}

static void resolveFlockAddresses() {
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
}
