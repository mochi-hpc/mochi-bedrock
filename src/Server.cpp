/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/Server.hpp"
#include "bedrock/Exception.hpp"
#include "bedrock/MargoContext.hpp"
#include "bedrock/ABTioContext.hpp"
#include "MargoLogging.hpp"
#include "ServerImpl.hpp"
#include <spdlog/spdlog.h>
#include <fstream>

namespace tl = thallium;

namespace bedrock {

using namespace std::string_literals;
using nlohmann::json;

Server::Server(const std::string& address, const std::string& configfile)
: self(std::make_unique<ServerImpl>()) {

    // Read JSON config file
    spdlog::trace("Parsing configuration file {}", configfile);
    json config;
    if (!configfile.empty()) {
        std::ifstream ifs(configfile);
        if (!ifs.good()) {
            self.reset();
            throw Exception("Could not access configuration file "s
                            + configfile);
        }
        ifs >> config;
    }
    spdlog::trace("Parsing done");

    // Extract margo section from the config
    spdlog::trace("Initializing MargoContext");
    auto margoConfig    = config["margo"];
    auto margoConfigStr = margoConfig.dump();
    // Dependency-injecting spdlog into Margo
    setupMargoLogging();
    // Initialize margo context
    auto margoCtx         = MargoContext(address, margoConfigStr);
    self->m_margo_context = margoCtx;
    spdlog::trace("MargoContext initialized");

    // Initialize abt-io context
    spdlog::trace("Initializing ABTioContext");
    auto abtioConfig      = config["abt_io"];
    auto abtioConfigStr   = abtioConfig.dump();
    auto abtioCtx         = ABTioContext(margoCtx, abtioConfigStr);
    self->m_abtio_context = abtioCtx;
    spdlog::trace("ABTioContext initialized");
}

Server::~Server() = default;

MargoContext Server::getMargoContext() const { return self->m_margo_context; }

void Server::waitForFinalize() const {
    margo_wait_for_finalize(self->m_margo_context->m_mid);
}

} // namespace bedrock
