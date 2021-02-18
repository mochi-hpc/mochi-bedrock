/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_CLIENT_WRAPPER_HPP
#define __BEDROCK_CLIENT_WRAPPER_HPP

#include <string>
#include <thallium/serialization/stl/string.hpp>

namespace bedrock {

class AbstractServiceFactory;

/**
 * @brief A ClientDescriptor is an object that describes a client.
 */
struct ClientDescriptor {

    std::string name; // name of the client
    std::string type; // name of the module

    template <typename A> void serialize(A& ar) { ar(name, type); }
};

/**
 * @brief A ClientWrapper is an object that wraps a void*
 * pointer representing a client created by a module.
 */
struct ClientWrapper : ClientDescriptor {

    void*                   handle  = nullptr; // client
    AbstractServiceFactory* factory = nullptr; // factory
};

} // namespace bedrock

#endif
