/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/Server.hpp"
#include "bedrock/Exception.hpp"
#include "bedrock/MargoManager.hpp"
#include "bedrock/ABTioManager.hpp"
#include "bedrock/ModuleContext.hpp"
#include "bedrock/ProviderManager.hpp"
#include "bedrock/DependencyFinder.hpp"
#include "bedrock/SSGManager.hpp"
#include "MargoLogging.hpp"
#include "ServerImpl.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
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
    spdlog::trace("Initializing MargoManager");
    auto margoConfig = config["margo"].dump();
    // Dependency-injecting spdlog into Margo
    setupMargoLogging();

    // Initialize margo context
    auto margoCtx         = MargoManager(address, margoConfig);
    self->m_margo_manager = margoCtx;
    spdlog::trace("MargoManager initialized");

    // Extract bedrock section from the config
    spdlog::trace("Reading Bedrock config");
    if (!config.contains("bedrock")) config["bedrock"] = json::object();
    if (!config["bedrock"].is_object())
        throw Exception("Invalid entry type for \"bedrock\" (expected object)");
    auto   bedrockConfig = config["bedrock"];
    double dependency_timeout
        = bedrockConfig.value("dependency_resolution_timeout", 30.0);
    uint16_t bedrock_provider_id = bedrockConfig.value("provider_id", 0);
    ABT_pool bedrock_pool        = ABT_POOL_NULL;
    if (bedrockConfig.contains("pool")) {
        auto bedrockPoolRef = bedrockConfig["pool"];
        if (bedrockPoolRef.is_string()) {
            bedrock_pool = margoCtx.getPool(bedrockPoolRef.get<std::string>());
        } else if (bedrockPoolRef.is_number_integer()) {
            bedrock_pool = margoCtx.getPool(bedrockPoolRef.get<int>());
        } else {
            throw Exception("Invalid type in Bedrock's \"pool\" entry");
        }
        if (bedrock_pool == ABT_POOL_NULL) {
            throw Exception(
                "Invalid pool reference {} in Bedrock configuration",
                bedrockPoolRef.dump());
        }
    }

    // Initializing the provider manager
    spdlog::trace("Initializing ProviderManager");
    auto providerManager
        = ProviderManager(margoCtx, bedrock_provider_id, bedrock_pool);
    self->m_provider_manager = providerManager;
    spdlog::trace("ProviderManager initialized");

    // Initialize abt-io context
    spdlog::trace("Initializing ABTioManager");
    auto abtioConfig      = config["abt_io"].dump();
    auto abtioCtx         = ABTioManager(margoCtx, abtioConfig);
    self->m_abtio_manager = abtioCtx;
    spdlog::trace("ABTioManager initialized");

    // Initialize the module context
    spdlog::trace("Initialize ModuleContext");
    auto librariesConfig = config["libraries"].dump();
    ModuleContext::loadModulesFromJSON(librariesConfig);
    spdlog::trace("ModuleContext initialized");

    // Initializing SSG context
    spdlog::trace("Initializing SSGManager");
    auto ssgConfig      = config["ssg"].dump();
    auto ssgCtx         = SSGManager(margoCtx, ssgConfig);
    self->m_ssg_manager = ssgCtx;
    spdlog::trace("SSGManager initialized");

    // Initializing dependency finder
    spdlog::trace("Initializing DependencyFinder");
    auto dependencyFinder
        = DependencyFinder(margoCtx, abtioCtx, ssgCtx, providerManager);
    self->m_dependency_finder            = dependencyFinder;
    self->m_dependency_finder->m_timeout = dependency_timeout;
    spdlog::trace("DependencyFinder initialized");

    // Starting up providers
    spdlog::trace("Initializing providers");
    auto providerManagerConfig = config["providers"].dump();
    providerManager.addProviderListFromJSON(providerManagerConfig,
                                            dependencyFinder);
    spdlog::trace("Providers initialized");
}

Server::~Server() = default;

MargoManager Server::getMargoManager() const { return self->m_margo_manager; }

ABTioManager Server::getABTioManager() const { return self->m_abtio_manager; }

ProviderManager Server::getProviderManager() const {
    return self->m_provider_manager;
}

SSGManager Server::getSSGManager() const { return self->m_ssg_manager; }

void Server::onFinalize(void* uargs) {
    auto server = reinterpret_cast<Server*>(uargs);
    server->self.reset();
}

std::string Server::getCurrentConfig() const {
    return self->makeConfig().dump();
}

void Server::waitForFinalize() {
    margo_push_finalize_callback(self->m_margo_manager->m_mid,
                                 &Server::onFinalize, this);
    margo_wait_for_finalize(self->m_margo_manager->m_mid);
}

} // namespace bedrock
