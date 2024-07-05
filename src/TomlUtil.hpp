/*
 * (C) 2024 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef BEDROCK_TOML_UTIL_H
#define BEDROCK_TOML_UTIL_H

#include <bedrock/DetailedException.hpp>
#include <nlohmann/json.hpp>
#include <toml.hpp>
#include <string>

namespace bedrock {

static inline nlohmann::json Toml2Json(const toml::value& toml_val);

nlohmann::json convertTomlValue(const toml::value& toml_val) {
    if (toml_val.is_table()) {
        nlohmann::json j;
        for (const auto& p : toml_val.as_table()) {
            j[p.first] = convertTomlValue(p.second);
        }
        return j;
    }
    else if (toml_val.is_array()) {
        nlohmann::json j = nlohmann::json::array();
        for (const auto& item : toml_val.as_array()) {
            j.push_back(convertTomlValue(item));
        }
        return j;
    }
    else if (toml_val.is_string()) {
        return static_cast<const std::string&>(toml_val.as_string());
    }
    else if (toml_val.is_integer()) {
        return toml_val.as_integer();
    }
    else if (toml_val.is_floating()) {
        return toml_val.as_floating();
    }
    else if (toml_val.is_boolean()) {
        return toml_val.as_boolean();
    }
    else {
        throw Exception{"Invalid TOML value encountered"};
    }
}

nlohmann::json Toml2Json(const toml::value& toml_val) {
    return convertTomlValue(toml_val);
}

}

#endif
