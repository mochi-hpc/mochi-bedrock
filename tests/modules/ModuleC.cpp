#include "Helpers.hpp"
#include <nlohmann/json.hpp>

class ComponentC : public bedrock::AbstractComponent {

    using json = nlohmann::json;

    std::unique_ptr<TestProvider> m_provider;

    public:

    ComponentC(const bedrock::ComponentArgs& args)
    : m_provider{std::make_unique<TestProvider>(args)} {}

    void* getHandle() override {
        return static_cast<void*>(m_provider.get());
    }

    static std::shared_ptr<bedrock::AbstractComponent>
        Register(const bedrock::ComponentArgs& args) {
            return std::make_shared<ComponentC>(args);
        }

    static std::vector<bedrock::Dependency>
        GetDependencies(const bedrock::ComponentArgs& args) {
            auto config = json::parse(args.config);
            std::vector<bedrock::Dependency> dependencies;
            if(!config.contains("expected_provider_dependencies"))
                return dependencies;
            auto& expected_dependencies = config["expected_provider_dependencies"];
            return extractDependencies(expected_dependencies);
        }

    private:

    static inline std::vector<bedrock::Dependency> extractDependencies(
            const json& expected_dependencies) {
        if(!expected_dependencies.is_array())
            return {};
        std::vector<bedrock::Dependency> dependencies;
        for(const auto& dep : expected_dependencies) {
            if(!dep.is_object()) continue;
            if(!dep.contains("name") || !dep.contains("type"))
                continue;
            bedrock::Dependency the_dep;
            the_dep.name = dep["name"].get<std::string>();
            the_dep.type = dep["type"].get<std::string>();
            the_dep.is_array = dep.value("is_array", false);
            the_dep.is_required = dep.value("is_required", false);
            the_dep.is_updatable = dep.value("is_updatable", false);
            dependencies.push_back(the_dep);
        }
        return dependencies;
    }

};

BEDROCK_REGISTER_COMPONENT_TYPE(module_c, ComponentC)
