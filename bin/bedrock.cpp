#include <bedrock/Server.hpp>
#include <spdlog/spdlog.h>
#include <tclap/CmdLine.h>
#include <iostream>
#include <vector>
#include <string>
#include <istream>
#include <ostream>
#include <iterator>
#include <fstream>
#include <streambuf>

static std::string g_address;
static std::string g_log_level;
static std::string g_config_file;
static bool        g_use_stdin;

static void        parseCommandLine(int argc, char** argv);
static std::string getConfigFromStdIn();
static std::string getConfigFromFile(const std::string& filename);

int main(int argc, char** argv) {
    parseCommandLine(argc, argv);
    spdlog::set_level(spdlog::level::from_str(g_log_level));
    try {
        std::string config;
        if (g_use_stdin)
            config = getConfigFromStdIn();
        else if (!g_config_file.empty())
            config = getConfigFromFile(g_config_file);
        bedrock::Server server(g_address, config);
        spdlog::trace("Done initializing server, config is as follows:\n{}",
                      server.getCurrentConfig());
        server.waitForFinalize();
    } catch (const std::exception& e) { spdlog::critical(e.what()); }
    return 0;
}

static void parseCommandLine(int argc, char** argv) {
    try {
        TCLAP::CmdLine cmd("Spawns a Bedrock daemon", ' ', "0.1");
        TCLAP::UnlabeledValueArg<std::string> address(
            "address",
            "Protocol (e.g. ofi+tcp) or address (e.g. "
            "ofi+tcp://127.0.0.1:1234)",
            true, "na+sm", "address");
        TCLAP::ValueArg<std::string> logLevel(
            "v", "verbose",
            "Log level (trace, debug, info, warning, error, critical, off)",
            false, "info", "level");
        TCLAP::ValueArg<std::string> configFile(
            "c", "config", "JSON configuration file", false, "", "config-file");
        TCLAP::SwitchArg stdinSwitch(
            "", "stdin", "Read JSON configuration from standard input", false);
        cmd.add(address);
        cmd.add(logLevel);
        cmd.add(configFile);
        cmd.add(stdinSwitch);
        cmd.parse(argc, argv);
        g_address     = address.getValue();
        g_log_level   = logLevel.getValue();
        g_config_file = configFile.getValue();
        g_use_stdin   = stdinSwitch.getValue();
        if (g_use_stdin && !g_config_file.empty()) {
            std::cerr << "error: both config file and --stdin were provided"
                      << std::endl;
            exit(-1);
        }
    } catch (TCLAP::ArgException& e) {
        std::cerr << "error: " << e.error() << " for arg " << e.argId()
                  << std::endl;
        exit(-1);
    }
}

static std::string getConfigFromStdIn() {
    std::cin >> std::noskipws;
    std::istream_iterator<char> it(std::cin);
    std::istream_iterator<char> end;
    return std::string(it, end);
}

static std::string getConfigFromFile(const std::string& filename) {
    std::ifstream t(filename.c_str());
    return std::string(std::istreambuf_iterator<char>(t),
                       std::istreambuf_iterator<char>());
}
