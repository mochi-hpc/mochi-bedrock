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

TEST_CASE("Tests Server initialization", "[config]") {

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
            auto input_config = jf[i]["input"].dump();
            auto expected_config = jf[i]["output"];
            try {
                bedrock::Server server("na+sm", input_config);
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
