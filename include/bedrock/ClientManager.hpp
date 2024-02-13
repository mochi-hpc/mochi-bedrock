/*
 * (C) 2021 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_CLIENT_MANAGER_HPP
#define __BEDROCK_CLIENT_MANAGER_HPP

#include <bedrock/MargoManager.hpp>
#include <bedrock/AbstractServiceFactory.hpp>
#include <bedrock/ClientDescriptor.hpp>
#include <bedrock/NamedDependency.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <memory>

namespace bedrock {

class ClientManagerImpl;
class Server;
class ServerImpl;
class DependencyFinder;
class Jx9Manager;

/**
 * @brief A ClientManager is a Bedrock provider that manages
 * a list of clients and uses the ModuleContext to create new ones
 * from configuration files.
 */
class ClientManager {

    friend class Server;
    friend class DependencyFinder;
    friend class ClientManagerImpl;
    friend class ServerImpl;

    using json = nlohmann::json;

  public:
    /**
     * @brief Constructor.
     *
     * @param margo Margo context
     * @param jx9 Jx9 manager
     * @param provider_id Provider id used by this client manager
     * @param pool Pool in which to execute RPCs looking up clients
     */
    ClientManager(const MargoManager& margo, const Jx9Manager& jx9,
                  uint16_t provider_id,
                  const std::shared_ptr<NamedDependency>& pool);

    /**
     * @brief Copy-constructor.
     */
    ClientManager(const ClientManager&);

    /**
     * @brief Move-constructor.
     */
    ClientManager(ClientManager&&);

    /**
     * @brief Copy-assignment operator.
     */
    ClientManager& operator=(const ClientManager&);

    /**
     * @brief Move-assignment operator.
     */
    ClientManager& operator=(ClientManager&&);

    /**
     * @brief Destructor.
     */
    ~ClientManager();

    /**
     * @brief Checks whether the ClientManager instance is valid.
     */
    operator bool() const;

    /**
     * @brief Set the DependencyFinder object to use to resolve dependencies.
     *
     * @param finder DependencyFinder
     */
    void setDependencyFinder(const DependencyFinder& finder);

    /**
     * @brief Get an internal client instance by its name.
     * If not found, this function will throw an Exception.
     * If returned, the shared_ptr is guaranteed not to be null.
     *
     * @return a NamedDependency representing the client instance.
     */
    std::shared_ptr<NamedDependency>
        getClient(const std::string& name) const;

    /**
     * @brief Get an internal client instance by its index.
     * If not found, this function will throw an Exception.
     * If returned, the shared_ptr is guaranteed not to be null.
     *
     * @return a NamedDependency representing the client instance.
     */
    std::shared_ptr<NamedDependency>
        getClient(size_t index) const;

    /**
     * @brief Find any client of a certain type. If none is found,
     * create such a client. This function will throw an exception
     * if the client could not be found nor created.
     *
     * @param [in] type Type of client to lookup
     * @param [out] wrapper Resulting client wrapper
     */
    std::shared_ptr<NamedDependency>
        getOrCreateAnonymous(const std::string& type);

    /**
     * @brief List the clients managed by the ClientManager.
     *
     * @return the clients managed by the ClientManager.
     */
    std::vector<ClientDescriptor> listClients() const;

    /**
     * @brief Return the number of clients.
     */
    size_t numClients() const;

    /**
     * @brief Create a client.
     *
     * @param name Name of the client.
     * @param type Type of the client.
     * @param config JSON configuration for the client.
     * @param dependencies Dependency map.
     * @param tags Tags.
     */
    std::shared_ptr<NamedDependency>
        addClient(const std::string&           name,
                  const std::string&           type,
                  const json&                  config,
                  const ResolvedDependencyMap& dependencies,
                  const std::vector<std::string>& tags = {});

    /**
     * @brief Destroy a client.
     *
     * @param name Name of the client.
     */
    void removeClient(const std::string& name);

    /**
     * @brief Destroy a client.
     *
     * @param index Index..
     */
    void removeClient(size_t index);

    /**
     * @brief Add a client from a full JSON description. The description should
     * be of the following form:
     *
     * {
     *      "type" : "remi",
     *      "name" : "my_remi_client",
     *      "dependencies" : {
     *          "abt_io" : "my_abt_io"
     *      },
     *      "config" : { ... },
     *      "tage" : [ "tag1", "tag2", ... ]
     *  }
     *
     * @param jsonString JSON string.
     */
    std::shared_ptr<NamedDependency>
        addClientFromJSON(const json& description);

    /**
     * @brief Add a list of providers represented by a JSON array.
     * The JSON array entries must follow the format expected by
     * addClientFromJSON.
     *
     * @param jsonString JSON string.
     */
    void addClientListFromJSON(const json& list);

    /**
     * @brief Return the current JSON configuration.
     */
    json getCurrentConfig() const;

  private:
    std::shared_ptr<ClientManagerImpl> self;

    inline operator std::shared_ptr<ClientManagerImpl>() const { return self; }

    inline ClientManager(std::shared_ptr<ClientManagerImpl> impl)
    : self(std::move(impl)) {}
};

} // namespace bedrock

#endif
