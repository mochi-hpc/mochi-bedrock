/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <bedrock/AbstractServiceFactory.hpp>
#include <iostream>

class ServiceBFactory : public bedrock::AbstractServiceFactory {

    public:

    ServiceBFactory() {
        m_provider_dependencies.push_back({
            "ssg_group", "ssg", BEDROCK_REQUIRED });
        m_provider_dependencies.push_back({
            "mona_instance", "mona", BEDROCK_REQUIRED });
        m_provider_dependencies.push_back({
            "a_provider", "module_a", BEDROCK_REQUIRED | BEDROCK_KIND_PROVIDER});
        m_provider_dependencies.push_back({
            "a_provider_handle", "module_a", BEDROCK_REQUIRED | BEDROCK_ARRAY | BEDROCK_KIND_PROVIDER_HANDLE});
        m_provider_dependencies.push_back({
            "a_client", "module_a", BEDROCK_REQUIRED | BEDROCK_KIND_CLIENT});
    }

    void* registerProvider(const bedrock::FactoryArgs& args) override {
        std::cout << "Registering a provider from module B" << std::endl;
        std::cout << " -> mid         = " << (void*)args.mid << std::endl;
        std::cout << " -> provider_id = " << args.provider_id << std::endl;
        std::cout << " -> pool        = " << (void*)args.pool << std::endl;
        std::cout << " -> config      = " << args.config << std::endl;
        std::cout << " -> name        = " << args.name << std::endl;
        return (void*)0x1;
    }

    void deregisterProvider(void* provider) override {
        (void)provider;
        std::cout << "Deregistring provider from module B" << std::endl;
    }

    std::string getProviderConfig(void* provider) override {
        (void)provider;
        return "{}";
    }

    void* initClient(const bedrock::FactoryArgs& args) override {
        (void)args;
        std::cout << "Initializing client from module B" << std::endl;
        return (void*)0x2;
    }

    void finalizeClient(void* client) override {
        (void)client;
        std::cout << "Finalizing client from module B" << std::endl;
    }

    std::string getClientConfig(void* client) override {
        (void)client;
        return "{}";
    }

    void* createProviderHandle(void* client, hg_addr_t address, uint16_t provider_id) override {
        (void)client;
        (void)address;
        (void)provider_id;
        std::cout << "Creating a provider handle from module B" << std::endl;
        return (void*)0x3;
    }

    void destroyProviderHandle(void* providerHandle) override {
        (void)providerHandle;
        std::cout << "Destroying provider handle from module B" << std::endl;
    }

    const std::vector<bedrock::Dependency>& getProviderDependencies() override {
        return m_provider_dependencies;
    }

    const std::vector<bedrock::Dependency>& getClientDependencies() override {
        return m_client_dependencies;
    }

    private:

    std::vector<bedrock::Dependency> m_provider_dependencies;
    std::vector<bedrock::Dependency> m_client_dependencies;
};

BEDROCK_REGISTER_MODULE_FACTORY(module_b, ServiceBFactory)
