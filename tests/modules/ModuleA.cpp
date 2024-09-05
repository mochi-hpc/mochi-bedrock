#include "BaseModule.hpp"

class ComponentA : public BaseComponent {

    public:

    using BaseComponent::Register;
    using BaseComponent::GetDependencies;
};

BEDROCK_REGISTER_COMPONENT_TYPE(module_a, ComponentA)
