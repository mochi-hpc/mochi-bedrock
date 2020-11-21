/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/ModuleContext.hpp"
#include "bedrock/Exception.hpp"
#include "bedrock/ServiceFactory.hpp"
#include "DynLibServiceFactory.hpp"
#include <bedrock/module.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace bedrock {

using nlohmann::json;

static std::unordered_map<std::string, std::unique_ptr<AbstractServiceFactory>> s_modules;

bool ModuleContext::registerModule(const std::string&    moduleName,
                                   const bedrock_module& module) {
    return registerFactory(moduleName, std::make_unique<DynLibServiceFactory>(module));
}

bool ModuleContext::registerFactory(const std::string& moduleName,
        std::unique_ptr<AbstractServiceFactory>&& factory) {
    if (s_modules.find(moduleName) != s_modules.end()) return false;
    s_modules[moduleName] = std::move(factory);
    return true;
}

bool ModuleContext::loadModule(const std::string& moduleName,
                               const std::string& library) {
    if (s_modules.find(moduleName) != s_modules.end()) return false;
    std::unique_ptr<AbstractServiceFactory> factory =
        std::make_unique<DynLibServiceFactory>(moduleName, library);
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

} // namespace bedrock
