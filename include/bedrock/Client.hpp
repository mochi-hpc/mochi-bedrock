/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_CLIENT_HPP
#define __BEDROCK_CLIENT_HPP

#include <bedrock/ServiceHandle.hpp>
#include <bedrock/UUID.hpp>
#include <thallium.hpp>
#include <memory>

namespace bedrock {

class ClientImpl;
class ServiceHandle;

/**
 * @brief The Client object is the main object used to establish
 * a connection with a Bedrock service.
 */
class Client {

    friend class ServiceHandle;

  public:
    /**
     * @brief Default constructor.
     */
    Client();

    /**
     * @brief Constructor using a margo instance id.
     *
     * @param mid Margo instance id.
     */
    Client(margo_instance_id mid);

    /**
     * @brief Constructor.
     *
     * @param engine Thallium engine.
     */
    Client(const thallium::engine& engine);

    /**
     * @brief Copy constructor.
     */
    Client(const Client&);

    /**
     * @brief Move constructor.
     */
    Client(Client&&);

    /**
     * @brief Copy-assignment operator.
     */
    Client& operator=(const Client&);

    /**
     * @brief Move-assignment operator.
     */
    Client& operator=(Client&&);

    /**
     * @brief Destructor.
     */
    ~Client();

    /**
     * @brief Returns the thallium engine used by the client.
     */
    const thallium::engine& engine() const;

    /**
     * @brief Creates a handle to a remote Service and returns.
     * You may set "check" to false if you know for sure that the
     * corresponding Service exists, which will avoid one RPC.
     *
     * @param address Address of the provider holding the database.
     * @param provider_id Provider id.
     * @param Service_id Service UUID.
     * @param check Checks if the Database exists by issuing an RPC.
     *
     * @return a ServiceHandle instance.
     */
    ServiceHandle makeServiceHandle(const std::string& address,
                                    uint16_t           provider_id,
                                    const UUID&        Service_id,
                                    bool               check = true) const;

    /**
     * @brief Checks that the Client instance is valid.
     */
    operator bool() const;

  private:
    Client(const std::shared_ptr<ClientImpl>& impl);

    std::shared_ptr<ClientImpl> self;
};

} // namespace bedrock

#endif
