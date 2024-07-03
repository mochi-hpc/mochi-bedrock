/*
 * (C) 2023 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_SERVICE_GROUP_HANDLE_HPP
#define __BEDROCK_SERVICE_GROUP_HANDLE_HPP

#include <bedrock/Client.hpp>
#include <bedrock/Exception.hpp>
#include <bedrock/AsyncRequest.hpp>
#include <bedrock/DependencyMap.hpp>

#include <thallium.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <unordered_set>
#include <vector>
#include <unordered_map>

namespace bedrock {

namespace tl = thallium;

class Client;
class ServiceGroupHandleImpl;

/**
 * @brief A ServiceGroupHandle object is a handle for a remote resource
 * on a set of servers. It enables invoking functionalities on all the
 * underlying ServiceHandles.
 */
class ServiceGroupHandle {

    friend class Client;

  public:
    /**
     * @brief Constructor. The resulting ServiceGroupHandle handle will be invalid.
     */
    ServiceGroupHandle();

    /**
     * @brief Copy-constructor.
     */
    ServiceGroupHandle(const ServiceGroupHandle&);

    /**
     * @brief Move-constructor.
     */
    ServiceGroupHandle(ServiceGroupHandle&&);

    /**
     * @brief Copy-assignment operator.
     */
    ServiceGroupHandle& operator=(const ServiceGroupHandle&);

    /**
     * @brief Move-assignment operator.
     */
    ServiceGroupHandle& operator=(ServiceGroupHandle&&);

    /**
     * @brief Destructor.
     */
    ~ServiceGroupHandle();

    /**
     * @brief Returns the client this database has been opened with.
     */
    Client client() const;

    /**
     * @brief If the ServiceGroupHandle was built from an SSG group
     * or a Flock group, this function will refresh the group view.
     */
    void refresh() const;

    /**
     * @brief Return the number of underlying ServiceHandles.
     */
    size_t size() const;

    /**
     * @brief Returns the i-th underlying ServiceHandle.
     */
    ServiceHandle operator[](size_t i) const;

    /**
     * @brief Get the JSON configuration of a service process.
     *
     * @param [out] config Resulting configuration.
     * @param [out] req Asynchronous request to wait on, if provided.
     */
    void getConfig(std::string* config, AsyncRequest* req = nullptr) const;

    /**
     * @brief Send a Jx9 script to be executed by the server.
     * In the Jx9 script, $__config__ represents the server's configuration.
     * The value of result will be set to the value returned by the script.
     *
     * @param script Jx9 script.
     * @param result Result from the script.
     * @param req Asynchronous request to wait on, if provided.
     */
    void queryConfig(const std::string& script, std::string* result,
                     AsyncRequest* req = nullptr) const;

    /**
     * @brief Checks if the ServiceGroupHandle instance is valid.
     */
    operator bool() const;

  private:
    /**
     * @brief Constructor is private. Use a Client object
     * to create a ServiceGroupHandle instance.
     *
     * @param impl Pointer to implementation.
     */
    ServiceGroupHandle(const std::shared_ptr<ServiceGroupHandleImpl>& impl);

    std::shared_ptr<ServiceGroupHandleImpl> self;
};

} // namespace bedrock

#endif
