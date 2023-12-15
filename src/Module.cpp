/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <bedrock/module.h>
#include <bedrock/AbstractServiceFactory.hpp>

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

void* bedrock_args_get_dependency(bedrock_args_t args, const char* name,
                                  size_t index) {
    auto a  = reinterpret_cast<bedrock::FactoryArgs*>(args);
    auto it = a->dependencies.find(name);
    if (it == a->dependencies.end()) { return nullptr; }
    if (index >= it->second.dependencies.size()) return nullptr;
    return it->second.dependencies[index]->getHandle<void*>();
}

size_t bedrock_args_get_num_dependencies(bedrock_args_t args,
                                         const char*    name) {
    auto a  = reinterpret_cast<bedrock::FactoryArgs*>(args);
    auto it = a->dependencies.find(name);
    if (it == a->dependencies.end()) { return 0; }
    return it->second.dependencies.size();
}

const char* bedrock_args_get_tag(bedrock_args_t args, size_t index) {
    auto a  = reinterpret_cast<bedrock::FactoryArgs*>(args);
    if(a->tags.size() >= index) return nullptr;
    return a->tags[index].c_str();
}

size_t bedrock_args_get_num_tags(bedrock_args_t args) {
    auto a  = reinterpret_cast<bedrock::FactoryArgs*>(args);
    return a->tags.size();
}

}
