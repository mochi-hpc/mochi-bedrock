/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/ModuleContext.hpp"
#include "bedrock/Exception.hpp"
#include "bedrock/AbstractServiceFactory.hpp"
#include "DynLibServiceFactory.hpp"
#include <bedrock/module.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <dlfcn.h>

namespace bedrock {

using nlohmann::json;

static std::unordered_map<std::string, std::string> s_libraries;
static std::unordered_map<std::string, std::shared_ptr<AbstractServiceFactory>>
    s_modules;

bool ModuleContext::registerModule(const std::string&    moduleName,
                                   const bedrock_module& module) {
    return registerFactory(moduleName,
                           std::make_shared<DynLibServiceFactory>(module));
}

bool ModuleContext::registerFactory(
    const std::string&                             moduleName,
    const std::shared_ptr<AbstractServiceFactory>& factory) {
    if (s_modules.find(moduleName) != s_modules.end()) return false;
    s_modules[moduleName]   = std::move(factory);
    s_libraries[moduleName] = "";
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
    s_libraries[moduleName] = library;
    // C++ libraries will have registered themselves automatically
    if (s_modules.find(moduleName) != s_modules.end()) return true;

    // otherwise, we need to create a DynLibServiceFactory
    std::shared_ptr<AbstractServiceFactory> factory
        = std::make_shared<DynLibServiceFactory>(moduleName, handle);
    s_modules[moduleName] = std::move(factory);
    return true;
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

AbstractServiceFactory*
ModuleContext::getServiceFactory(const std::string& moduleName) {
    auto it = s_modules.find(moduleName);
    if (it == s_modules.end())
        return nullptr;
    else
        return it->second.get();
}

std::string ModuleContext::getCurrentConfig() {
    std::string config = "{";
    unsigned    i      = 0;
    for (const auto& m : s_libraries) {
        config += "\"" + m.first + "\":\"" + m.second + "\"";
        i += 1;
        if (i < s_libraries.size()) config += ",";
    }
    config += "}";
    return config;
}

} // namespace bedrock
