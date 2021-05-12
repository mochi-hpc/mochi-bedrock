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
#include "bedrock/ClientManager.hpp"
#include "bedrock/DependencyFinder.hpp"
#include "bedrock/SSGManager.hpp"
#include "bedrock/Jx9Manager.hpp"
#include "MargoLogging.hpp"
#include "ServerImpl.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <fstream>

namespace tl = thallium;

namespace bedrock {

using namespace std::string_literals;
using nlohmann::json;

Server::Server(const std::string& address, const std::string& configString) {

    // Read JSON config file
    spdlog::trace("Parsing JSON configuration");
    json config;
    if (!configString.empty()) {
        config = json::parse(configString);
    } else {
        config = json::object();
    }
    spdlog::trace("Parsing done");

    // Extract margo section from the config
    spdlog::trace("Initializing MargoManager");
    auto margoConfig = config["margo"].dump();
    // Dependency-injecting spdlog into Margo
    setupMargoLogging();

    // Initialize margo context
    auto margoMgr = MargoManager(address, margoConfig);
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
    ABT_pool bedrock_pool        = margoMgr.getDefaultHandlerPool();
    if (bedrockConfig.contains("pool")) {
        auto bedrockPoolRef = bedrockConfig["pool"];
        if (bedrockPoolRef.is_string()) {
            bedrock_pool = margoMgr.getPool(bedrockPoolRef.get<std::string>());
        } else if (bedrockPoolRef.is_number_integer()) {
            bedrock_pool = margoMgr.getPool(bedrockPoolRef.get<int>());
        } else {
            throw Exception("Invalid type in Bedrock's \"pool\" entry");
        }
        if (bedrock_pool == ABT_POOL_NULL) {
            throw Exception(
                "Invalid pool reference {} in Bedrock configuration",
                bedrockPoolRef.dump());
        }
    }

    // Create self
    self = std::make_unique<ServerImpl>(margoMgr.getThalliumEngine(),
                                        bedrock_provider_id,
                                        tl::pool(bedrock_pool));
    self->m_margo_manager = margoMgr;

    // Initializing the jx9 manaher
    spdlog::trace("Initializing Jx9Manager");
    auto jx9Manager = Jx9Manager();
    self->m_jx9_manager = jx9Manager;
    spdlog::trace("Jx9Manager initialized");

    // Initializing the provider manager
    spdlog::trace("Initializing ProviderManager");
    auto providerManager
        = ProviderManager(margoMgr, bedrock_provider_id, bedrock_pool);
    self->m_provider_manager = providerManager;
    spdlog::trace("ProviderManager initialized");

    // Initializing the client manager
    spdlog::trace("Initializing ClientManager");
    auto clientManager
        = ClientManager(margoMgr, bedrock_provider_id, bedrock_pool);
    self->m_client_manager = clientManager;
    spdlog::trace("ClientManager initialized");

    // Initialize abt-io context
    spdlog::trace("Initializing ABTioManager");
    auto abtioConfig      = config["abt_io"].dump();
    auto abtioMgr         = ABTioManager(margoMgr, abtioConfig);
    self->m_abtio_manager = abtioMgr;
    spdlog::trace("ABTioManager initialized");

    // Initialize the module context
    spdlog::trace("Initialize ModuleContext");
    auto librariesConfig = config["libraries"].dump();
    ModuleContext::loadModulesFromJSON(librariesConfig);
    spdlog::trace("ModuleContext initialized");

    // Initializing SSG context
    spdlog::trace("Initializing SSGManager");
    auto ssgConfig      = config["ssg"].dump();
    auto ssgMgr         = SSGManager(margoMgr, ssgConfig);
    self->m_ssg_manager = ssgMgr;
    spdlog::trace("SSGManager initialized");

    // Initializing dependency finder
    spdlog::trace("Initializing DependencyFinder");
    auto dependencyFinder     = DependencyFinder(margoMgr, abtioMgr, ssgMgr,
                                             providerManager, clientManager);
    self->m_dependency_finder = dependencyFinder;
    self->m_dependency_finder->m_timeout = dependency_timeout;
    spdlog::trace("DependencyFinder initialized");

    // Creating clients
    spdlog::trace("Initializing clients");
    auto clientManagerConfig = config["clients"].dump();
    clientManager.addClientListFromJSON(clientManagerConfig, dependencyFinder);

    // Starting up providers
    spdlog::trace("Initializing providers");
    auto providerManagerConfig = config["providers"].dump();
    ProviderManager(self->m_provider_manager)
        .setDependencyFinder(dependencyFinder);
    providerManager.addProviderListFromJSON(providerManagerConfig);
    spdlog::trace("Providers initialized");

    spdlog::info("Bedrock daemon now running at {}",
                 static_cast<std::string>(margoMgr.getThalliumEngine().self()));
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
