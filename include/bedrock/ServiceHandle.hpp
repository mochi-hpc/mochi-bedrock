/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_RESOURCE_HANDLE_HPP
#define __BEDROCK_RESOURCE_HANDLE_HPP

#include <thallium.hpp>
#include <memory>
#include <unordered_set>
#include <nlohmann/json.hpp>
#include <bedrock/Client.hpp>
#include <bedrock/Exception.hpp>
#include <bedrock/AsyncRequest.hpp>

namespace bedrock {

namespace tl = thallium;

class Client;
class ServiceHandleImpl;

/**
 * @brief A ServiceHandle object is a handle for a remote resource
 * on a server. It enables invoking the resource's functionalities.
 */
class ServiceHandle {

    friend class Client;

  public:
    /**
     * @brief Constructor. The resulting ServiceHandle handle will be invalid.
     */
    ServiceHandle();

    /**
     * @brief Copy-constructor.
     */
    ServiceHandle(const ServiceHandle&);

    /**
     * @brief Move-constructor.
     */
    ServiceHandle(ServiceHandle&&);

    /**
     * @brief Copy-assignment operator.
     */
    ServiceHandle& operator=(const ServiceHandle&);

    /**
     * @brief Move-assignment operator.
     */
    ServiceHandle& operator=(ServiceHandle&&);

    /**
     * @brief Destructor.
     */
    ~ServiceHandle();

    /**
     * @brief Returns the client this database has been opened with.
     */
    Client client() const;

    /**
     * @brief Get the JSON configuration of a service process.
     *
     * @param [out] config Resulting configuration.
     * @param [out] req Asynchronous request to wait on, if provided.
     */
    void getConfig(std::string* config, AsyncRequest* req = nullptr) const;

    /**
     * @brief Checks if the ServiceHandle instance is valid.
     */
    operator bool() const;

  private:
    /**
     * @brief Constructor is private. Use a Client object
     * to create a ServiceHandle instance.
     *
     * @param impl Pointer to implementation.
     */
    ServiceHandle(const std::shared_ptr<ServiceHandleImpl>& impl);

    std::shared_ptr<ServiceHandleImpl> self;
};

} // namespace bedrock

#endif
