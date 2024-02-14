/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_PROVIDER_MANAGER_HPP
#define __BEDROCK_PROVIDER_MANAGER_HPP

#include <bedrock/NamedDependency.hpp>
#include <bedrock/ProviderDescriptor.hpp>
#include <bedrock/MargoManager.hpp>
#include <bedrock/AbstractServiceFactory.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <memory>

namespace bedrock {

class ProviderManagerImpl;
class Server;
class DependencyFinder;
class Jx9Manager;

/**
 * @brief A ProviderManager is a Bedrock provider that manages
 * a list of providers and uses the ModuleContext to create new ones
 * from configuration files.
 */
class ProviderManager {

    friend class Server;
    friend class DependencyFinder;
    friend class ProviderManagerImpl;

    using json = nlohmann::json;

  public:
    /**
     * @brief Constructor.
     *
     * @param margo Margo context
     * @param jx9 Jx9Manager
     * @param provider_id Provider id at which this provider manager
     * @param pool Pool in which to execute RPCs looking up providers
     */
    ProviderManager(const MargoManager& margo,
                    const Jx9Manager& jx9,
                    uint16_t provider_id,
                    std::shared_ptr<NamedDependency> pool);

    /**
     * @brief Copy-constructor.
     */
    ProviderManager(const ProviderManager&);

    /**
     * @brief Move-constructor.
     */
    ProviderManager(ProviderManager&&);

    /**
     * @brief Copy-assignment operator.
     */
    ProviderManager& operator=(const ProviderManager&);

    /**
     * @brief Move-assignment operator.
     */
    ProviderManager& operator=(ProviderManager&&);

    /**
     * @brief Destructor.
     */
    ~ProviderManager();

    /**
     * @brief Checks whether the ProviderManager instance is valid.
     */
    operator bool() const;

    /**
     * @brief Set the DependencyFinder object to use to resolve dependencies.
     *
     * @param finder DependencyFinder
     */
    void setDependencyFinder(const DependencyFinder& finder);

    /**
     * @brief Return the number of providers registered.
     */
    size_t numProviders() const;

    /**
     * @brief Get an internal provider instance by its name.
     * If not found, this function will throw an Exception.
     * If returned, the shared_ptr is guaranteed not to be null.
     *
     * Note: contrary to lookupProvider, which can accept either
     * "name" or "type:id" as specification, getProvider only expects
     * a provider's name as argument.
     *
     * @return a NamedDependency representing the provider instance.
     */
    std::shared_ptr<ProviderDependency>
        getProvider(const std::string& name) const;

    /**
     * @brief Get an internal provider instance by its index.
     * If not found, this function will throw an Exception.
     * If returned, the shared_ptr is guaranteed not to be null.
     *
     * @return a NamedDependency representing the provider instance.
     */
    std::shared_ptr<ProviderDependency>
        getProvider(size_t index) const;

    /**
     * @brief Look up whether a provider with a given specification exists.
     * The specification must be in the form "<provider-name>" (e.g.
     * "myprovider") or "<provider-type>:<provider-id>" (e.g. "bake:42"). This
     * function returns true if a provider satisfying the spec was found, false
     * otherwise. If a provider is found and wrapper is not nullptr, wrapper is
     * set to the corresponding ProviderWrapper.
     *
     * @param [in] spec Specification string
     *
     * @return a std::shared_ptr<ProviderDependency> pointing to the provider dependency,
     * this pointer will be null if no provider was found.
     */
    std::shared_ptr<ProviderDependency>
        lookupProvider(const std::string& spec) const;

    /**
     * @brief Register a provider from a descriptor.
     *
     * @param name Name of the provider.
     * @param type Type of provider.
     * @param provider_id Provider ID.
     * @param pool Pool.
     * @param config JSON configuration for the provider.
     * @param dependencies Dependency map.
     * @param tags Tags.
     */
    std::shared_ptr<ProviderDependency>
        registerProvider(const std::string&               name,
                         const std::string&               type,
                         uint16_t                         provider_id,
                         std::shared_ptr<NamedDependency> pool,
                         const json&                      config,
                         const ResolvedDependencyMap&     dependencies,
                         const std::vector<std::string>&  tags = {});

    /**
     * @brief Deregister a provider from a specification. The specification has
     * the same format as in lookupProvider().
     *
     * @param spec Specification string
     */
    void deregisterProvider(const std::string& spec);

    /**
     * @brief Add a provider from a JSON description. The description should be
     * of the following form:
     *
     * {
     *      "type" : "remi",
     *      "name" : "my_remi_provider",
     *      "provider_id" : 23,
     *      "dependencies" : {
     *          "abt_io" : "my_abt_io"
     *      },
     *      "config" : { ... },
     *      "tags": [ "tags1", "tag2", ... ]
     *  }
     *
     * @param jsonString JSON string.
     */
    std::shared_ptr<ProviderDependency>
        addProviderFromJSON(const json& description);

    /**
     * @brief Add a list of providers represented by a JSON array.
     *
     * @param list JSON array.
     */
    void addProviderListFromJSON(const json& list);

    /**
     * @brief Change the pool associated with a provider.
     *
     * @param provider Provider.
     * @param pool New pool.
     */
    void changeProviderPool(const std::string& provider,
                            const std::string& pool);

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
     */
    void migrateProvider(const std::string& provider,
                         const std::string& dest_addr,
                         uint16_t           dest_provider_id,
                         const std::string& migration_config,
                         bool               remove_source);

    /**
     * @brief Snapshot the specified provider state to the destination path.
     *
     * @param provider Provider name.
     * @param dest_path Destination path.
     * @param snapshot_config Provider-specific snapshot configuration.
     * @param remove_source Whether to remove the source state.
     */
    void snapshotProvider(const std::string& provider,
                          const std::string& dest_path,
                          const std::string& snapshot_config,
                          bool               remove_source);

    /**
     * @brief Restore the specified provider state from the source path.
     *
     * @param provider Provider name.
     * @param src_path Source path.
     * @param restore_config Provider-specific snapshot configuration.
     * @param remove_source Whether to remove the source state.
     */
    void restoreProvider(const std::string& provider,
                         const std::string& src_path,
                         const std::string& restore_config);

    /**
     * @brief Return the current JSON configuration.
     */
    json getCurrentConfig() const;

  private:
    std::shared_ptr<ProviderManagerImpl> self;

    inline operator std::shared_ptr<ProviderManagerImpl>() const {
        return self;
    }

    inline ProviderManager(std::shared_ptr<ProviderManagerImpl> impl)
    : self(std::move(impl)) {}
};

} // namespace bedrock

#endif
