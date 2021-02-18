/*
 * (C) 2021 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_CLIENT_MANAGER_HPP
#define __BEDROCK_CLIENT_MANAGER_HPP

#include <bedrock/MargoManager.hpp>
#include <bedrock/AbstractServiceFactory.hpp>
#include <bedrock/ClientWrapper.hpp>
#include <string>
#include <memory>

namespace bedrock {

class ClientManagerImpl;
class Server;
class DependencyFinder;

/**
 * @brief A ClientManager is a Bedrock provider that manages
 * a list of clients and uses the ModuleContext to create new ones
 * from configuration files.
 */
class ClientManager {

    friend class Server;
    friend class DependencyFinder;
    friend class ClientManagerImpl;

  public:
    /**
     * @brief Constructor.
     *
     * @param margo Margo context
     * @param provider_id Provider id used by this client manager
     * @param pool Pool in which to execute RPCs looking up clients
     */
    ClientManager(const MargoManager& margo, uint16_t provider_id = 0,
                  ABT_pool pool = ABT_POOL_NULL);

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
     * @brief Look up whether a client with a given name exists.
     * This function returns true if a client was found, false
     * otherwise. If a client is found and wrapper is not nullptr,
     * wrapper is set to the corresponding ClientWrapper.
     *
     * @param [in] name Name of the client
     * @param [out] wrapper Resulting client wrapper
     *
     * @return true if client was found, false otherwise.
     */
    bool lookupClient(const std::string& name,
                      ClientWrapper*     wrapper = nullptr) const;

    /**
     * @brief Find any client of a certain type. If none is found,
     * create such a client. This function will throw an exception
     * if the client could not be found nor created.
     *
     * @param [in] type Type of client to lookup
     * @param [out] wrapper Resulting client wrapper
     */
    void lookupOrCreateAnonymous(const std::string& type,
                                 ClientWrapper*     wrapper = nullptr);

    /**
     * @brief List the clients managed by the ClientManager.
     *
     * @return the clients managed by the ClientManager.
     */
    std::vector<ClientDescriptor> listClients() const;

    /**
     * @brief Register a client from a descriptor.
     *
     * @param descriptor Descriptor (name and type).
     * @param config JSON configuration for the client.
     * @param dependencies Dependency map.
     */
    void createClient(const ClientDescriptor&      descriptor,
                      const std::string&           config,
                      const ResolvedDependencyMap& dependencies);

    /**
     * @brief Destroy a client.
     *
     * @param name Name of the client.
     */
    void destroyClient(const std::string& name);

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
     *      "config" : { ... }
     *  }
     *
     * @param jsonString JSON string.
     * @param finder DependencyFinder to resolve the dependencies found.
     */
    void addClientFromJSON(const std::string&      jsonString,
                           const DependencyFinder& finder);

    /**
     * @brief Add a list of providers represented by a JSON string.
     *
     * @param jsonString JSON string.
     * @param finder DependencyFinder.
     */
    void addClientListFromJSON(const std::string&      jsonString,
                               const DependencyFinder& finder);

    /**
     * @brief Return the current JSON configuration.
     */
    std::string getCurrentConfig() const;

  private:
    std::shared_ptr<ClientManagerImpl> self;

    inline operator std::shared_ptr<ClientManagerImpl>() const { return self; }

    inline ClientManager(std::shared_ptr<ClientManagerImpl> impl)
    : self(std::move(impl)) {}
};

} // namespace bedrock

#endif
