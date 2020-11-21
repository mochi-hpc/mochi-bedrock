/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <bedrock/module.h>
#include <bedrock/ServiceFactory.hpp>

extern "C" {

const char* bedrock_args_get_name(bedrock_args_t args) {
    auto a = reinterpret_cast<bedrock::FactoryArgs*>(args);
    return a->name.c_str();
}

margo_instance_id bedrock_args_get_margo_instance(bedrock_args_t args) {
    auto a = reinterpret_cast<bedrock::FactoryArgs*>(args);
    return a->mid;
}

uint16_t bedrock_args_get_provider_id(bedrock_args_t args) {
    auto a = reinterpret_cast<bedrock::FactoryArgs*>(args);
    return a->provider_id;
}

ABT_pool bedrock_args_get_pool(bedrock_args_t args) {
    auto a = reinterpret_cast<bedrock::FactoryArgs*>(args);
    return a->pool;
}

const char* bedrock_args_get_config(bedrock_args_t args) {
    auto a = reinterpret_cast<bedrock::FactoryArgs*>(args);
    return a->config.c_str();
}

void* bedrock_args_get_dependency(bedrock_args_t args, const char* name) {
    auto a  = reinterpret_cast<bedrock::FactoryArgs*>(args);
    auto it = a->dependencies.find(name);
    if (it == a->dependencies.end()) { return nullptr; }
    return it->second;
}
}
