/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_DYN_LIB_SERVICE_FACTORY_H
#define __BEDROCK_DYN_LIB_SERVICE_FACTORY_H

#include "bedrock/module.h"
#include "bedrock/AbstractServiceFactory.hpp"
#include "bedrock/Exception.hpp"
#include <spdlog/spdlog.h>
#include <dlfcn.h>

namespace bedrock {

class DynLibServiceFactory : public AbstractServiceFactory {

  public:
    /**
     * @brief Constructor.
     */
    DynLibServiceFactory(const bedrock_module& mod)
    : m_handle(nullptr), m_module(mod) {}

    /**
     * @brief Constructor.
     */
    DynLibServiceFactory(const std::string& moduleName, void* handle) {
        m_handle         = handle;
        auto symbol_name = moduleName + "_bedrock_init";
        typedef void (*module_init_fn)(bedrock_module*);
        module_init_fn init_module
            = (module_init_fn)dlsym(m_handle, symbol_name.c_str());
        if (!init_module) {
            std::string error = dlerror();
            dlclose(m_handle);
            throw Exception("Could not load {} module: {}", moduleName, error);
        }
        init_module(&m_module);
    }

    /**
     * @brief Move-constructor.
     */
    DynLibServiceFactory(DynLibServiceFactory&& other)
    : m_handle(other.m_handle), m_module(other.m_module) {
        other.m_handle = nullptr;
    }

    /**
     * @brief Copy-constructor.
     */
    DynLibServiceFactory(const DynLibServiceFactory&) = delete;

    /**
     * @brief Move-assignment operator.
     */
    DynLibServiceFactory& operator=(DynLibServiceFactory&&) = delete;

    /**
     * @brief Copy-assignment operator.
     */
    DynLibServiceFactory& operator=(const DynLibServiceFactory&) = delete;

    /**
     * @brief Destructor.
     */
    virtual ~DynLibServiceFactory() {
        if (m_handle) dlclose(m_handle);
    }

    /**
     * @brief Register a provider with the given args. The resulting provider
     * must be cast into a void* and returned. This pointer is what may be
     * passed as dependency to other providers if required.
     *
     * @param args Arguments.
     *
     * @return The provider cast into a void*.
     */
    void* registerProvider(const FactoryArgs& args) override {
        void* provider = nullptr;
        auto  a
            = reinterpret_cast<bedrock_args_t>(&const_cast<FactoryArgs&>(args));
        int ret = m_module.register_provider(a, &provider);
        if (ret != BEDROCK_SUCCESS) {
            throw Exception("Module register_provider function returned {}",
                            ret);
        }
        return provider;
    }

    /**
     * @brief Deregister a provider.
     *
     * @param provider Provider.
     */
    void deregisterProvider(void* provider) override {
        int ret = m_module.deregister_provider(provider);
        if (ret != BEDROCK_SUCCESS) {
            throw Exception("Module deregister_provider function returned {}",
                            ret);
        }
    }

    /**
     * @brief Register a client for the service.
     *
     * @param mid Margo instance.
     *
     * @return A client cast into a void*.
     */
    void* initClient(margo_instance_id mid) override {
        void* client = nullptr;
        int   ret    = m_module.init_client(mid, &client);
        if (ret != BEDROCK_SUCCESS) {
            throw Exception("Module register_client function returned {}", ret);
        }
        return client;
    }

    /**
     * @brief Deregister a client.
     *
     * @param client Client.
     */
    void finalizeClient(void* client) override {
        int ret = m_module.finalize_client(client);
        if (ret != BEDROCK_SUCCESS) {
            throw Exception("Module finalize_client function returned {}", ret);
        }
    }

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
    void* createProviderHandle(void* client, hg_addr_t address,
                               uint16_t provider_id) override {
        void* ph = nullptr;
        int ret  = m_module.create_provider_handle(client, address, provider_id,
                                                  &ph);
        if (ret != BEDROCK_SUCCESS) {
            throw Exception(
                "Module create_provider_handle function returned {}", ret);
        }
        return ph;
    }

    /**
     * @brief Destroy a provider handle.
     *
     * @param providerHandle Provider handle.
     */
    void destroyProviderHandle(void* providerHandle) override {
        int ret = m_module.destroy_provider_handle(providerHandle);
        if (ret != BEDROCK_SUCCESS) {
            throw Exception(
                "Module destroy_provider_handle function returned {}", ret);
        }
    }

    /**
     * @brief Return the dependencies of a provider.
     */
    std::vector<const bedrock_dependency*> getDependencies() override {
        std::vector<const bedrock_dependency*> deps;
        if (m_module.dependencies == nullptr) { return deps; }
        int i = 0;
        while (m_module.dependencies[i].name != nullptr) {
            deps.push_back(&m_module.dependencies[i]);
        }
        return deps;
    }

  private:
    void*          m_handle = nullptr;
    bedrock_module m_module;
};

} // namespace bedrock

#endif
