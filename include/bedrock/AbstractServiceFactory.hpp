/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_ABSTRACT_SERVICE_FACTORY_HPP
#define __BEDROCK_ABSTRACT_SERVICE_FACTORY_HPP

#include <bedrock/module.h>
#include <bedrock/ModuleContext.hpp>
#include <bedrock/Exception.hpp>
#include <bedrock/NamedDependency.hpp>
#include <thallium.hpp>
#include <vector>
#include <unordered_map>
#include <string>
#include <iostream>

/**
 * @brief Helper class to register module types into the ModuleContext.
 */
template <typename AbstractServiceFactoryType>
class __BedrockAbstractServiceFactoryRegistration;

namespace bedrock {

/**
 * @brief The Dependency structure is the C++ counterpart of the
 * bedrock_dependency structure in C.
 */
struct Dependency {
    std::string name;
    std::string type;
    int32_t     flags;
};

struct DependencyGroup {
    bool                                          is_array;
    std::vector<std::shared_ptr<NamedDependency>> dependencies;
};

typedef std::unordered_map<std::string, DependencyGroup> ResolvedDependencyMap;

/**
 * @brief This structure is passed to a factory's registerProvider function
 * to provide the factory with relevant information to initialize the provider.
 * The C equivalent of this structure is the bedrock_args_t handle.
 */
struct FactoryArgs {
    std::string              name;         // name of the provider
    thallium::engine         engine;       // thallium engine
    margo_instance_id        mid;          // margo instance id
    uint16_t                 provider_id;  // provider id
    ABT_pool                 pool;         // Argobots pool
    std::vector<std::string> tags;         // Tags
    std::string              config;       // JSON configuration
    ResolvedDependencyMap    dependencies; // dependencies
};

/**
 * @brief Interface for service modules. To build a new module in C++,
 * implement a class MyServiceFactory that inherits from AbstractServiceFactory,
 * and put BEDROCK_REGISTER_MODULE_FACTORY(mymodule, MyServiceFactory);
 * in a cpp file that includes your module class' header file.
 */
class AbstractServiceFactory {

  public:

    /**
     * @brief Constructor.
     */
    AbstractServiceFactory() = default;

    /**
     * @brief Move-constructor.
     */
    AbstractServiceFactory(AbstractServiceFactory&&) = default;

    /**
     * @brief Copy-constructor.
     */
    AbstractServiceFactory(const AbstractServiceFactory&) = default;

    /**
     * @brief Move-assignment operator.
     */
    AbstractServiceFactory& operator=(AbstractServiceFactory&&) = default;

    /**
     * @brief Copy-assignment operator.
     */
    AbstractServiceFactory& operator=(const AbstractServiceFactory&) = default;

    /**
     * @brief Destructor.
     */
    virtual ~AbstractServiceFactory() = default;

    /**
     * @brief Register a provider with the given args. The resulting provider
     * must be cast into a void* and returned. This pointer is what may be
     * passed as dependency to other providers if required.
     *
     * @param args Arguments.
     *
     * @return The provider cast into a void*.
     */
    virtual void* registerProvider(const FactoryArgs& args) = 0;

    /**
     * @brief Deregister a provider.
     *
     * @param provider Provider.
     */
    virtual void deregisterProvider(void* provider) = 0;

    /**
     * @brief Get the configuration of a provider.
     *
     * @return the string configuration.
     */
    virtual std::string getProviderConfig(void* provider) = 0;

    /**
     * @brief Change pool used by a provider.
     *
     * @param provider Provider.
     * @param new_pool New Argobots pool.
     *
     * This function may throw a bedrock::Exception if the change was not possible.
     */
    virtual void changeProviderPool(void* provider, ABT_pool new_pool) {
        (void)provider;
        (void)new_pool;
        throw Exception{"Changing pool not supported for this provider"};
    }

    /**
     * @brief Migrates the state of the designated provider.
     *
     * @param provider Provider to migrate.
     * @param dest_addr Destination address.
     * @param dest_provider_id Destination provider ID.
     * @param options_json JSON-formatted parameters.
     * @param remove_source Whether to remove the source.
     */
    virtual void migrateProvider(
        void* provider, const char* dest_addr,
        uint16_t dest_provider_id,
        const char* options_json, bool remove_source) {
        (void)provider;
        (void)dest_addr;
        (void)dest_provider_id;
        (void)options_json;
        (void)remove_source;
        throw Exception{"Migration not supported for this provider"};
    }

    /**
     * @brief Snapshots the state of the designated provider.
     *
     * @param provider Provider to snapshot.
     * @param dest_path Destination directory.
     * @param options_json JSON-formatted parameters.
     * @param remove_source Whether to remove the source.
     */
    virtual void snapshotProvider(
        void* provider, const char* dest_path,
        const char* options_json, bool remove_source) {
        (void)provider;
        (void)dest_path;
        (void)options_json;
        (void)remove_source;
        throw Exception{"Snapshot not supported for this provider"};
    }

    /**
     * @brief Restores the state of the designated provider.
     *
     * @param provider Provider to snapshot.
     * @param src_path Source directory.
     * @param options_json JSON-formatted parameters.
     */
    virtual void restoreProvider(
        void* provider, const char* src_path,
        const char* options_json) {
        (void)provider;
        (void)src_path;
        (void)options_json;
        throw Exception{"Restore not supported for this provider"};
    }

    /**
     * @brief Register a client for the service.
     *
     * @param args Arguments.
     *
     * @return A client cast into a void*.
     */
    virtual void* initClient(const FactoryArgs& args) = 0;

    /**
     * @brief Deregister a client.
     *
     * @param client Client.
     */
    virtual void finalizeClient(void* client) = 0;

    /**
     * @brief Get the configuration of a client.
     *
     * @return the string configuration.
     */
    virtual std::string getClientConfig(void* client) = 0;

    /**
     * @brief Create a provider handle from the client, and address,
     * and a provider id.
     *
     * @param client Client
     * @param address Address
     * @param provider_id Provider id
     *
     * @return a provider handle cast into a void*.
     */
    virtual void* createProviderHandle(void* client, hg_addr_t address,
                                       uint16_t provider_id)
        = 0;

    /**
     * @brief Destroy a provider handle.
     *
     * @param providerHandle Provider handle.
     */
    virtual void destroyProviderHandle(void* providerHandle) = 0;

    /**
     * @brief Return the dependencies of a provider.
     *
     * Note: if you need the dependencies to depend on the content
     * of the provider's configuration, please override the
     * getProviderDependenciesFromConfig method.
     */
    virtual const std::vector<Dependency>& getProviderDependencies() {
        static const std::vector<Dependency> no_dependencies;
        return no_dependencies;
    }

    /**
     * @brief Return the dependencies of a provider from a desired config.
     *
     * By default this method will ignore the configuration.
     */
    virtual std::vector<Dependency> getProviderDependencies(const char* config) {
        (void)config;
        return getProviderDependencies();
    }

    /**
     * @brief Return the dependencies of a client.
     *
     * Note: if you need the dependencies to depend on the content
     * of the provider's configuration, please override the
     * getProviderDependenciesFromConfig method.
     */
    virtual const std::vector<Dependency>& getClientDependencies() {
        static const std::vector<Dependency> no_dependencies;
        return no_dependencies;
    }

    /**
     * @brief Return the dependencies of a client from a desired config.
     *
     * By default this method will ignore the configuration.
     */
    virtual std::vector<Dependency> getClientDependencies(const char* config) {
        (void)config;
        return getClientDependencies();
    }
};

} // namespace bedrock

#define BEDROCK_REGISTER_MODULE_FACTORY(__module_name, __factory_type) \
    __BedrockAbstractServiceFactoryRegistration<__factory_type>        \
        __bedrock##__module_name##_module(#__module_name);

template <typename AbstractServiceFactoryType>
class __BedrockAbstractServiceFactoryRegistration {

  public:
    __BedrockAbstractServiceFactoryRegistration(const std::string& moduleName) {
        auto factory = std::make_shared<AbstractServiceFactoryType>();
        bedrock::ModuleContext::registerFactory(moduleName, factory);
    }
};

#endif
