/*
 * (C) 2023 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_NAMED_DEPENDENCY_H
#define __BEDROCK_NAMED_DEPENDENCY_H

#include <string>
#include <memory>

namespace bedrock {

class NamedDependency {

    public:

    NamedDependency() = default;

    virtual ~NamedDependency() = default;

    virtual const std::string& getName() const = 0;
};

} // namespace bedrock

#endif
