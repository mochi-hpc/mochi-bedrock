#include <catch2/catch_test_macros.hpp>
#include <bedrock/Server.hpp>
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

TEST_CASE("Tests Server initialization", "[config]") {

    SECTION("Default initialization") {
        bedrock::Server server("na+sm");
        server.finalize();
    }

    {
    std::ifstream ifs("ValidConfigs.json");
    json jf = json::parse(ifs);

    for(unsigned i = 0; i < jf.size(); i++) {
        DYNAMIC_SECTION("Initialization with config " << i << " from ValidConfigs.json") {
            bedrock::Server server("na+sm", jf[i].get_ref<const std::string&>());
            server.finalize();
        }
    }
    }

    {
    std::ifstream ifs("InvalidConfigs.json");
    json jf = json::parse(ifs);

    for(unsigned i = 0; i < jf.size(); i++) {
        DYNAMIC_SECTION("Initialization with config " << i << " from InvalidConfigs.json") {
            REQUIRE_THROWS_AS(bedrock::Server("na+sm", jf[i].get_ref<const std::string&>()),
                              bedrock::Exception);
        }
    }
    }
}
