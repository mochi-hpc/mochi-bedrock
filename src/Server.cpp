/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <bedrock/Server.hpp>
#include <bedrock/MargoManager.hpp>
#include <bedrock/ABTioManager.hpp>
#include <bedrock/MonaManager.hpp>
#include <bedrock/ModuleContext.hpp>
#include <bedrock/ProviderManager.hpp>
#include <bedrock/ClientManager.hpp>
#include <bedrock/DependencyFinder.hpp>
#include <bedrock/SSGManager.hpp>
#include <bedrock/Jx9Manager.hpp>
#include <bedrock/Exception.hpp>
#include "MargoLogging.hpp"
#include "ServerImpl.hpp"
#include "JsonUtil.hpp"
#include "TomlUtil.hpp"
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <fstream>

namespace tl = thallium;

namespace bedrock {

using namespace std::string_literals;
using nlohmann::json;

Server::Server(const std::string& address, const std::string& configString,
               ConfigType configType, const Jx9ParamMap& jx9Params) {

    std::string configStr;

    auto mpi = MPIEnv{};

    auto jx9Manager = Jx9Manager{mpi};

    if(configType == ConfigType::JX9) {
        spdlog::trace("Interpreting JX9 template configuration");
        configStr = jx9Manager.executeQuery(configString, jx9Params);
        configType = ConfigType::JSON;
        spdlog::trace("JX9 template configuration interpreted");
    } else { // JSON or TOML
        configStr = configString;
    }

    // Read JSON or TOML config
    json config;
    if(configType == ConfigType::JSON) {
        spdlog::trace("Parsing JSON configuration");
        if (!configStr.empty()) {
            try {
                config = json::parse(configStr);
            } catch(const std::exception& ex) {
                throw Exception(ex.what());
            }
        } else {
            config = json::object();
        }
        config = expandSimplifiedJSON(config);
        spdlog::trace("Parsing done");
    } else { // TOML
        spdlog::trace("Parsing TOML configuration");
        toml::value tomlConfig;
        try {
            tomlConfig = toml::parse_str(configStr);
        } catch(const std::exception& ex) {
            throw Exception{ex.what()};
        }
        config = Toml2Json(tomlConfig);
    }

    // Filter __if__ statements in configuration
    config = filterIfConditionsInJSON(config, jx9Manager);

    // If the config is an array, it should have only one entry remaining after filtering
    if(config.is_array()) {
        if(config.size() == 1) {
            config = config[0];
        } else {
            if(!mpi.isEnabled()) {
                throw Exception{"Configuration resolved to an array but MPI is not enabled"};
            }
            if(config.size() == (size_t)mpi.globalSize()) {
                config = config[mpi.globalRank()];
            } else {
                throw Exception{"Ambiguous configuration did not resolve "
                                "to a single possibility for the process"};
            }
        }
    }

    // Extract margo section from the config
    spdlog::trace("Initializing MargoManager");
    auto margoConfig = config["margo"].dump();
    // Dependency-injecting spdlog into Margo
    setupMargoLogging();

    // Initialize margo context
    auto margoMgr = MargoManager(address, margoConfig);
    spdlog::trace("MargoManager initialized");

    // Sync addresses with other MPI members
    std::string thisAddress = margoMgr.getThalliumEngine().self();
    mpi.self->exchangeAddresses(thisAddress);

    // Extract bedrock section from the config
    spdlog::trace("Reading Bedrock config");
    if (!config.contains("bedrock")) config["bedrock"] = json::object();
    if (!config["bedrock"].is_object())
        throw BEDROCK_DETAILED_EXCEPTION("Invalid entry type for \"bedrock\" (expected object)");
    auto   bedrockConfig = config["bedrock"];
    double dependency_timeout
        = bedrockConfig.value("dependency_resolution_timeout", 30.0);
    uint16_t bedrock_provider_id = bedrockConfig.value("provider_id", 0);
    std::shared_ptr<NamedDependency> bedrock_pool = margoMgr.getDefaultHandlerPool();
    if (bedrockConfig.contains("pool")) {
        auto bedrockPoolRef = bedrockConfig["pool"];
        if (bedrockPoolRef.is_string()) {
            bedrock_pool = margoMgr.getPool(bedrockPoolRef.get<std::string>());
        } else {
            throw BEDROCK_DETAILED_EXCEPTION("Invalid type in Bedrock's \"pool\" entry");
        }
        if (!bedrock_pool) {
            throw BEDROCK_DETAILED_EXCEPTION(
                "Invalid pool reference {} in Bedrock configuration",
                bedrockPoolRef.dump());
        }
    }

    // Create self
    self = std::unique_ptr<ServerImpl>(
            new ServerImpl(margoMgr, bedrock_provider_id, bedrock_pool));
    self->m_mpi = mpi.self;
    self->m_jx9_manager = jx9Manager;

    try {

        // Initializing SSG context
        spdlog::trace("Initializing SSGManager");
        auto& ssgConfig     = config["ssg"];
        auto ssgMgr         = SSGManager(margoMgr, jx9Manager, ssgConfig);
        self->m_ssg_manager = ssgMgr;
        spdlog::trace("SSGManager initialized");

        // Initialize abt-io context
        spdlog::trace("Initializing ABTioManager");
        auto& abtioConfig     = config["abt_io"];
        auto abtioMgr         = ABTioManager(margoMgr, jx9Manager, abtioConfig);
        self->m_abtio_manager = abtioMgr;
        spdlog::trace("ABTioManager initialized");

        // Initialize mona manager
        spdlog::trace("Initializing MonaManager");
        auto& monaConfig     = config["mona"];
        auto monaMgr         = MonaManager(margoMgr, jx9Manager, monaConfig, address);
        self->m_mona_manager = monaMgr;
        spdlog::trace("MonaManager initialized");

        // Initializing the provider manager
        spdlog::trace("Initializing ProviderManager");
        auto providerManager
            = ProviderManager(margoMgr, jx9Manager, bedrock_provider_id, bedrock_pool);
        self->m_provider_manager = providerManager;
        spdlog::trace("ProviderManager initialized");

        // Initializing the client manager
        spdlog::trace("Initializing ClientManager");
        auto clientManager
            = ClientManager(margoMgr, jx9Manager, bedrock_provider_id, bedrock_pool);
        self->m_client_manager = clientManager;
        spdlog::trace("ClientManager initialized");

        // Initialize the module context
        spdlog::trace("Initialize ModuleContext");
        auto librariesConfig = config["libraries"].dump();
        ModuleContext::loadModulesFromJSON(librariesConfig);
        spdlog::trace("ModuleContext initialized");

        // Initializing dependency finder
        spdlog::trace("Initializing DependencyFinder");
        auto dependencyFinder     = DependencyFinder(
            mpi, margoMgr, abtioMgr, ssgMgr, monaMgr, providerManager, clientManager);
        self->m_dependency_finder = dependencyFinder;
        self->m_dependency_finder->m_timeout = dependency_timeout;
        spdlog::trace("DependencyFinder initialized");

        // Creating clients
        spdlog::trace("Initializing clients");
        clientManager.setDependencyFinder(dependencyFinder);
        auto& clientManagerConfig = config["clients"];
        clientManager.addClientListFromJSON(clientManagerConfig);

        // Starting up providers
        spdlog::trace("Initializing providers");
        auto& providerManagerConfig = config["providers"];
        providerManager.setDependencyFinder(dependencyFinder);
        providerManager.addProviderListFromJSON(providerManagerConfig);
        spdlog::trace("Providers initialized");

    } catch(const Exception& ex) {
        finalize();
        throw;
    }

    spdlog::info("Bedrock daemon now running at {}",
                 static_cast<std::string>(margoMgr.getThalliumEngine().self()));
}

Server::~Server() {
    if(self) finalize();
}

MargoManager Server::getMargoManager() const { return self->m_margo_manager; }

ABTioManager Server::getABTioManager() const { return self->m_abtio_manager; }

ProviderManager Server::getProviderManager() const {
    return self->m_provider_manager;
}

ClientManager Server::getClientManager() const {
    return self->m_client_manager;
}

SSGManager Server::getSSGManager() const { return self->m_ssg_manager; }

void Server::onPreFinalize(void* uargs) {
    spdlog::trace("Calling Server's pre-finalize callback");
    auto server = reinterpret_cast<Server*>(uargs);
    if(server->self && server->self->m_ssg_manager)
        server->self->m_ssg_manager->clear();
}

void Server::onFinalize(void* uargs) {
    spdlog::trace("Calling Server's finalize callback");
    auto server = reinterpret_cast<Server*>(uargs);
    server->self.reset();
}

std::string Server::getCurrentConfig() const {
    return self->makeConfig().dump();
}

void Server::waitForFinalize() {
    margo_push_finalize_callback(self->m_margo_manager->m_mid,
                                 &Server::onFinalize, this);
    margo_push_prefinalize_callback(self->m_margo_manager->m_mid,
                                 &Server::onPreFinalize, this);
    margo_wait_for_finalize(self->m_margo_manager->m_mid);
}

void Server::finalize() {
    margo_push_finalize_callback(self->m_margo_manager->m_mid,
                                 &Server::onFinalize, this);
    margo_push_prefinalize_callback(self->m_margo_manager->m_mid,
                                 &Server::onPreFinalize, this);
    margo_finalize_and_wait(self->m_margo_manager->m_mid);
}

} // namespace bedrock
