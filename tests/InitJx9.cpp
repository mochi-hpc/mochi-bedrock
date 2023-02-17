#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_all.hpp>
#include <bedrock/Server.hpp>
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

static void cleanupOutputConfig(json& config) {
    config["margo"].erase("mercury");
    config["margo"].erase("version");
    for(auto& instance : config["abt_io"]) {
        instance["config"].erase("version");
    }
    for(auto& instance : config["mona"]) {
        if(instance.contains("address"))
            instance["address"] = "<replaced>";
    }
}

static std::string jsonToJx9(const json& config) {
    std::stringstream ss;
    ss << "$config = " << config.dump() << ";\nreturn $config;" << std::endl;
    return ss.str();
}

TEST_CASE("Tests Server initialization with JX9", "[init-jx9]") {

    //spdlog::set_level(spdlog::level::from_str("trace"));

    SECTION("Default initialization") {
        bedrock::Server server("na+sm");
        server.finalize();
    }

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
                params["a"] = "123";
                bedrock::Server server("na+sm", input_jx9, bedrock::ConfigType::JX9, params);
                auto output_config = json::parse(server.getCurrentConfig());
                cleanupOutputConfig(output_config);
                REQUIRE(output_config == expected_config);
            } catch(bedrock::Exception& ex) {
                INFO("Details: " << ex.details());
                throw;
            }
        }
    }
}
