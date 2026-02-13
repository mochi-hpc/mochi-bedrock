#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_all.hpp>
#include <bedrock/Server.hpp>
#include <bedrock/Exception.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

TEST_CASE("Tests protocol inference from mercury.address", "[protocol-inference]") {

    SECTION("Infer protocol from mercury.address in config") {
        json config;
        config["margo"]["mercury"]["address"] = "na+sm://0";
        bedrock::Server server("", config.dump());
        auto engine = server.getMargoManager().getThalliumEngine();
        std::string addr = static_cast<std::string>(engine.self());
        REQUIRE(addr.find("na+sm://") == 0);
        server.finalize();
    }

    SECTION("Empty address and empty config throws") {
        REQUIRE_THROWS_AS(
            bedrock::Server("", ""),
            bedrock::Exception);
    }

    SECTION("Empty address and config without mercury throws") {
        json config;
        config["margo"] = json::object();
        REQUIRE_THROWS_AS(
            bedrock::Server("", config.dump()),
            bedrock::Exception);
    }

    SECTION("Empty address and mercury without address field throws") {
        json config;
        config["margo"]["mercury"] = json::object();
        REQUIRE_THROWS_AS(
            bedrock::Server("", config.dump()),
            bedrock::Exception);
    }

    SECTION("Empty address and mercury.address without colon throws") {
        json config;
        config["margo"]["mercury"]["address"] = "nocolon";
        REQUIRE_THROWS_AS(
            bedrock::Server("", config.dump()),
            bedrock::Exception);
    }

    SECTION("Empty address and mercury.address is not a string throws") {
        json config;
        config["margo"]["mercury"]["address"] = 12345;
        REQUIRE_THROWS_AS(
            bedrock::Server("", config.dump()),
            bedrock::Exception);
    }
}
