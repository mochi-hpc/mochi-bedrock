/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_PROVIDER_HANDLE_H
#define __BEDROCK_PROVIDER_HANDLE_H

#include <thallium.hpp>

namespace bedrock {

/**
 * @brief A pointer to a ProviderHandle is returned by
 * DependencyFinder::makeProviderHandle, wrapped as a
 * std::shared_ptr<NamedDependency>.
 */
using ProviderHandle = thallium::provider_handle;

} // namespace bedrock

#endif
