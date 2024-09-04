#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_all.hpp>
#include <bedrock/Server.hpp>
#include <bedrock/MargoManager.hpp>
#include <bedrock/Client.hpp>
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

static void cleanupOutputConfig(json& config) {
    config["margo"].erase("mercury");
    config["margo"].erase("version");
}

TEST_CASE("Tests Server initialization", "[init-json]") {

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
                SECTION("Get configuration directly from Server object") {
                    bedrock::Server server("na+sm", input_config);
                    auto engine = server.getMargoManager().getThalliumEngine();
                    auto output_config = json::parse(server.getCurrentConfig());
                    cleanupOutputConfig(output_config);
                    REQUIRE(output_config == expected_config);
                    server.finalize();
                }
                SECTION("Get configuration synchronously using a Client") {
                    bedrock::Server server("na+sm", input_config);
                    auto engine = server.getMargoManager().getThalliumEngine();
                    bedrock::Client client(engine);
                    auto service_handle = client.makeServiceHandle(engine.self(), 0);
                    std::string output_config_str;
                    service_handle.getConfig(&output_config_str);
                    auto output_config = json::parse(output_config_str);
                    cleanupOutputConfig(output_config);
                    REQUIRE(output_config == expected_config);
                    server.finalize();
                }
                SECTION("Get configuration asynchronously using a Client") {
                    bedrock::Server server("na+sm", input_config);
                    auto engine = server.getMargoManager().getThalliumEngine();
                    bedrock::Client client(engine);
                    auto service_handle = client.makeServiceHandle(engine.self(), 0);
                    std::string output_config_str;
                    bedrock::AsyncRequest req;
                    service_handle.getConfig(&output_config_str, &req);
                    req.wait();
                    auto output_config = json::parse(output_config_str);
                    cleanupOutputConfig(output_config);
                    REQUIRE(output_config == expected_config);
                    server.finalize();
                }
            } catch(bedrock::Exception& ex) {
                INFO("Details: " << ex.details());
                throw;
            }
        }
    }

    SECTION("Initialize from TOML") {

        const std::string input_config = R"(
[margo]
use_progress_thread = false

[[margo.argobots.pools]]
name   = "my_pool_1"
access = "mpmc"
kind   = "fifo_wait"

[[margo.argobots.pools]]
name   = "my_pool_2"
access = "mpmc"
kind   = "fifo"
)";
        bedrock::Server server("na+sm", input_config, bedrock::ConfigType::TOML);
        {
            auto margo_manager = server.getMargoManager();
            REQUIRE(margo_manager.getNumPools() == 3);
            REQUIRE(margo_manager.getPool((uint32_t)0)->getName() == "my_pool_1");
            REQUIRE(margo_manager.getPool(1)->getName() == "my_pool_2");
        }
        server.finalize();
    }
}
