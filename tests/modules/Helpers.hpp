#ifndef BEDROCK_TEST_HELPER
#define BEDROCK_TEST_HELPER

#include <string>
#include <vector>
#include <bedrock/AbstractServiceFactory.hpp>

struct TestProvider {

    std::string                            name;
    margo_instance_id                      mid;
    uint16_t                               provider_id;
    ABT_pool                               pool;
    std::string                            config;
    std::unordered_map<std::string, std::vector<void*>> dependencies;

    TestProvider(const bedrock::FactoryArgs& args)
    : name(args.name)
    , mid(args.mid)
    , provider_id(args.provider_id)
    , pool(args.pool)
    , config(args.config)
    {
        for(const auto& dep : args.dependencies) {
            for(const auto& a : dep.second.dependencies) {
                dependencies[dep.first].push_back(a->getHandle<void*>());
            }
        }
    }
};

struct TestClient {

    std::string                            name;
    margo_instance_id                      mid;
    uint16_t                               provider_id;
    ABT_pool                               pool;
    std::string                            config;
    std::unordered_map<std::string, std::vector<void*>> dependencies;

    TestClient(const bedrock::FactoryArgs& args)
    : name(args.name)
    , mid(args.mid)
    , provider_id(args.provider_id)
    , pool(args.pool)
    , config(args.config)
    {
        for(const auto& dep : args.dependencies) {
            for(const auto& a : dep.second.dependencies) {
                dependencies[dep.first].push_back(a->getHandle<void*>());
            }
        }
    }
};

struct TestProviderHandle {

    TestClient* client;
    hg_addr_t   addr = HG_ADDR_NULL;
    uint16_t    provider_id;

    TestProviderHandle(void* c, hg_addr_t a, uint16_t pr_id)
    : client(static_cast<TestClient*>(c))
    , provider_id(pr_id)
    {
        margo_addr_dup(client->mid, a, &addr);
    }

    ~TestProviderHandle()
    {
        margo_addr_free(client->mid, addr);
    }

};

#endif
