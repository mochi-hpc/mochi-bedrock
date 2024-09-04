#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_all.hpp>
#include <bedrock/Server.hpp>
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

static void cleanupOutputConfig(json& config) {
    config["margo"].erase("mercury");
    config["margo"].erase("version");
}

static std::string jsonToJx9(const json& config) {
    std::stringstream ss;
    ss << "print \"Hello from JX9, \", $user.name, JX9_EOL;\n";
    ss << "print $a, JX9_EOL;\n";
    ss << "$config = " << config.dump() << ";\n";
    ss << "return $config;\n";
    return ss.str();
}

TEST_CASE("Tests Server initialization with JX9", "[init-jx9]") {

    //spdlog::set_level(spdlog::level::from_str("trace"));

    std::ifstream ifs("ValidConfigs.json");
    json jf = json::parse(ifs);

    for(unsigned i = 0; i < jf.size(); i++) {
        DYNAMIC_SECTION(
            "Initialization with config " << i
            << " from ValidConfigs.json ("
            << jf[i]["test"].get<std::string>() << ")")
        {
            auto input_config = jf[i]["input"];
            auto input_jx9 = jsonToJx9(input_config);
            auto expected_config = jf[i]["output"];
            try {
                bedrock::Jx9ParamMap params;
                params["user"] = "{\"name\":\"Matthieu\",\"year\":2023,\"bool\":true,\"float\":1.23,\"array\":[],\"netagive\":-1}";
                bedrock::Server server("na+sm", input_jx9, bedrock::ConfigType::JX9, params);
                auto output_config = json::parse(server.getCurrentConfig());
                cleanupOutputConfig(output_config);
                REQUIRE(output_config == expected_config);
                server.finalize();
            } catch(bedrock::Exception& ex) {
                INFO("Details: " << ex.details());
                throw;
            }
        }
    }

    SECTION("Invalid jx9 script") {
        std::string input_jx9 = "+&*";
        REQUIRE_THROWS_MATCHES(
              bedrock::Server("na+sm", input_jx9, bedrock::ConfigType::JX9),
              bedrock::Exception,
              Catch::Matchers::Message("Jx9 script failed to compile: 1 Error: '&': Missing operand"));

    }
}
