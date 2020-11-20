/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/ModuleContext.hpp"
#include "bedrock/Exception.hpp"
#include <bedrock/module.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <dlfcn.h>

namespace bedrock {

using nlohmann::json;

static std::unordered_map<std::string, bedrock_module> s_modules;

bool ModuleContext::registerModule(const std::string&    moduleName,
                                   const bedrock_module& module) {
    if (s_modules.find(moduleName) != s_modules.end()) return false;
    s_modules[moduleName] = module;
    return true;
}

bool ModuleContext::loadModule(const std::string& moduleName,
                               const std::string& library) {
    if (s_modules.find(moduleName) != s_modules.end()) return false;
    spdlog::trace("Loading module {} from library {}", moduleName, library);
    void* handle = nullptr;
    if (library == "")
        handle = dlopen(nullptr, RTLD_NOW | RTLD_GLOBAL);
    else
        handle = dlopen(library.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (!handle)
        throw Exception("Could not dlopen library {}: {}", library, dlerror());
    auto symbol_name = moduleName + "_bedrock_module";
    typedef void (*module_init_fn)(bedrock_module*);
    module_init_fn init_module
        = (module_init_fn)dlsym(handle, symbol_name.c_str());
    if (!init_module) {
        std::string error = dlerror();
        dlclose(handle);
        throw Exception("Could not load {} module: {}", moduleName, error);
    }
    bedrock_module m;
    init_module(&m);
    return registerModule(moduleName, m);
}

void ModuleContext::loadModules(
    const std::unordered_map<std::string, std::string>& modules) {
    for (const auto& m : modules) loadModule(m.first, m.second);
}

void ModuleContext::loadModulesFromJSON(const std::string& jsonString) {
    auto modules = json::parse(jsonString);
    if (modules.is_null()) return;
    if (!modules.is_object())
        throw Exception(
            "JSON configuration for ModuleContext should be an object");
    for (auto& mod : modules.items()) {
        if (!(mod.value().is_string() || mod.value().is_null())) {
            throw Exception("Module library for {} should be a string or null",
                            mod.key());
        }
    }
    for (auto& mod : modules.items()) {
        if (mod.value().is_string()) {
            loadModule(mod.key(), mod.value().get<std::string>());
        } else {
            loadModule(mod.key(), "");
        }
    }
}

} // namespace bedrock
