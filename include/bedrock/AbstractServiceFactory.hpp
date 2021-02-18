/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_ABSTRACT_SERVICE_FACTORY_HPP
#define __BEDROCK_ABSTRACT_SERVICE_FACTORY_HPP

#include <bedrock/module.h>
#include <bedrock/ModuleContext.hpp>
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

struct ResolvedDependency : public Dependency {
    std::string spec;
    void*       handle;
};

typedef std::unordered_map<std::string, std::vector<ResolvedDependency>>
    ResolvedDependencyMap;

/**
 * @brief This structure is passed to a factory's registerProvider function
 * to provide the factory with relevant information to initialize the provider.
 * The C equivalent of this structure is the bedrock_args_t handle.
 */
struct FactoryArgs {
    std::string           name;         // name of the provider
    margo_instance_id     mid;          // margo instance
    uint16_t              provider_id;  // provider id
    ABT_pool              pool;         // Argobots pool
    std::string           config;       // JSON configuration
    ResolvedDependencyMap dependencies; // dependencies
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
     */
    virtual const std::vector<Dependency>& getProviderDependencies() = 0;

    /**
     * @brief Return the dependencies of a client.
     */
    virtual const std::vector<Dependency>& getClientDependencies() = 0;
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
