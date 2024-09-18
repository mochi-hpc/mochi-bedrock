#ifndef BEDROCK_TEST_HELPER
#define BEDROCK_TEST_HELPER

#include <string>
#include <vector>
#include <bedrock/AbstractComponent.hpp>

struct TestProvider {

    std::string                          name;
    thallium::engine                     engine;
    uint16_t                             provider_id;
    std::string                          config;
    std::unordered_map<
        std::string, std::vector<std::string>> dependencies;

    TestProvider(const bedrock::ComponentArgs& args)
    : name(args.name)
    , engine(args.engine)
    , provider_id(args.provider_id)
    , config(args.config)
    {
        for(const auto& dep : args.dependencies) {
            for(const auto& a : dep.second) {
                dependencies[dep.first].push_back(a->getName());
            }
        }
    }
};

#endif
