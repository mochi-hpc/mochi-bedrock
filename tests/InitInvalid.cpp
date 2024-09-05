#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_all.hpp>
#include <bedrock/Server.hpp>
#include <bedrock/Exception.hpp>
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

TEST_CASE("Tests Server initialization", "[init-invalid]") {

    //spdlog::set_level(spdlog::level::from_str("trace"));

    std::ifstream ifs("InvalidConfigs.json");
    json jf = json::parse(ifs);

    for(unsigned i = 0; i < jf.size(); i++) {
        DYNAMIC_SECTION(
            "Initialization with config " << i
            << " from InvalidConfigs.json ("
            << jf[i]["test"].get<std::string>() << ")")
        {
            auto input_config = jf[i]["input"].dump();
            CAPTURE(input_config);
            REQUIRE_THROWS_AS(
                bedrock::Server("na+sm", input_config),
                bedrock::Exception);
        }
    }
}
