/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_SERVER_HPP
#define __BEDROCK_SERVER_HPP

#include <bedrock/MargoManager.hpp>
#include <bedrock/ABTioManager.hpp>
#include <bedrock/SSGManager.hpp>
#include <bedrock/ProviderManager.hpp>
#include <bedrock/ClientManager.hpp>
#include <thallium.hpp>
#include <memory>

namespace bedrock {

namespace tl = thallium;

class ServerImpl;

/**
 * @brief Enum class to specify the type of configuration
 * being used to intialize the server.
 */
enum class ConfigType {
    JSON,
    JX9
};

/**
 * @brief Parameters to pass to Jx9 config.
 */
typedef std::unordered_map<std::string, std::string> Jx9ParamMap;

/**
 * @brief The Server class is encapsulating everything needed for
 * a Bedrock service to to be instaniated on a process. It is used
 * in bin/bedrock.cpp.
 */
class Server {

  public:
    /**
     * @brief Constructor.
     *
     * @param address Address of the server.
     * @param config JSON or JX9 configuration script.
     * @param configType type of configuration (JSON or JX9).
     * @param jx9Params parameters to pass to Jx9 configuration.
     */
    Server(const std::string& address, const std::string& config = "",
           ConfigType configType = ConfigType::JSON,
           const Jx9ParamMap& jx9Params = Jx9ParamMap());

    /**
     * @brief Copy-constructor is deleted.
     */
    Server(const Server&) = delete;

    /**
     * @brief Move-constructor.
     */
    Server(Server&&) = delete;

    /**
     * @brief Copy-assignment operator is deleted.
     */
    Server& operator=(const Server&) = delete;

    /**
     * @brief Move-assignment operator is deleted.
     */
    Server& operator=(Server&&) = delete;

    /**
     * @brief Destructor.
     */
    ~Server();

    /**
     * @brief Get the underlying MargoManager.
     */
    MargoManager getMargoManager() const;

    /**
     * @brief Get the underlying ABTioManager.
     */
    ABTioManager getABTioManager() const;

    /**
     * @brief Get the underlying ProviderManager.
     */
    ProviderManager getProviderManager() const;

    /**
     * @brief Get the underlying ClientManager.
     */
    ClientManager getClientManager() const;

    /**
     * @brief Get the underlying SSG context.
     */
    SSGManager getSSGManager() const;

    /**
     * @brief Blocks until the underlying margo instance is finalized.
     */
    void waitForFinalize();

    /**
     * @brief Finalize the underlying margo instance.
     */
    void finalize();

    /**
     * @brief Return the current JSON configuration.
     */
    std::string getCurrentConfig() const;

  private:
    std::unique_ptr<ServerImpl> self;

    static void onFinalize(void* uargs);
    static void onPreFinalize(void* uargs);
};

} // namespace bedrock

#endif
