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
static std::string g_output_file;
static bedrock::ConfigType g_config_type = bedrock::ConfigType::JSON;
static std::string g_jx9_params;

static void        parseCommandLine(int argc, char** argv);
static std::string getConfigFromStdIn();
static std::string getConfigFromFile(const std::string& filename);
static bedrock::Jx9ParamMap parseJx9Params(const std::string& args);

int main(int argc, char** argv) {
    parseCommandLine(argc, argv);
    spdlog::set_level(spdlog::level::from_str(g_log_level));
    try {
        std::string config;
        if (g_use_stdin)
            config = getConfigFromStdIn();
        else if (!g_config_file.empty())
            config = getConfigFromFile(g_config_file);
        auto jx9_params = parseJx9Params(g_jx9_params);
        bedrock::Server server(g_address, config, g_config_type, jx9_params);
        if (!g_output_file.empty()) {
            std::ofstream output_file(g_output_file);
            output_file << server.getCurrentConfig();
        }
        server.waitForFinalize();
    } catch (const std::exception& e) { spdlog::critical(e.what()); }
    return 0;
}

static bedrock::Jx9ParamMap parseJx9Params(const std::string& args) {
    bedrock::Jx9ParamMap map;
    if(args.empty())
        return map;
    auto s_stream = std::stringstream(args);
    while(s_stream.good()) {
        std::string assignment;
        std::getline(s_stream, assignment, ',');
        auto equal = assignment.find('=');
        if(equal == std::string::npos) {
            std::cerr << "Invalid definition of " << assignment << " in jx9 parameters"
                      << std::endl;
            exit(-1);
        }
        auto varname = assignment.substr(0, equal);
        auto vardef = assignment.substr(equal+1);
        map[varname] = vardef;
    }
    return map;
}

static void parseCommandLine(int argc, char** argv) {
    try {
        TCLAP::CmdLine cmd("Spawns a Bedrock daemon", ' ', "0.4");
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
            "c", "config", "JSON, JX9, or TOML configuration file", false, "", "config-file");
        TCLAP::ValueArg<std::string> outConfigFile(
            "o", "output-config", "JSON file to write after deployment", false,
            "", "config-file");
        TCLAP::SwitchArg stdinSwitch(
            "", "stdin", "Read configuration from standard input", false);
        TCLAP::SwitchArg jx9Switch(
            "j", "jx9", "Interpret configuration as a Jx9 script", false);
        TCLAP::SwitchArg tomlSwitch(
            "t", "toml", "Configuration is in TOML format instead of JSON", false);
        TCLAP::ValueArg<std::string> jx9Params(
            "", "jx9-context", "Comma-separated list of Jx9 parameters for the Jx9 script",
            false, "", "x=1,y=2,z=something,...");
        cmd.add(address);
        cmd.add(logLevel);
        cmd.add(configFile);
        cmd.add(outConfigFile);
        cmd.add(stdinSwitch);
        cmd.add(jx9Switch);
        cmd.add(jx9Params);
        cmd.add(tomlSwitch);
        cmd.parse(argc, argv);
        g_address     = address.getValue();
        g_log_level   = logLevel.getValue();
        g_config_file = configFile.getValue();
        g_output_file = outConfigFile.getValue();
        g_use_stdin   = stdinSwitch.getValue();
        g_jx9_params  = jx9Params.getValue();
        if (g_use_stdin && configFile.isSet()) {
            std::cerr << "error: both config file and --stdin were provided"
                      << std::endl;
            exit(-1);
        }
        if (jx9Switch.getValue()) {
            if (tomlSwitch.getValue()) {
                std::cerr << "error: cannot use both --jx9/-j and --toml/-t" << std::endl;
                exit(-1);
            }
            g_config_type = bedrock::ConfigType::JX9;
        } else if(jx9Params.isSet()) {
            std::cerr << "error: passing Jx9 parameters for a JSON configuration"
                      << std::endl;
            exit(-1);
        }
        if (tomlSwitch.getValue()) {
            g_config_type = bedrock::ConfigType::TOML;
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
    if(!t.good()) {
        std::cerr << "error: could not read configuration file " << filename << std::endl;
        exit(-1);
    }
    return std::string(std::istreambuf_iterator<char>(t),
                       std::istreambuf_iterator<char>());
}
