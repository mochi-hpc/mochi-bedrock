/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <bedrock/AbstractComponent.hpp>
#include <iostream>

struct ActualProviderA {};

class ComponentA : public bedrock::AbstractComponent {

    ActualProviderA* m_provider = nullptr;

    public:

    ComponentA()
    : m_provider{new ActualProviderA{}} {}

    ~ComponentA() {
        delete m_provider;
    }

    static std::shared_ptr<bedrock::AbstractComponent>
        Register(const bedrock::ComponentArgs& args) {
            std::cout << "Registering a ComponentA" << std::endl;
            std::cout << " -> mid = " << (void*)args.engine.get_margo_instance() << std::endl;
            std::cout << " -> provider id = " << args.provider_id << std::endl;
            std::cout << " -> config = " << args.config << std::endl;
            std::cout << " -> name = " << args.name << std::endl;
            std::cout << " -> tags = ";
            for(auto& t : args.tags) std::cout << t << " ";
            std::cout << std::endl;
            auto pool_it = args.dependencies.find("pool");
            auto pool = pool_it->second[0]->getHandle<thallium::pool>();
            return std::make_shared<ComponentA>();
    }

    static std::vector<bedrock::Dependency>
        GetDependencies(const bedrock::ComponentArgs& args) {
        (void)args;
        std::vector<bedrock::Dependency> deps = {
            { "pool", "pool", true, false, false }
        };
        return deps;
    }

    void* getHandle() override {
        return static_cast<void*>(m_provider);
    }
};

BEDROCK_REGISTER_COMPONENT_TYPE(module_a, ComponentA)
