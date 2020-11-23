/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_DEPENDENCY_WRAPPER_HPP
#define __BEDROCK_DEPENDENCY_WRAPPER_HPP

namespace bedrock {

/**
 * @brief DependencyWrapper wraps a void* pointer
 * representing a dependency and calls the provided
 * function on it upon destruction.
 */
struct DependencyWrapper {

    void* handle         = nullptr;
    void (*clear)(void*) = nullptr;

    DependencyWrapper() = default;

    template <typename T>
    DependencyWrapper(T* _handle, void (*_clear)(void*) = nullptr)
    : handle(static_cast<void*>(_handle)), clear(_clear) {}

    ~DependencyWrapper() {
        if (clear) clear(handle);
    }

    DependencyWrapper(const DependencyWrapper&) = delete;

    DependencyWrapper(DependencyWrapper&& other)
    : handle(other.handle), clear(other.clear) {
        other.handle = nullptr;
        other.clear  = nullptr;
    }

    DependencyWrapper& operator=(const DependencyWrapper&) = delete;

    DependencyWrapper& operator=(DependencyWrapper&& other) {
        if (clear && this != &other) clear(handle);
        handle       = other.handle;
        clear        = other.clear;
        other.handle = nullptr;
        other.clear  = nullptr;
        return *this;
    }
};

} // namespace bedrock

#endif
