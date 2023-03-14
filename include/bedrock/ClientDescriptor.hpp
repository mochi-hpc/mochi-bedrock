/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_CLIENT_DESC_HPP
#define __BEDROCK_CLIENT_DESC_HPP

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

} // namespace bedrock

#endif
