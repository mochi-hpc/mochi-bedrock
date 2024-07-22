/*
 * (C) 2024 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef BEDROCK_JSON_UTIL_H
#define BEDROCK_JSON_UTIL_H

#include <bedrock/DetailedException.hpp>
#include <bedrock/Jx9Manager.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json-schema.hpp>
#include <string>

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

static inline void expandJson(const json& input, json& output) {
    for (auto it = input.begin(); it != input.end(); ++it) {
        std::istringstream key_stream(it.key());
        std::string segment;
        json* current = &output;

        while (std::getline(key_stream, segment, '.')) {
            if (!key_stream.eof()) {
                if (current->find(segment) == current->end() || !(*current)[segment].is_object()) {
                    (*current)[segment] = json::object();
                }
                current = &(*current)[segment];
            } else {
                if (it.value().is_object()) {
                    expandJson(it.value(), (*current)[segment]);
                } else {
                    (*current)[segment] = it.value();
                }
            }
        }
    }
}

static inline json expandSimplifiedJSON(const json& input) {
    json output = json::object();
    expandJson(input, output);
    return output;
}

static inline json filterIfConditionsInJSON(const json& input, Jx9Manager jx9) {
    json output; // null by default
    if(input.is_array()) {
        output = json::array();
        for(auto& item : input) {
            auto filteredItem = filterIfConditionsInJSON(item, jx9);
            if(filteredItem.is_null()) continue;
            output.push_back(filterIfConditionsInJSON(item, jx9));
        }
    } else if(input.is_object()) {
        bool condition = true;
        if(input.contains("__if__")) {
            auto _if = input["__if__"];
            if(_if.is_boolean()) condition = _if.get<bool>();
            else if(_if.is_string())
                condition = jx9.evaluateCondition(
                    _if.get_ref<const std::string&>(),
                    std::unordered_map<std::string, std::string>());
            else
                throw Exception("__if__ condition should be a string or a boolean");
        }
        if(condition) {
            output = json::object();
            for(auto& p : input.items()) {
                output[p.key()] = filterIfConditionsInJSON(p.value(), jx9);
            }
        }
    } else {
        output = input;
    }
    return output;
}

}

#endif
