/*
 * (C) 2023 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/NamedDependency.hpp"
#include <spdlog/spdlog.h>

namespace bedrock {

NamedDependency::NamedDependency(NamedDependency&& other)
: m_name(std::move(other.m_name))
, m_handle(other.m_handle)
, m_release(std::move(other.m_release)) {
    other.m_handle = nullptr;
    other.m_release = std::function<void(void*)>();
}

NamedDependency::~NamedDependency() {
    spdlog::trace("Releasing resource \"{}\" of type \"{}\"", m_name, m_type);
    if(m_release) m_release(m_handle);
}

} // namespace bedrock
