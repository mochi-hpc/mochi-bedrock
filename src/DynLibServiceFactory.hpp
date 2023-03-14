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
    DynLibServiceFactory(const bedrock_module_v2& mod)
    : m_handle(nullptr), m_module(mod) {}

    DynLibServiceFactory(const bedrock_module_v1& mod)
    : m_handle(nullptr) {
        memset(&m_module, 0, sizeof(m_module));
        memcpy(&m_module, &mod, sizeof(mod));
    }

    DynLibServiceFactory(const std::string& moduleName, void* handle) {
        m_handle         = handle;
        auto symbol_name = moduleName + "_bedrock_init";
        typedef void (*module_init_fn)(bedrock_module_v1**);
        module_init_fn init_module
            = (module_init_fn)dlsym(m_handle, symbol_name.c_str());
        if (!init_module) {
            std::string error = dlerror();
            dlclose(m_handle);
            throw Exception("Could not load {} module: {}", moduleName, error);
        }
        bedrock_module_v1* module_v1_ptr = nullptr;
        init_module(&module_v1_ptr);
        // handle actual module version
        if(module_v1_ptr->api_version == 1) {
            memset(&m_module, 0, sizeof(m_module));
            memcpy(&m_module, module_v1_ptr, sizeof(*module_v1_ptr));
        } else { // version 2
            bedrock_module_v2* module_v2_ptr
                = reinterpret_cast<bedrock_module_v2*>(module_v1_ptr);
            memset(&m_module, 0, sizeof(m_module));
            memcpy(&m_module, module_v2_ptr, sizeof(*module_v2_ptr));
        };
        if (m_module.provider_dependencies) {
            int i = 0;
            while (m_module.provider_dependencies[i].name != nullptr) {
                Dependency d;
                d.name  = m_module.provider_dependencies[i].name;
                d.type  = m_module.provider_dependencies[i].type;
                d.flags = m_module.provider_dependencies[i].flags;
                m_provider_dependencies.push_back(d);
                i++;
            }
        }
        if (m_module.client_dependencies) {
            int i = 0;
            while (m_module.client_dependencies[i].name != nullptr) {
                Dependency d;
                d.name  = m_module.client_dependencies[i].name;
                d.type  = m_module.client_dependencies[i].type;
                d.flags = m_module.client_dependencies[i].flags;
                m_client_dependencies.push_back(d);
                i++;
            }
        }
    }

    DynLibServiceFactory(DynLibServiceFactory&& other)
    : m_handle(other.m_handle), m_module(other.m_module) {
        other.m_handle = nullptr;
    }

    DynLibServiceFactory(const DynLibServiceFactory&) = delete;

    DynLibServiceFactory& operator=(DynLibServiceFactory&&) = delete;

    DynLibServiceFactory& operator=(const DynLibServiceFactory&) = delete;

    virtual ~DynLibServiceFactory() {
        if (m_handle) dlclose(m_handle);
    }

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

    void deregisterProvider(void* provider) override {
        int ret = m_module.deregister_provider(provider);
        if (ret != BEDROCK_SUCCESS) {
            throw Exception("Module deregister_provider function returned {}",
                            ret);
        }
    }

    std::string getProviderConfig(void* provider) override {
        if (!m_module.get_provider_config) return std::string("{}");
        auto config = m_module.get_provider_config(provider);
        if (!config) return std::string("{}");
        auto config_str = std::string(config);
        free(config);
        return config_str;
    }

    void changeProviderPool(void* provider, ABT_pool new_pool) override {
        if (!m_module.change_provider_pool)
            throw Exception{"Changing pool not supported for this provider"};
        int ret = m_module.change_provider_pool(provider, new_pool);
        if (ret != 0) {
            throw Exception{
                "Provider's change_provider_pool callback"
                " failed with error code {}", ret};
        }
    }

    void* initClient(const FactoryArgs& args) override {
        void* client = nullptr;
        int   ret    = m_module.init_client(
            reinterpret_cast<bedrock_args_t>(const_cast<FactoryArgs*>(&args)),
            &client);
        if (ret != BEDROCK_SUCCESS) {
            throw Exception("Module register_client function returned {}", ret);
        }
        return client;
    }

    void finalizeClient(void* client) override {
        int ret = m_module.finalize_client(client);
        if (ret != BEDROCK_SUCCESS) {
            throw Exception("Module finalize_client function returned {}", ret);
        }
    }

    std::string getClientConfig(void* client) override {
        if (!m_module.get_client_config) return std::string("{}");
        auto config = m_module.get_client_config(client);
        if (!config) return std::string("{}");
        auto config_str = std::string(config);
        free(config);
        return config_str;
    }

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

    void destroyProviderHandle(void* providerHandle) override {
        int ret = m_module.destroy_provider_handle(providerHandle);
        if (ret != BEDROCK_SUCCESS) {
            throw Exception(
                "Module destroy_provider_handle function returned {}", ret);
        }
    }

    const std::vector<Dependency>& getProviderDependencies() override {
        return m_provider_dependencies;
    }

    const std::vector<Dependency>& getClientDependencies() override {
        return m_client_dependencies;
    }

  private:
    void*                   m_handle = nullptr;
    bedrock_module_v2       m_module;
    std::vector<Dependency> m_provider_dependencies;
    std::vector<Dependency> m_client_dependencies;
};

} // namespace bedrock

#endif
