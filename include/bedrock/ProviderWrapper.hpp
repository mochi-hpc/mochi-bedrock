/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_PROVIDER_WRAPPER_HPP
#define __BEDROCK_PROVIDER_WRAPPER_HPP

#include <string>
#include <thallium/serialization/stl/string.hpp>

namespace bedrock {

class AbstractServiceFactory;

/**
 * @brief A ProviderDescriptor is an object that describes a provider.
 */
struct ProviderDescriptor {

    std::string name;        // name of the provider
    std::string type;        // name of the module
    uint16_t    provider_id; // provider id

    template <typename A> void serialize(A& ar) { ar(name, type, provider_id); }
};

/**
 * @brief A ProviderWrapper is an object that wraps a void*
 * pointer representing a provider created by a module.
 */
struct ProviderWrapper : ProviderDescriptor {

    void*                   handle  = nullptr; // provider
    AbstractServiceFactory* factory = nullptr; // factory
};

} // namespace bedrock

#endif
