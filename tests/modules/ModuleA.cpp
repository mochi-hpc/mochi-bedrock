#include "BaseModule.hpp"

class ModuleAServiceFactory : public BaseServiceFactory {

};

BEDROCK_REGISTER_MODULE_FACTORY(module_a, ModuleAServiceFactory)
