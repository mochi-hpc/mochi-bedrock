#include <bedrock/Client.hpp>
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
static uint16_t                 g_provider_id;
static bool                     g_pretty;

static void        parseCommandLine(int argc, char** argv);
static void        resolveSSGAddresses();
static std::string lookupBedrockConfig(const bedrock::Client& client,
                                       const std::string&     address);

int main(int argc, char** argv) {
    parseCommandLine(argc, argv);
    spdlog::set_level(spdlog::level::from_str(g_log_level));
    try {
        thallium::engine engine(g_protocol, THALLIUM_CLIENT_MODE);
        resolveSSGAddresses();
        bedrock::Client          client(engine);
        std::vector<std::string> configs(g_addresses.size());
        std::vector<thallium::managed<thallium::thread>> ults;
        for (unsigned i = 0; i < g_addresses.size(); i++) {
            ults.push_back(
                thallium::xstream::self().make_thread([i, &client, &configs]() {
                    configs[i] = lookupBedrockConfig(client, g_addresses[i]);
                }));
        }
        for (auto& ult : ults) { ult->join(); }
        ults.clear();
        std::stringstream ss;
        ss << "{";
        for (unsigned i = 0; i < g_addresses.size(); i++) {
            ss << "\"" << g_addresses[i] << "\":" << configs[i];
            if (i != g_addresses.size() - 1) ss << ",";
        }
        ss << "}";
        if (!g_pretty)
            std::cout << ss.str() << std::endl;
        else {
            std::cout << json::parse(ss.str()).dump(4) << std::endl;
        }
    } catch (const std::exception& e) { spdlog::critical(e.what()); }
    return 0;
}

static void parseCommandLine(int argc, char** argv) {
    try {
        TCLAP::CmdLine cmd("Query the configuration from Bedrock daemons", ' ',
                           "0.1");
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
        cmd.add(providerID);
        cmd.add(addresses);
        cmd.add(prettyJSON);
        cmd.parse(argc, argv);
        g_addresses   = addresses.getValue();
        g_log_level   = logLevel.getValue();
        g_ssg_file    = ssgFile.getValue();
        g_protocol    = protocol.getValue();
        g_provider_id = providerID.getValue();
        g_pretty      = prettyJSON.getValue();
    } catch (TCLAP::ArgException& e) {
        std::cerr << "error: " << e.error() << " for arg " << e.argId()
                  << std::endl;
        exit(-1);
    }
}

static void resolveSSGAddresses() {
    if (g_ssg_file.empty()) return;
    throw std::runtime_error(
        "SSG support is not implemented yet in bedrock-query");
}

static std::string lookupBedrockConfig(const bedrock::Client& client,
                                       const std::string&     address) {
    auto serviceHandle = client.makeServiceHandle(address, g_provider_id);
    std::string config;
    serviceHandle.getConfig(&config);
    return config;
}
