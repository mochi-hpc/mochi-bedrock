/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/Server.hpp"
#include "bedrock/Exception.hpp"
#include "bedrock/MargoContext.hpp"
#include "bedrock/ABTioContext.hpp"
#include "bedrock/ModuleContext.hpp"
#include "bedrock/ProviderManager.hpp"
#include "bedrock/DependencyFinder.hpp"
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
    auto margoConfig = config["margo"].dump();
    // Dependency-injecting spdlog into Margo
    setupMargoLogging();

    // Initialize margo context
    auto margoCtx         = MargoContext(address, margoConfig);
    self->m_margo_context = margoCtx;
    spdlog::trace("MargoContext initialized");

    // Initializing the provider manager
    spdlog::trace("Initializing ProviderManager");
    auto providerManager     = ProviderManager(margoCtx);
    self->m_provider_manager = providerManager;
    spdlog::trace("ProviderManager initialized");

    // Initialize abt-io context
    spdlog::trace("Initializing ABTioContext");
    auto abtioConfig      = config["abt_io"].dump();
    auto abtioCtx         = ABTioContext(margoCtx, abtioConfig);
    self->m_abtio_context = abtioCtx;
    spdlog::trace("ABTioContext initialized");

    // Initialize the module context
    spdlog::trace("Initialize ModuleContext");
    auto librariesConfig = config["libraries"].dump();
    ModuleContext::loadModulesFromJSON(librariesConfig);
    spdlog::trace("ModuleContext initialized");

    // Initializing dependency finder
    spdlog::trace("Initializing DependencyFinder");
    auto dependencyFinder
        = DependencyFinder(margoCtx, abtioCtx, providerManager);
    spdlog::trace("DependencyFinder initialized");

    // Starting up providers
    spdlog::trace("Initializing providers");
    auto providerManagerConfig = config["providers"].dump();
    providerManager.addProviderListFromJSON(providerManagerConfig,
                                            dependencyFinder);
    spdlog::trace("Providers initialized");
}

Server::~Server() = default;

MargoContext Server::getMargoContext() const { return self->m_margo_context; }

void Server::waitForFinalize() const {
    margo_wait_for_finalize(self->m_margo_context->m_mid);
}

} // namespace bedrock
