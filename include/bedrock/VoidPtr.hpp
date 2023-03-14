/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_VOID_PTR_HPP
#define __BEDROCK_VOID_PTR_HPP

#include <functional>

namespace bedrock {

/**
 * @brief VoidPtr wraps a void* pointer
 * representing an abstract handle obtained from a C call,
 * and calls the provided function on it upon destruction.
 * It is somewhat equivalent to an std::unique_ptr<void>,
 * except that such a construct is not allowed.
 */
struct VoidPtr {

    void*                      handle = nullptr;
    std::function<void(void*)> clear;

    VoidPtr() = default;

    template <typename T, typename F>
    VoidPtr(T* _handle, F&& _clear)
    : handle(static_cast<void*>(_handle)), clear(std::forward<F>(_clear)) {}

    template <typename T>
    VoidPtr(T* _handle) : handle(static_cast<void*>(_handle)) {}

    ~VoidPtr() {
        if (clear) clear(handle);
    }

    VoidPtr(const VoidPtr&) = delete;

    VoidPtr(VoidPtr&& other) : handle(other.handle), clear(other.clear) {
        other.handle = nullptr;
        other.clear  = std::function<void(void*)>();
    }

    VoidPtr& operator=(const VoidPtr&) = delete;

    VoidPtr& operator=(VoidPtr&& other) {
        if (clear && this != &other) clear(handle);
        handle       = other.handle;
        clear        = other.clear;
        other.handle = nullptr;
        other.clear  = std::function<void(void*)>();
        return *this;
    }
};

} // namespace bedrock

#endif
