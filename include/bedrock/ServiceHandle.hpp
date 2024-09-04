/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_SERVICE_HANDLE_HPP
#define __BEDROCK_SERVICE_HANDLE_HPP

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
class ServiceHandleImpl;
class ServiceGroupHandle;

/**
 * @brief A ServiceHandle object is a handle for a remote resource
 * on a server. It enables invoking the resource's functionalities.
 */
class ServiceHandle {

    friend class Client;
    friend class ServiceGroupHandle;

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
     * @brief Returns the provider handle underlying the ServiceHandle.
     */
    tl::provider_handle providerHandle() const;

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
     * @brief Creates a provider on the target service daemon.
     *
     * @param description JSON description of the provider.
     * @param req Asynchronous request to wait on, if provided.
     */
    void addProvider(const std::string& description,
                     uint16_t*          provider_id_out = nullptr,
                     AsyncRequest*      req = nullptr) const;


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
     * @brief Migrates the specified provider state to the destination.
     *
     * Note: the source provider will not be deregistered.
     * Note: a provider of the same type must exist at the designated
     * destination address and ID.
     *
     * @param provider Provider name.
     * @param dest_addr Destination address.
     * @param dest_provider_id Destination provider ID.
     * @param migration_config Provider-specific JSON configuration.
     * @param remove_source Whether to remove the source state.
     * @param req Asynchronous request to wait on, if provided.
     */
    void migrateProvider(
            const std::string& provider,
            const std::string& dest_addr,
            uint16_t           dest_provider_id,
            const std::string& migration_config,
            bool               remove_source,
            AsyncRequest*      req = nullptr) const;

    /**
     * @brief Snapshot the specified provider state to the destination path.
     *
     * @param provider Provider name.
     * @param dest_path Destination path.
     * @param snapshot_config Provider-specific snapshot configuration.
     * @param remove_source Whether to remove the source state.
     * @param req Asynchronous request to wait on, if provided.
     */
    void snapshotProvider(
            const std::string& provider,
            const std::string& dest_path,
            const std::string& snapshot_config,
            bool               remove_source,
            AsyncRequest*      req = nullptr) const;

    /**
     * @brief Restore the specified provider state from the source path.
     *
     * @param provider Provider name.
     * @param src_path Source path.
     * @param restore_config Provider-specific snapshot configuration.
     * @param remove_source Whether to remove the source state.
     * @param req Asynchronous request to wait on, if provided.
     */
    void restoreProvider(
            const std::string& provider,
            const std::string& src_path,
            const std::string& restore_config,
            AsyncRequest*      req = nullptr) const;

    /**
     * @brief Creates a client on the target service daemon.
     *
     * @param description JSON description of the client.
     * @param req Asynchronous request to wait on, if provided.
     */
    void addClient(const std::string& description,
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
