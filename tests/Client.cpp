#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_all.hpp>
#include <bedrock/Server.hpp>
#include <bedrock/Client.hpp>
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

TEST_CASE("Tests various object creation and removal via a ServiceHandle", "[servier-handle]") {

    bedrock::Server server("na+sm");
    {
        auto engine = server.getMargoManager().getThalliumEngine();
        bedrock::Client client(engine);
        auto serviceHandle = client.makeServiceHandle(engine.self(), 0);
        SECTION("Add and remove pool remotely") {
            // add a pool called "my_pool1", synchronously
            serviceHandle.addPool("{\"name\":\"my_pool1\",\"kind\":\"fifo_wait\",\"access\":\"mpmc\"}");
            auto output_config = json::parse(server.getCurrentConfig());
            auto pools = output_config["margo"]["argobots"]["pools"];
            REQUIRE(std::find_if(pools.begin(), pools.end(),
                    [](auto& p) { return p["name"] == "my_pool1"; })
                    != pools.end());
            // remove "my_pool1", asynchronously
            serviceHandle.removePool("my_pool1");
            output_config = json::parse(server.getCurrentConfig());
            pools = output_config["margo"]["argobots"]["pools"];
            REQUIRE(std::find_if(pools.begin(), pools.end(),
                    [](auto& p) { return p["name"] == "my_pool1"; })
                    == pools.end());
            // add a pool called "my_pool2", asynchronously
            bedrock::AsyncRequest req;
            serviceHandle.addPool("{\"name\":\"my_pool2\",\"kind\":\"fifo_wait\",\"access\":\"mpmc\"}", &req);
            req.wait();
            output_config = json::parse(server.getCurrentConfig());
            pools = output_config["margo"]["argobots"]["pools"];
            REQUIRE(std::find_if(pools.begin(), pools.end(),
                    [](auto& p) { return p["name"] == "my_pool2"; })
                    != pools.end());
            // remove "my_pool2", asynchronously
            serviceHandle.removePool("my_pool2", &req);
            req.wait();
            output_config = json::parse(server.getCurrentConfig());
            pools = output_config["margo"]["argobots"]["pools"];
            REQUIRE(std::find_if(pools.begin(), pools.end(),
                    [](auto& p) { return p["name"] == "my_pool2"; })
                    == pools.end());
            // try to add a pool with an invalid configuration, synchronously
            REQUIRE_THROWS_AS(serviceHandle.addPool("1234"), bedrock::Exception);
            // try to add a pool with an invalid configuration, asynchronously
            serviceHandle.addPool("1234", &req);
            REQUIRE_THROWS_AS(req.wait(), bedrock::Exception);
            // try to remove a pool that does not exist, synchronously
            REQUIRE_THROWS_AS(serviceHandle.removePool("something"), bedrock::Exception);
            // try to remove a pool that does not exist, asynchronously
            serviceHandle.removePool("something", &req);
            REQUIRE_THROWS_AS(req.wait(), bedrock::Exception);
        }

        SECTION("Add and remove xstream remotely") {
            // add "my_es1" synchronously
            serviceHandle.addXstream(
                "{\"name\":\"my_es1\",\"scheduler\":{\"pools\":[0],\"type\":\"basic_wait\"}}");
            auto output_config = json::parse(server.getCurrentConfig());
            auto xstreams = output_config["margo"]["argobots"]["xstreams"];
            REQUIRE(std::find_if(xstreams.begin(), xstreams.end(),
                    [](auto& x) { return x["name"] == "my_es1"; })
                    != xstreams.end());
            // remove "my_es1" synchronously
            serviceHandle.removeXstream("my_es1");
            output_config = json::parse(server.getCurrentConfig());
            xstreams = output_config["margo"]["argobots"]["xstreams"];
            REQUIRE(std::find_if(xstreams.begin(), xstreams.end(),
                    [](auto& x) { return x["name"] == "my_es1"; })
                    == xstreams.end());
            // add "my_es2" asynchronously
            bedrock::AsyncRequest req;
            serviceHandle.addXstream(
                "{\"name\":\"my_es2\",\"scheduler\":{\"pools\":[0],\"type\":\"basic_wait\"}}",
                &req);
            req.wait();
            output_config = json::parse(server.getCurrentConfig());
            xstreams = output_config["margo"]["argobots"]["xstreams"];
            REQUIRE(std::find_if(xstreams.begin(), xstreams.end(),
                    [](auto& x) { return x["name"] == "my_es2"; })
                    != xstreams.end());
            // remove "my_es2" asynchronously
            serviceHandle.removeXstream("my_es2", &req);
            req.wait();
            output_config = json::parse(server.getCurrentConfig());
            xstreams = output_config["margo"]["argobots"]["xstreams"];
            REQUIRE(std::find_if(xstreams.begin(), xstreams.end(),
                    [](auto& x) { return x["name"] == "my_es2"; })
                    == xstreams.end());
            // try to add an xstream with an invalid configuration, synchronously
            REQUIRE_THROWS_AS(serviceHandle.addXstream("1234"), bedrock::Exception);
            // try to add an xstream with an invalid configuration, asynchronously
            serviceHandle.addXstream("1234", &req);
            REQUIRE_THROWS_AS(req.wait(), bedrock::Exception);
            // try to remove an xstream that does not exist, synchronously
            REQUIRE_THROWS_AS(serviceHandle.removeXstream("something"), bedrock::Exception);
            // try to remove an xstream that does not exist, asynchronously
            serviceHandle.removeXstream("something", &req);
            REQUIRE_THROWS_AS(req.wait(), bedrock::Exception);
        }

        SECTION("Load a library") {
            auto server_config = server.getCurrentConfig();
            REQUIRE(server_config.find("./libModuleA.so") == std::string::npos);
            REQUIRE(server_config.find("./libModuleB.so") == std::string::npos);
            // load libModuleA.so synchronously
            serviceHandle.loadModule("./libModuleA.so");
            server_config = server.getCurrentConfig();
            REQUIRE(server_config.find("./libModuleA.so") != std::string::npos);
            // load libModuleA.so asynchronously
            bedrock::AsyncRequest req;
            serviceHandle.loadModule("./libModuleB.so", &req);
            req.wait();
            server_config = server.getCurrentConfig();
            REQUIRE(server_config.find("./libModuleB.so") != std::string::npos);
            // load libModuleX.so, which does not exist
            REQUIRE_THROWS_AS(
                serviceHandle.loadModule("./libModuleX.so"),
                bedrock::Exception);
        }

        SECTION("Add and remove providers") {
            // load module_a
            serviceHandle.loadModule("./libModuleA.so");
            // create a provider of type module_a
            REQUIRE_NOTHROW(serviceHandle.addProvider(R"(
                {"name":"my_provider_a1", "type":"module_a", "provider_id":123})"));
            auto output_config = json::parse(server.getCurrentConfig());
            auto providers = output_config["providers"];
            REQUIRE(std::find_if(providers.begin(), providers.end(),
                    [](auto& p) { return p["name"] == "my_provider_a1"; })
                    != providers.end());
            // TODO delete the provider
            // create a provider with the non-blocking API
            bedrock::AsyncRequest req;
            REQUIRE_NOTHROW(serviceHandle.addProvider(R"(
                {"name":"my_provider_a2", "type":"module_a", "provider_id":34,
                 "pool":"__primary__", "dependencies":{}, "tags":[]})",
                nullptr, &req));
            req.wait();
            output_config = json::parse(server.getCurrentConfig());
            providers = output_config["providers"];
            REQUIRE(std::find_if(providers.begin(), providers.end(),
                    [](auto& p) { return p["name"] == "my_provider_a2"; })
                    != providers.end());
            // TODO delete the provider
            // create a provider of type module_a but let Bedrock choose the provider ID
            REQUIRE_NOTHROW(serviceHandle.addProvider(
                    R"({"name":"my_provider_a3", "type":"module_a", "provider_id":65535})"));
            output_config = json::parse(server.getCurrentConfig());
            providers = output_config["providers"];
            auto it = std::find_if(providers.begin(), providers.end(),
                      [](auto& p) { return p["name"] == "my_provider_a3"; });
            REQUIRE(it != providers.end());
            REQUIRE((*it)["provider_id"] == 1);
            // create a provider of type module_a but let Bedrock choose the provider ID again
            REQUIRE_NOTHROW(serviceHandle.addProvider(
                    R"({"name":"my_provider_a4", "type":"module_a", "provider_id":65535})"));
            output_config = json::parse(server.getCurrentConfig());
            providers = output_config["providers"];
            it = std::find_if(providers.begin(), providers.end(),
                      [](auto& p) { return p["name"] == "my_provider_a4"; });
            REQUIRE(it != providers.end());
            REQUIRE((*it)["provider_id"] == 2);


            // create a provider of an invalid type
            REQUIRE_THROWS_AS(
                serviceHandle.addProvider(
                    R"({"name":"my_provider_x", "type":"module_x", "provider_id":234})"),
                bedrock::Exception);
            // create a provider of an invalid type asynchronously
            serviceHandle.addProvider(
                R"({"name":"my_provider_x", "type":"module_x", "provider_id":234})",
                nullptr, &req);
            REQUIRE_THROWS_AS(req.wait(), bedrock::Exception);
        }
    }
    server.finalize();
}
