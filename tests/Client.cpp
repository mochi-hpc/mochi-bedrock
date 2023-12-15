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

        SECTION("Add and remove ABT-IO instances remotely") {
            // add ABT-IO instance synchronously
            serviceHandle.addABTioInstance("my_abt_io1", "__primary__");
            auto output_config = json::parse(server.getCurrentConfig());
            auto abt_io = output_config["abt_io"];
            REQUIRE(std::find_if(abt_io.begin(), abt_io.end(),
                    [](auto& x) { return x["name"] == "my_abt_io1"; })
                    != abt_io.end());
            // add ABT-IO instance asynchronously
            bedrock::AsyncRequest req;
            serviceHandle.addABTioInstance("my_abt_io2", "__primary__", "{}", &req);
            req.wait();
            output_config = json::parse(server.getCurrentConfig());
            abt_io = output_config["abt_io"];
            REQUIRE(std::find_if(abt_io.begin(), abt_io.end(),
                    [](auto& x) { return x["name"] == "my_abt_io2"; })
                    != abt_io.end());
            // add ABT-IO instance with invalid configuration
            REQUIRE_THROWS_AS(
                serviceHandle.addABTioInstance("my_abt_io3", "__primary__", "1234"),
                bedrock::Exception);
            // TODO: add removal when we have the functionality for it
        }

        SECTION("Load a library") {
            // load libModuleA.so synchronously
            serviceHandle.loadModule("module_a", "libModuleA.so");
            REQUIRE(bedrock::ModuleContext::getServiceFactory("module_a") != nullptr);
            // load libModuleA.so asynchronously
            bedrock::AsyncRequest req;
            serviceHandle.loadModule("module_b", "libModuleB.so", &req);
            req.wait();
            REQUIRE(bedrock::ModuleContext::getServiceFactory("module_b") != nullptr);
            // load libModuleC.so, which does not exist
            REQUIRE_THROWS_AS(
                serviceHandle.loadModule("module_c", "libModuleC.so"),
                bedrock::Exception);
        }

        SECTION("Add and remove providers") {
            // load module_a
            serviceHandle.loadModule("module_a", "libModuleA.so");
            REQUIRE(bedrock::ModuleContext::getServiceFactory("module_a") != nullptr);
            // create a provider of type module_a
            REQUIRE_NOTHROW(serviceHandle.startProvider("my_provider_a1", "module_a", 123));
            auto output_config = json::parse(server.getCurrentConfig());
            auto providers = output_config["providers"];
            REQUIRE(std::find_if(providers.begin(), providers.end(),
                    [](auto& p) { return p["name"] == "my_provider_a1"; })
                    != providers.end());
            // TODO delete the provider
            // create a provider with the non-blocking API
            bedrock::AsyncRequest req;
            REQUIRE_NOTHROW(serviceHandle.startProvider(
                    "my_provider_a2", "module_a", 34, nullptr,
                    "__primary__", "{}",
                    bedrock::DependencyMap(), {}, &req));
            req.wait();
            output_config = json::parse(server.getCurrentConfig());
            providers = output_config["providers"];
            REQUIRE(std::find_if(providers.begin(), providers.end(),
                    [](auto& p) { return p["name"] == "my_provider_a2"; })
                    != providers.end());
            // TODO delete the provider
            // create a provider of type module_a but let Bedrock choose the provider ID
            REQUIRE_NOTHROW(serviceHandle.startProvider(
                    "my_provider_a3", "module_a",
                    bedrock::ServiceHandle::NewProviderID));
            output_config = json::parse(server.getCurrentConfig());
            providers = output_config["providers"];
            auto it = std::find_if(providers.begin(), providers.end(),
                      [](auto& p) { return p["name"] == "my_provider_a3"; });
            REQUIRE(it != providers.end());
            REQUIRE((*it)["provider_id"] == 0);
            // create a provider of type module_a but let Bedrock choose the provider ID again
            REQUIRE_NOTHROW(serviceHandle.startProvider(
                    "my_provider_a4", "module_a",
                    bedrock::ServiceHandle::NewProviderID));
            output_config = json::parse(server.getCurrentConfig());
            providers = output_config["providers"];
            it = std::find_if(providers.begin(), providers.end(),
                      [](auto& p) { return p["name"] == "my_provider_a4"; });
            REQUIRE(it != providers.end());
            REQUIRE((*it)["provider_id"] == 1);


            // create a provider of an invalid type
            REQUIRE_THROWS_AS(
                serviceHandle.startProvider("my_provider_c", "module_c", 234),
                bedrock::Exception);
            // create a provider of an invalid type asynchronously
            serviceHandle.startProvider(
                "my_provider_c", "module_c", 234, nullptr,
                "", "{}", bedrock::DependencyMap(), {}, &req);
            REQUIRE_THROWS_AS(req.wait(), bedrock::Exception);
        }
    }
    server.finalize();
}
