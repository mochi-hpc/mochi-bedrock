/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_PROVIDER_MANAGER_HPP
#define __BEDROCK_PROVIDER_MANAGER_HPP

#include <bedrock/ProviderWrapper.hpp>
#include <bedrock/MargoManager.hpp>
#include <string>
#include <memory>

namespace bedrock {

class ProviderManagerImpl;
class Server;
class DependencyFinder;

/**
 * @brief A ProviderManager is a Bedrock provider that manages
 * a list of providers and uses the ModuleContext to create new ones
 * from configuration files.
 */
class ProviderManager {

    friend class Server;
    friend class DependencyFinder;
    friend class ProviderManagerImpl;

  public:
    /**
     * @brief Constructor.
     *
     * @param margo Margo context
     * @param provider_id Provider id at which this provider manager
     * @param pool Pool in which to execute RPCs looking up providers
     * responds for remote provider lookups.
     */
    ProviderManager(const MargoManager& margo, uint16_t provider_id = 0,
                    ABT_pool pool = ABT_POOL_NULL);

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
     * @brief Look up whether a provider with a given specification exists.
     * The specification must be in the form "<provider-name>" (e.g.
     * "myprovider") or "<provider-type>:<provider-id>" (e.g. "bake:42"). This
     * function returns true if a provider satisfying the spec was found, false
     * otherwise. If a provider is found and wrapper is not nullptr, wrapper is
     * set to the corresponding ProviderWrapper.
     *
     * @param [in] spec Specification string
     * @param [out] wrapper Resulting provider wrapper
     *
     * @return true if provider was found, false otherwise.
     */
    bool lookupProvider(const std::string& spec,
                        ProviderWrapper*   wrapper = nullptr) const;

    /**
     * @brief List the providers managed by the ProviderManager.
     *
     * @return the providers managed by the ProviderManager.
     */
    std::vector<ProviderDescriptor> listProviders() const;

    /**
     * @brief Register a provider from a descriptor.
     *
     * @param descriptor Descriptor.
     * @param pool_name Pool name.
     * @param config JSON configuration for the provider.
     * @param dependencies Dependency map.
     */
    void
    registerProvider(const ProviderDescriptor& descriptor,
                     const std::string& pool_name, const std::string& config,
                     const std::unordered_map<std::string, std::vector<void*>>&
                         dependencies);

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
     *      "config" : { ... }
     *  }
     *
     * @param jsonString JSON string.
     * @param finder DependencyFinder to resolve the dependencies found.
     */
    void addProviderFromJSON(const std::string&      jsonString,
                             const DependencyFinder& finder);

    /**
     * @brief Add a list of providers represented by a JSON string.
     *
     * @param jsonString JSON string.
     * @param finder DependencyFinder.
     */
    void addProviderListFromJSON(const std::string&      jsonString,
                                 const DependencyFinder& finder);

    /**
     * @brief Return the current JSON configuration.
     */
    std::string getCurrentConfig() const;

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
