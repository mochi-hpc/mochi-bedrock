#include "BaseModule.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ModuleCServiceFactory : public BaseServiceFactory {

    static inline std::vector<bedrock::Dependency> extractDependencies(
            const json& expected_dependencies) {
        if(!expected_dependencies.is_array())
            return {};
        std::vector<bedrock::Dependency> dependencies;
        for(const auto& dep : expected_dependencies) {
            if(!dep.is_object()) continue;
            if(!dep.contains("name") || !dep.contains("type"))
                continue;
            int32_t flags = 0;
            auto name = dep["name"].get<std::string>();
            auto type = dep["type"].get<std::string>();
            auto kind = dep.value("kind", std::string{""});
            if(kind == "client") {
                flags |= BEDROCK_KIND_CLIENT;
            }
            if(kind == "provider") {
                flags |= BEDROCK_KIND_PROVIDER;
            }
            if(kind == "provider_handle") {
                flags |= BEDROCK_KIND_PROVIDER_HANDLE;
            }
            if(dep.value("is_required", false)) {
                flags |= BEDROCK_REQUIRED;
            }
            if(dep.value("is_array", false)) {
                flags |= BEDROCK_ARRAY;
            }
            dependencies.push_back(bedrock::Dependency{name, type, flags});
        }
        return dependencies;
    }

    std::vector<bedrock::Dependency> getProviderDependencies(const char* cfg) override {
        auto config = json::parse(cfg);
        std::vector<bedrock::Dependency> dependencies;
        if(!config.contains("expected_provider_dependencies"))
            return dependencies;
        auto& expected_dependencies = config["expected_provider_dependencies"];
        return extractDependencies(expected_dependencies);
    }

    std::vector<bedrock::Dependency> getClientDependencies(const char* cfg) override {
        auto config = json::parse(cfg);
        std::vector<bedrock::Dependency> dependencies;
        if(!config.contains("expected_client_dependencies"))
            return dependencies;
        auto& expected_dependencies = config["expected_client_dependencies"];
        return extractDependencies(expected_dependencies);
    }

};

BEDROCK_REGISTER_MODULE_FACTORY(module_c, ModuleCServiceFactory)
