/*
 * (C) 2024 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef BEDROCK_JSON_UTIL_H
#define BEDROCK_JSON_UTIL_H

#include <nlohmann/json.hpp>
#include <valijson/adapters/nlohmann_json_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>
#include <string>
#include "Exception.hpp"

namespace bedrock {

struct JsonValidator {

    using json = nlohmann::json;

    json             schemaDocument;
    valijson::Schema schemaValidator;

    JsonValidator(const char* schema_str) {
        schemaDocument = json::parse(schema_str);
        valijson::SchemaParser schemaParser;
        valijson::adapters::NlohmannJsonAdapter schemaAdapter(schemaDocument);
        schemaParser.populateSchema(schemaAdapter, schemaValidator);
    }

    using ErrorList = std::vector<std::string>;

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
        valijson::Validator validator;
        valijson::ValidationResults validationResults;
        valijson::adapters::NlohmannJsonAdapter jsonAdapter(config);
        validator.validate(schemaValidator, jsonAdapter, &validationResults);
        if(validationResults.numErrors() == 0) return;
        std::stringstream ss;
        auto it=validationResults.begin();
        for(size_t i=0;
            i < validationResults.numErrors(); ++i, ++it) {
            if (context || it->context.size() != 1) {
                ss << "In ";
                if(context) ss << context;
                if (it->context.size() != 1) {
                    ss << ' ';
                    for(size_t j = 1; j < it->context.size(); ++j) {
                        ss << it->context[j];
                    }
                }
                ss << ": ";
            }
            ss << it->description;
            if(i < validationResults.numErrors() - 1)
                ss << '\n';
        }
        throw Exception{ss.str()};
    }

};

}

#endif
