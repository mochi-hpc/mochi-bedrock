/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <bedrock/AbstractComponent.hpp>
#include <iostream>

struct ActualProviderA;

struct ActualProviderB {};

class ComponentB : public bedrock::AbstractComponent {

    ActualProviderB* m_provider = nullptr;

    public:

    ComponentB()
    : m_provider{new ActualProviderB{}} {}

    ~ComponentB() {
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
            std::cout << " -> pool = " << pool.native_handle() << std::endl;
            auto a_provider_it = args.dependencies.find("a_provider");
            auto a_provider_comp_handle = a_provider_it->second[0]->getHandle<bedrock::ComponentPtr>();
            auto a_provider = a_provider_comp_handle->getHandle();
            std::cout << " -> a_provider = " << a_provider << std::endl;
            auto a_ph_it = args.dependencies.find("a_provider_handles");
            int i = 0;
            for(auto& p : a_ph_it->second) {
                auto ph = p->getHandle<thallium::provider_handle>();
                std::cout << " -> a_provider_handles[" << i << "] = " <<
                    static_cast<std::string>(ph) << " with provider id " << ph.provider_id() << std::endl;
            }
            return std::make_shared<ComponentB>();
    }

    static std::vector<bedrock::Dependency>
        GetDependencies(const bedrock::ComponentArgs& args) {
        (void)args;
        std::vector<bedrock::Dependency> deps = {
            { "pool", "pool", true, false, false },
            { "a_provider", "module_a", true, false, false },
            { "a_provider_handles", "module_a", false, true, false }
        };
        return deps;
    }

    void* getHandle() override {
        return static_cast<void*>(m_provider);
    }
};

BEDROCK_REGISTER_COMPONENT_TYPE(module_a, ComponentB)
