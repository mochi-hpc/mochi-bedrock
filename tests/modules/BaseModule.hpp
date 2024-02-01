#include "Helpers.hpp"
#include <iostream>

class BaseServiceFactory : public bedrock::AbstractServiceFactory {

    public:

    BaseServiceFactory() = default;

    virtual ~BaseServiceFactory() = default;

    void* registerProvider(const bedrock::FactoryArgs& args) override {
        return new TestProvider(args);
    }

    void deregisterProvider(void* provider) override {
        delete static_cast<TestProvider*>(provider);
    }

    std::string getProviderConfig(void* provider) override {
        return static_cast<TestProvider*>(provider)->config;
    }

    void* initClient(const bedrock::FactoryArgs& args) override {
        return new TestClient(args);
    }

    void finalizeClient(void* client) override {
        delete static_cast<TestClient*>(client);
    }

    std::string getClientConfig(void* client) override {
        return static_cast<TestClient*>(client)->config;
    }

    void* createProviderHandle(void* client, hg_addr_t address, uint16_t provider_id) override {
        return new TestProviderHandle(client, address, provider_id);
    }

    void destroyProviderHandle(void* providerHandle) override {
        delete static_cast<TestProviderHandle*>(providerHandle);
    }

    std::vector<bedrock::Dependency> getProviderDependencies(const char*) override {
        return {};
    }

    std::vector<bedrock::Dependency> getClientDependencies(const char*) override {
        return {};
    }
};
