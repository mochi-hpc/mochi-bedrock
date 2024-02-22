/*
 * (C) 2024 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef BEDROCK_JSON_UTIL_H
#define BEDROCK_JSON_UTIL_H

#include <nlohmann/json.hpp>
#include <nlohmann/json-schema.hpp>
#include <string>
#include "Exception.hpp"

namespace bedrock {

using nlohmann::json;
using nlohmann::json_schema::json_validator;

struct JsonValidator {

    json_validator m_validator;

    JsonValidator(const json& schema) {
        m_validator.set_root_schema(schema);
    }

    JsonValidator(json&& schema) {
        m_validator.set_root_schema(std::move(schema));
    }

    JsonValidator(const char* schema) {
        m_validator.set_root_schema(json::parse(schema));
    }

    json parseAndValidate(const char* config_str, const char* context = nullptr) const {
        json config;
        try {
            config = json::parse(config_str);
        } catch(const std::exception& ex) {
            throw Exception(ex.what());
        }
        validate(config, context);
        return config;
    }

    void validate(const json& config, const char* context = nullptr) const {
        try {
            m_validator.validate(config);
        } catch(const std::exception& ex) {
            throw Exception("Error validating JSON ({}): {}",
                  context ? context : "unknown context",
                  ex.what());
        }
    }

};

}

#endif
