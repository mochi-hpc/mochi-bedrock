/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_SERVER_HPP
#define __BEDROCK_SERVER_HPP

#include <bedrock/MargoContext.hpp>
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
     * @brief Get the underlying MargoContext.
     *
     * @return the underlying MargoContext.
     */
    MargoContext getMargoContext() const;

    /**
     * @brief Blocks until the underlying margo instance is finalized.
     */
    void waitForFinalize() const;

  private:
    std::unique_ptr<ServerImpl> self;
};

} // namespace bedrock

#endif
