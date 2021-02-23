/*
 * (C) 2021 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_DEPENDENCY_MAP_HPP
#define __BEDROCK_DEPENDENCY_MAP_HPP

#include <string>
#include <vector>
#include <unordered_map>

namespace bedrock {

typedef std::unordered_map<std::string, std::vector<std::string>> DependencyMap;

} // namespace bedrock

#endif
