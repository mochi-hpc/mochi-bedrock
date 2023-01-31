/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_MODULE_CONTEXT_HPP
#define __BEDROCK_MODULE_CONTEXT_HPP

#include <bedrock/module.h>
#include <string>
#include <memory>
#include <unordered_map>

namespace bedrock {

class AbstractServiceFactory;

/**
 * @brief The ModuleContext class contains functions to load modules.
 * Since modules are handled by dynamic libraries, the ModuleContext
 * class provides only static functions and manages these dynamic libraries
 * globally.
 */
class ModuleContext {

  public:
    ModuleContext() = delete;

    /**
     * @brief Register a module that is already in the program's memory.
     *
     * @param moduleName Module name.
     * @param mod Module structure.
     *
     * @return true if the module was registered, false if a module with the
     * same name was already present.
     */
    static bool registerModule(const std::string&        moduleName,
                               const bedrock_module_v1& mod);

    /**
     * @brief Register a factory. This function is used mainly by the
     * BEDROCK_REGISTER_MODULE_FACTORY macro in ServiceFactory.hpp to
     * automatically register C++ modules.
     *
     * @param moduleName Module name.
     * @param factory Factory.
     *
     * @return true if the module factory was register, false if a factory
     * with the same name was already present.
     */
    static bool
    registerFactory(const std::string&                             moduleName,
                    const std::shared_ptr<AbstractServiceFactory>& factory);

    /**
     * @brief Load a module with a given name from the specified library file.
     *
     * @param moduleName Module name.
     * @param library Library.
     *
     * @return true if the module was registered, false if a module with the
     * same name was already present.
     */
    static bool loadModule(const std::string& moduleName,
                           const std::string& library);

    /**
     * @brief Load multiple modules. The provided unordered map maps module
     * names to libraries.
     *
     * @param modules Modules.
     */
    static void
    loadModules(const std::unordered_map<std::string, std::string>& modules);

    /**
     * @brief Load modules from a JSON-formatted object mapping module names to
     * libraries.
     *
     * @param jsonString JSON string.
     */
    static void loadModulesFromJSON(const std::string& jsonString);

    /**
     * @brief Get an AbstractServiceFactory from a given module name.
     *
     * @param moduleName Module name.
     *
     * @return An AbstractServiceFactory, or nullptr if not found.
     */
    static AbstractServiceFactory*
    getServiceFactory(const std::string& moduleName);

    /**
     * @brief Return the current JSON configuration.
     */
    static std::string getCurrentConfig();
};

} // namespace bedrock

#endif
