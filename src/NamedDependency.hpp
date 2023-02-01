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

    NamedDependency(std::string name)
    : m_name(std::move(name)) {}

    virtual ~NamedDependency() = default;

    const std::string& name() const {
        return m_name;
    }

    std::string& name() {
        return m_name;
    }

    protected:

    std::string m_name;
};

} // namespace bedrock

#endif
