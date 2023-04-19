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
     * @brief If the ServiceGroupHandle was built from an SSG group,
     * this function will refresh the group view.
     */
    void refresh() const;

    /**
     * @brief Ask the remote service daemon to load a module library.
     *
     * @param name Name of the module.
     * @param path Library path.
     * @param req Asynchronous request to wait on, if provided.
     */
    void loadModule(const std::string& name, const std::string& path,
                    AsyncRequest* req = nullptr) const;

    /**
     * @brief Starts a new provider on the target service daemon.
     *
     * @param name Name of the new provider.
     * @param type Type of the new provider.
     * @param provider_id Provider id.
     * @param pool Pool for the provider to use.
     * @param config Configuration (JSON-formatted).
     * @param dependencies Dependencies for the provider.
     * @param req Asynchronous request to wait on, if provided.
     */
    void startProvider(const std::string& name, const std::string& type,
                       uint16_t provider_id, const std::string& pool = "",
                       const std::string&   config       = "{}",
                       const DependencyMap& dependencies = DependencyMap(),
                       AsyncRequest*        req          = nullptr) const;

    /**
     * @brief Request that a provider change its pool for another one.
     *
     * @param provider_name Name of the provider.
     * @param pool_name Name of the new pool.
     * @param req Asynchronous request to wait on, if provided.
     */
    void changeProviderPool(const std::string& provider_name,
                            const std::string& pool_name,
                            AsyncRequest*      req = nullptr) const;

    /**
     * @brief Creates a client on the target service daemon.
     *
     * @param name Name of the new client.
     * @param type Type of the new client.
     * @param config Configuration (JSON-formatted).
     * @param dependencies Dependencies for the client.
     * @param req Asynchronous request to wait on, if provided.
     */
    void addClient(const std::string& name, const std::string& type,
                   const std::string&   config       = "{}",
                   const DependencyMap& dependencies = DependencyMap(),
                   AsyncRequest*        req          = nullptr) const;

    /**
     * @brief Creates a new ABT-IO instance on the target service daemon.
     *
     * @param name Name of the new ABT-IO instance.
     * @param pool Name of the pool to use.
     * @param config Configuration (JSON-formatted).
     * @param req Asynchronous request to wait on, if provided.
     */
    void addABTioInstance(const std::string& name, const std::string& pool,
                          const std::string& config = "{}",
                          AsyncRequest*      req    = nullptr) const;

    /**
     * @brief Adds an SSG group to the target service daemon.
     * The group is created from the provided JSON configuration.
     *
     * @param config JSON configuration.
     * @param req Asynchronous request to wait on, if provided.
     */
    void addSSGgroup(const std::string& config,
                     AsyncRequest*      req = nullptr) const;


    /**
     * @brief Adds an Argobots pool to the Margo instance of
     * the target service.
     *
     * @param config JSON configuration of the pool.
     * @param req Asynchronous request to wait on, if provided.
     */
    void addPool(const std::string& config,
                 AsyncRequest*      req = nullptr) const;

    /**
     * @brief Adds an Argobots xstream to the Margo instance of
     * the target service.
     *
     * @param config JSON configuration of the ES.
     * @param req Asynchronous request to wait on, if provided.
     */
    void addXstream(const std::string& config,
                    AsyncRequest*      req = nullptr) const;

    /**
     * @brief Removes an Argobots pool from the Margo instance of
     * the target service.
     *
     * @param name Name of the pool to remove.
     * @param req Asynchronous request to wait on, if provided.
     */
    void removePool(const std::string& name,
                    AsyncRequest*      req = nullptr) const;

    /**
     * @brief Removes an Argobots xstream from the Margo instance of
     * the target service.
     *
     * @param name Name of the ES to remove.
     * @param req Asynchronous request to wait on, if provided.
     */
    void removeXstream(const std::string& name,
                       AsyncRequest*      req = nullptr) const;

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
