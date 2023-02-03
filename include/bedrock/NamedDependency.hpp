/*
 * (C) 2023 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_NAMED_DEPENDENCY_H
#define __BEDROCK_NAMED_DEPENDENCY_H

#include <string>
#include <memory>
#include <functional>

namespace bedrock {

/**
 * @brief NamedDependency is a parent class for any object
 * that can be a dependency to another one, including providers,
 * provider handles, clients, SSG groups, ABT-IO instances,
 * Argobots pools, etc.
 *
 * It abstract their internal handle as a void* with a
 * release function to call when the dependency is no longer used.
 */
class NamedDependency {

    public:

    using ReleaseFn = std::function<void(void*)>;

    template<typename T>
    NamedDependency(std::string name, std::string type, T handle, ReleaseFn release)
    : m_name(std::move(name))
    , m_type(std::move(type))
    , m_handle(reinterpret_cast<void*>(handle))
    , m_release(std::move(release)) {}

    NamedDependency(NamedDependency&& other);

    virtual ~NamedDependency();

    const std::string& getName() const {
        return m_name;
    }

    const std::string& getType() const {
        return m_type;
    }

    template<typename H> H getHandle() const {
        return reinterpret_cast<H>(m_handle);
    }

    protected:

    std::string                m_name;
    std::string                m_type;
    void*                      m_handle;
    std::function<void(void*)> m_release;
};

} // namespace bedrock

#endif
