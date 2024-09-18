#include "BaseModule.hpp"

class ComponentB : public BaseComponent {

    public:

    using BaseComponent::Register;
    using BaseComponent::GetDependencies;
};

BEDROCK_REGISTER_COMPONENT_TYPE(module_b, ComponentB)
