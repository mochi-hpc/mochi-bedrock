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
        auto& engine = server.getMargoManager().getThalliumEngine();
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
    }
    server.finalize();
}
