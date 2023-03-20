#include "BaseModule.hpp"

class ModuleBServiceFactory : public BaseServiceFactory {

};

BEDROCK_REGISTER_MODULE_FACTORY(module_b, ModuleBServiceFactory)
