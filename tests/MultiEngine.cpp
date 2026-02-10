#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_all.hpp>
#include <bedrock/Server.hpp>
#include <bedrock/MargoManager.hpp>
#include <bedrock/Client.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

TEST_CASE("Tests multi-engine MargoManager", "[multi-engine]") {

    SECTION("Multi-engine initialization with array margo config") {
        json config;
        config["margo"] = json::array();
        config["margo"].push_back(json::object());
        config["margo"].push_back(json::object());
        bedrock::Server server("na+sm", config.dump());
        auto mgr = server.getMargoManager();
        REQUIRE(mgr.getNumEngines() == 2);
        // Both engines should have valid addresses
        std::string addr0 = static_cast<std::string>(mgr.getThalliumEngine(0).self());
        std::string addr1 = static_cast<std::string>(mgr.getThalliumEngine(1).self());
        REQUIRE(addr0.find("na+sm://") == 0);
        REQUIRE(addr1.find("na+sm://") == 0);
        REQUIRE(addr0 != addr1);
        server.finalize();
    }

    SECTION("Single-engine backward compatibility") {
        json config;
        config["margo"] = json::object();
        bedrock::Server server("na+sm", config.dump());
        auto mgr = server.getMargoManager();
        REQUIRE(mgr.getNumEngines() == 1);
        // Config output should be a JSON object (not array)
        auto output = json::parse(server.getCurrentConfig());
        REQUIRE(output["margo"].is_object());
        server.finalize();
    }

    SECTION("Multi-engine config output is an array") {
        json config;
        config["margo"] = json::array();
        config["margo"].push_back(json::object());
        config["margo"].push_back(json::object());
        bedrock::Server server("na+sm", config.dump());
        auto output = json::parse(server.getCurrentConfig());
        REQUIRE(output["margo"].is_array());
        REQUIRE(output["margo"].size() == 2);
        server.finalize();
    }

    SECTION("Engine index bounds checking") {
        json config;
        config["margo"] = json::array();
        config["margo"].push_back(json::object());
        config["margo"].push_back(json::object());
        bedrock::Server server("na+sm", config.dump());
        auto mgr = server.getMargoManager();

        REQUIRE_THROWS_AS(mgr.getThalliumEngine(5), bedrock::Exception);
        REQUIRE_THROWS_AS(mgr.getMargoInstance(5), bedrock::Exception);

        server.finalize();
    }

    SECTION("Empty margo array throws") {
        json config;
        config["margo"] = json::array();
        REQUIRE_THROWS_AS(
            bedrock::Server("na+sm", config.dump()),
            bedrock::Exception);
    }

    SECTION("Client can query config via engine 0") {
        json config;
        config["margo"] = json::array();
        config["margo"].push_back(json::object());
        config["margo"].push_back(json::object());
        bedrock::Server server("na+sm", config.dump());
        auto mgr = server.getMargoManager();
        auto engine = mgr.getThalliumEngine(0);
        bedrock::Client client(engine);
        auto service_handle = client.makeServiceHandle(engine.self(), 0);
        std::string output_config_str;
        service_handle.getConfig(&output_config_str);
        auto output_config = json::parse(output_config_str);
        REQUIRE(output_config["margo"].is_array());
        REQUIRE(output_config["margo"].size() == 2);
        server.finalize();
    }

    SECTION("Default initialization still works") {
        bedrock::Server server("na+sm");
        auto mgr = server.getMargoManager();
        REQUIRE(mgr.getNumEngines() == 1);
        server.finalize();
    }
}
