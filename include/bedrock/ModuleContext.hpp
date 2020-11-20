/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_MODULE_CONTEXT_HPP
#define __BEDROCK_MODULE_CONTEXT_HPP

#include <bedrock/module.h>
#include <string>
#include <unordered_map>

namespace bedrock {

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
     * @param module Module structure.
     *
     * @return true if the module was registered, false if a module with the
     * same name was already present.
     */
    static bool registerModule(const std::string&    moduleName,
                               const bedrock_module& module);

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

    // ModuleFactory getModuleFactory(const std::string& moduleName);
};

} // namespace bedrock

#endif
