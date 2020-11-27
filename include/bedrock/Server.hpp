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
#include <thallium.hpp>
#include <memory>

namespace bedrock {

namespace tl = thallium;

class ServerImpl;

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
     * @param configfile Path to a configuration file.
     */
    Server(const std::string& address, const std::string& configfile = "");

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
     * @brief Get the underlying SSG context.
     */
    SSGManager getSSGManager() const;

    /**
     * @brief Blocks until the underlying margo instance is finalized.
     */
    void waitForFinalize();

    /**
     * @brief Return the current JSON configuration.
     */
    std::string getCurrentConfig() const;

  private:
    std::unique_ptr<ServerImpl> self;

    static void onFinalize(void* uargs);
};

} // namespace bedrock

#endif
