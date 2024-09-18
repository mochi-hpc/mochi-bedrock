#include "Helpers.hpp"
#include <iostream>

class BaseComponent : public bedrock::AbstractComponent {

    std::unique_ptr<TestProvider> m_provider;

    public:

    BaseComponent(const bedrock::ComponentArgs& args)
    : m_provider{std::make_unique<TestProvider>(args)} {}

    virtual ~BaseComponent() = default;

    void* getHandle() override {
        return static_cast<void*>(m_provider.get());
    }

    static std::shared_ptr<bedrock::AbstractComponent>
        Register(const bedrock::ComponentArgs& args) {
            return std::make_shared<BaseComponent>(args);
        }

    static std::vector<bedrock::Dependency>
        GetDependencies(const bedrock::ComponentArgs& args) {
            return std::vector<bedrock::Dependency>{};
        }
};
