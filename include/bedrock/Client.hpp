/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_CLIENT_HPP
#define __BEDROCK_CLIENT_HPP

#include <bedrock/ServiceHandle.hpp>
#include <thallium.hpp>
#include <memory>

// forward declared to avoid including flock if Bedrock
// wasn't built with support for it.
typedef struct flock_group_handle* flock_group_handle_t;
typedef struct flock_client* flock_client_t;

namespace bedrock {

class ClientImpl;
class ServiceHandle;
class ServiceGroupHandle;

/**
 * @brief The Client object is the main object used to establish
 * a connection with a Bedrock service.
 */
class Client {

    friend class ServiceHandle;
    friend class ServiceGroupHandle;

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
     * @brief Creates a handle to a remote Service.
     *
     * @param address Address of the provider holding the database.
     * @param provider_id Provider id.
     *
     * @return a ServiceHandle instance.
     */
    ServiceHandle makeServiceHandle(
            const std::string& address,
            uint16_t           provider_id=0) const;

    /**
     * @brief Creates a handle to a group of Bedrock processes
     * from an Flock group file.
     *
     * @param groupfile Flock group file.
     * @param provider_id Provider ID of the bedrock providers.
     *
     * @return ServiceGroupHandle instance.
     */
    ServiceGroupHandle makeServiceGroupHandleFromFlockFile(
            const std::string& groupfile,
            uint16_t provider_id=0) const;

    /**
     * @brief Creates a handle to a group of Bedrock processes.
     *
     * @param handle Existing Flock group handle.
     * @param provider_id Provider ID of the bedrock providers.
     *
     * @return ServiceGroupHandle instance.
     */
    ServiceGroupHandle makeServiceGroupHandleFromFlockGroup(
            flock_group_handle_t handle,
            uint16_t provider_id=0) const;

    /**
     * @brief Creates a handle to a group of Bedrock processes
     * from a list of addresses. This type of group cannot be refreshed.
     *
     * @param addresses Array of addresses.
     * @param provider_id Provider ID of the bedrock providers.
     *
     * @return ServiceGroupHandle instance.
     */
    ServiceGroupHandle makeServiceGroupHandle(
            const std::vector<std::string>& addresses,
            uint16_t provider_id=0) const;

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
