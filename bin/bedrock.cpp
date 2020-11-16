#include <bedrock/Server.hpp>
#include <spdlog/spdlog.h>
#include <tclap/CmdLine.h>
#include <iostream>
#include <vector>

static std::string g_address;
static std::string g_log_level;
static std::string g_config_file;

static void parseCommandLine(int argc, char** argv);

int main(int argc, char** argv) {
    parseCommandLine(argc, argv);
    spdlog::set_level(spdlog::level::from_str(g_log_level));
    try {
        bedrock::Server server(g_address, g_config_file);
        server.waitForFinalize();
    } catch(const std::exception& e) {
        spdlog::critical(e.what());
    }
    return 0;
}

static void parseCommandLine(int argc, char** argv) {
    try {
        TCLAP::CmdLine cmd("Spawns a Bedrock daemon", ' ', "0.1");
        TCLAP::UnlabeledValueArg<std::string> address(
                "address", "Protocol (e.g. ofi+tcp) or address (e.g. ofi+tcp://127.0.0.1:1234)", true, "na+sm", "address");
        TCLAP::ValueArg<std::string> logLevel("v","verbose", "Log level (trace, debug, info, warning, error, critical, off)", false, "info", "level");
        TCLAP::ValueArg<std::string> configFile("c", "config", "JSON configuration file", false, "", "config-file");
        cmd.add(address);
        cmd.add(logLevel);
        cmd.add(configFile);
        cmd.parse(argc, argv);
        g_address = address.getValue();
        g_log_level = logLevel.getValue();
        g_config_file = configFile.getValue();
    } catch(TCLAP::ArgException &e) {
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
        exit(-1);
    }
}
