import unittest
import pymargo.logging
import mochi.bedrock.server as mbs
import mochi.bedrock.spec as spec


class TestProviderManager(unittest.TestCase):

    def setUp(self):
        config = {
            "margo": {
                "argobots": {
                    "pools": [
                        { "name": "my_pool", "kind": "fifo_wait", "access": "mpmc" }
                    ],
                    "xstreams": [
                        { "name": "my_xstream",
                          "scheduler": {
                            "type": "basic_wait",
                            "pools": ["my_pool"]
                          }
                        }
                    ]
                }
            },
            "libraries": {
                "module_a": "./libModuleA.so",
                "module_b": "./libModuleB.so",
                "module_c": "./libModuleC.so",
            },
            "providers": [
                {
                    "name": "my_provider_a",
                    "type": "module_a",
                    "provider_id": 1
                },
                {
                    "name": "my_provider_b",
                    "type": "module_b",
                    "provider_id": 2
                }
            ],
            "clients": [
                {
                    "name": "my_client_a",
                    "type": "module_a"
                },
                {
                    "name": "my_client_b",
                    "type": "module_b"
                }
            ],
            "abt_io": [
                {
                    "name": "my_abt_io",
                    "pool": "__primary__"
                }
            ],
            "mona": [
                {
                    "name": "my_mona",
                    "pool": "__primary__"
                }
            ]
        }
        self.server = mbs.Server(address="na+sm", config=config)
        self.server.margo.engine.logger.set_log_level(pymargo.logging.level.critical)

    def tearDown(self):
        self.server.finalize()
        del self.server

    def make_client_params(self, expected_dependencies: dict={}):
        params = {
            "name": "my_provider_C",
            "type": "module_c",
            "config": {
                "expected_client_dependencies": expected_dependencies
            }
        }
        return params

    def make_provider_params(self, expected_dependencies: dict={}):
        params = self.make_client_params({})
        params["provider_id"] = 3
        params["pool"] = "my_pool"
        params["config"]["expected_provider_dependencies"] = expected_dependencies
        return params


    def test_no_dependency(self):
        providers = self.server.providers
        self.assertEqual(len(providers), 2)
        clients = self.server.clients
        self.assertEqual(len(clients), 2)

        client_params = self.make_client_params()
        clients.create(**client_params)

        provider_params = self.make_provider_params()
        providers.create(**provider_params)

        self.assertEqual(len(providers), 3)
        self.assertEqual(len(clients), 3)

    def test_optional_dependency(self):
        providers = self.server.providers
        self.assertEqual(len(providers), 2)
        clients = self.server.clients
        self.assertEqual(len(clients), 2)

        client_params = self.make_client_params([
            {"name": "dep1",
             "type": "module_a",
             "kind": "provider_handle",
             "is_array": False,
             "is_required": False,
            }])
        clients.create(**client_params)

        provider_params = self.make_provider_params([
            {"name": "dep1",
             "type": "module_a",
             "kind": "provider_handle",
             "is_array": False,
             "is_required": False,
            }])
        providers.create(**provider_params)

        self.assertEqual(len(providers), 3)
        self.assertEqual(len(clients), 3)

    def test_required_dependency(self):
        providers = self.server.providers
        self.assertEqual(len(providers), 2)
        clients = self.server.clients
        self.assertEqual(len(clients), 2)

        # Try creating a client without the required dependency
        client_params = self.make_client_params([
            {"name": "dep1",
             "type": "module_a",
             "kind": "provider_handle",
             "is_array": False,
             "is_required": True,
            }])
        with self.assertRaises(mbs.BedrockException):
            clients.create(**client_params)

        # Try creating a client with the required dependency
        client_params["dependencies"] = {
            "dep1": "my_provider_a@local"
        }
        clients.create(**client_params)
        self.assertEqual(len(clients), 3)

        # Try creating a provider without the required dependency
        provider_params = self.make_provider_params([
            {"name": "dep1",
             "type": "module_a",
             "kind": "provider_handle",
             "is_array": False,
             "is_required": True,
            }])
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with the required dependency
        provider_params["dependencies"] = {
            "dep1": "my_provider_a@local"
        }
        providers.create(**provider_params)
        self.assertEqual(len(providers), 3)

    def test_dependency_on_pool(self):
        providers = self.server.providers
        self.assertEqual(len(providers), 2)
        clients = self.server.clients
        self.assertEqual(len(clients), 2)

        # Try creating a client without the required dependency
        client_params = self.make_client_params([
            {"name": "dep1", "type": "pool", "is_required": True}])
        with self.assertRaises(mbs.BedrockException):
            clients.create(**client_params)

        # Try creating a client with a wrong dependency
        client_params["dependencies"] = {"dep1": "my_pool_bad"}
        with self.assertRaises(mbs.BedrockException):
            clients.create(**client_params)

        # Try creating a client with the required dependency
        client_params["dependencies"] = {"dep1": "my_pool"}
        clients.create(**client_params)
        self.assertEqual(len(clients), 3)

        # Try creating a provider without the required dependency
        provider_params = self.make_provider_params([
            {"name": "dep1", "type": "pool", "is_required": True}])
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with a wrong dependency
        provider_params["dependencies"] = {"dep1": "my_pool_bad"}
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with the required dependency
        provider_params["dependencies"] = {"dep1": "my_pool"}
        providers.create(**provider_params)
        self.assertEqual(len(providers), 3)

    def test_dependency_on_xstream(self):
        providers = self.server.providers
        self.assertEqual(len(providers), 2)
        clients = self.server.clients
        self.assertEqual(len(clients), 2)

        # Try creating a client without the required dependency
        client_params = self.make_client_params([
            {"name": "dep1", "type": "xstream", "is_required": True}])
        with self.assertRaises(mbs.BedrockException):
            clients.create(**client_params)

        # Try creating a client with the wrong dependency
        client_params["dependencies"] = {"dep1": "my_xstream_bad"}
        with self.assertRaises(mbs.BedrockException):
            clients.create(**client_params)

        # Try creating a client with the required dependency
        client_params["dependencies"] = {"dep1": "my_xstream"}
        clients.create(**client_params)
        self.assertEqual(len(clients), 3)

        # Try creating a provider without the required dependency
        provider_params = self.make_provider_params([
            {"name": "dep1", "type": "xstream", "is_required": True}])
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with the wrong dependency
        provider_params["dependencies"] = {"dep1": "my_xstream_bad"}
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with the required dependency
        provider_params["dependencies"] = {"dep1": "my_xstream"}
        providers.create(**provider_params)
        self.assertEqual(len(providers), 3)

    def test_dependency_on_abt_io(self):
        providers = self.server.providers
        self.assertEqual(len(providers), 2)
        clients = self.server.clients
        self.assertEqual(len(clients), 2)

        # Try creating a client without the required dependency
        client_params = self.make_client_params([
            {"name": "dep1", "type": "abt_io", "is_required": True}])
        with self.assertRaises(mbs.BedrockException):
            clients.create(**client_params)

        # Try creating a client witht the wrong dependency
        client_params["dependencies"] = {"dep1": "my_abt_io_bad"}
        with self.assertRaises(mbs.BedrockException):
            clients.create(**client_params)

        # Try creating a client with the required dependency
        client_params["dependencies"] = {"dep1": "my_abt_io"}
        clients.create(**client_params)
        self.assertEqual(len(clients), 3)

        # Try creating a provider without the required dependency
        provider_params = self.make_provider_params([
            {"name": "dep1", "type": "abt_io", "is_required": True}])
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with the wrong dependency
        provider_params["dependencies"] = {"dep1": "my_abt_io_bad"}
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with the required dependency
        provider_params["dependencies"] = {"dep1": "my_abt_io"}
        providers.create(**provider_params)
        self.assertEqual(len(providers), 3)

    def test_dependency_on_mona(self):
        providers = self.server.providers
        self.assertEqual(len(providers), 2)
        clients = self.server.clients
        self.assertEqual(len(clients), 2)

        # Try creating a client without the required dependency
        client_params = self.make_client_params([
            {"name": "dep1", "type": "mona", "is_required": True}])
        with self.assertRaises(mbs.BedrockException):
            clients.create(**client_params)

        # Try creating a client with the wrong dependency
        client_params["dependencies"] = {"dep1": "my_mona_bad"}
        with self.assertRaises(mbs.BedrockException):
            clients.create(**client_params)

        # Try creating a client with the required dependency
        client_params["dependencies"] = {"dep1": "my_mona"}
        clients.create(**client_params)
        self.assertEqual(len(clients), 3)

        # Try creating a provider without the required dependency
        provider_params = self.make_provider_params([
            {"name": "dep1", "type": "mona", "is_required": True}])
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with the wrong dependency
        provider_params["dependencies"] = {"dep1": "my_mona_bad"}
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with the required dependency
        provider_params["dependencies"] = {"dep1": "my_mona"}
        providers.create(**provider_params)
        self.assertEqual(len(providers), 3)

    def test_dependency_on_client(self):
        providers = self.server.providers
        self.assertEqual(len(providers), 2)
        clients = self.server.clients
        self.assertEqual(len(clients), 2)

        # Try creating a client without the required dependency
        client_params = self.make_client_params([
            {"name": "dep1", "type": "module_a",
             "is_required": True, "kind": "client"}])
        with self.assertRaises(mbs.BedrockException):
            clients.create(**client_params)

        # Try creating a client with the wrong dependency
        client_params["dependencies"] = {"dep1": "my_client_a_bad"}
        with self.assertRaises(mbs.BedrockException):
            clients.create(**client_params)

        # Try creating a client with the required dependency
        client_params["dependencies"] = {"dep1": "my_client_a"}
        clients.create(**client_params)
        self.assertEqual(len(clients), 3)

        # Try creating a provider without the required dependency
        provider_params = self.make_provider_params([
            {"name": "dep1", "type": "module_a",
             "is_required": True, "kind": "client"}])
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with the wrong dependency
        provider_params["dependencies"] = {"dep1": "my_client_a_bad"}
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with the required dependency
        provider_params["dependencies"] = {"dep1": "my_client_a"}
        providers.create(**provider_params)
        self.assertEqual(len(providers), 3)

    def test_dependency_on_provider(self):
        providers = self.server.providers
        self.assertEqual(len(providers), 2)
        clients = self.server.clients
        self.assertEqual(len(clients), 2)

        # Try creating a client without the required dependency
        client_params = self.make_client_params([
            {"name": "dep1", "type": "module_a",
             "is_required": True, "kind": "provider"}])
        with self.assertRaises(mbs.BedrockException):
            clients.create(**client_params)

        # Try creating a client with the wrong dependency
        client_params["dependencies"] = {"dep1": "my_provider_a_bad"}
        with self.assertRaises(mbs.BedrockException):
            clients.create(**client_params)

        # Try creating a client with the required dependency
        client_params["dependencies"] = {"dep1": "my_provider_a"}
        clients.create(**client_params)
        self.assertEqual(len(clients), 3)

        # Try creating a provider without the required dependency
        provider_params = self.make_provider_params([
            {"name": "dep1", "type": "module_a",
             "is_required": True, "kind": "provider"}])
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with the wrong dependency
        provider_params["dependencies"] = {"dep1": "my_provider_a_bad"}
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with the required dependency
        provider_params["dependencies"] = {"dep1": "my_provider_a"}
        providers.create(**provider_params)
        self.assertEqual(len(providers), 3)

    def test_dependency_on_provider_with_id(self):
        providers = self.server.providers
        self.assertEqual(len(providers), 2)
        clients = self.server.clients
        self.assertEqual(len(clients), 2)

        client_params = self.make_client_params([
            {"name": "dep1", "type": "module_a",
             "is_required": True, "kind": "provider"}])

        # Try creating a client with dependency on the wrong provider ID
        client_params["dependencies"] = {"dep1": "module_a:999"}
        with self.assertRaises(mbs.BedrockException):
            clients.create(**client_params)

        # Try creating a client with dependency on the wrong provider type
        client_params["dependencies"] = {"dep1": "module_b:2"}
        with self.assertRaises(mbs.BedrockException):
            clients.create(**client_params)

        # Try creating a client with the required dependency
        client_params["dependencies"] = {"dep1": "module_a:1"}
        clients.create(**client_params)
        self.assertEqual(len(clients), 3)

        provider_params = self.make_provider_params([
            {"name": "dep1", "type": "module_a",
             "is_required": True, "kind": "provider"}])

        # Try creating a provider with dependency on the wrong provider ID
        provider_params["dependencies"] = {"dep1": "module_a:999"}
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with dependency on the wrong provider type
        provider_params["dependencies"] = {"dep1": "module_b:2"}
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with the required dependency
        provider_params["dependencies"] = {"dep1": "module_a:1"}
        providers.create(**provider_params)
        self.assertEqual(len(providers), 3)

    def test_dependency_on_ph_with_id_and_address(self):
        providers = self.server.providers
        self.assertEqual(len(providers), 2)
        clients = self.server.clients
        self.assertEqual(len(clients), 2)

        # Get the address of this process to use instead of "local"
        address = str(self.server.margo.engine.address)

        # Try creating a client without the required dependency
        client_params = self.make_client_params([
            {"name": "dep1", "type": "module_a",
             "is_required": True, "kind": "provider_handle"}])
        with self.assertRaises(mbs.BedrockException):
            clients.create(**client_params)

        # Try creating a client with the wrong provider ID
        client_params["dependencies"] = {"dep1": "module_a:999@{address}"}
        with self.assertRaises(mbs.BedrockException):
            clients.create(**client_params)

        # Try creating a client with the wrong module
        client_params["dependencies"] = {"dep1": "module_b:1@{address}"}
        with self.assertRaises(mbs.BedrockException):
            clients.create(**client_params)

        # Try creating a client with the required dependency
        client_params["dependencies"] = {"dep1": f"module_a:1@{address}"}
        clients.create(**client_params)
        self.assertEqual(len(clients), 3)

        # Try creating a provider without the required dependency
        provider_params = self.make_provider_params([
            {"name": "dep1", "type": "module_a",
             "is_required": True, "kind": "provider_handle"}])
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with the wrong provider ID
        provider_params["dependencies"] = {"dep1": "module_a:999@{address}"}
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with the wrong module
        provider_params["dependencies"] = {"dep1": "module_b:1@{address}"}
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with the required dependency
        provider_params["dependencies"] = {"dep1": f"module_a:1@{address}"}
        providers.create(**provider_params)
        self.assertEqual(len(providers), 3)

    def test_dependency_on_ph_with_id_and_rank(self):
        providers = self.server.providers
        self.assertEqual(len(providers), 2)
        clients = self.server.clients
        self.assertEqual(len(clients), 2)

        # Get the rank of the process to use instead of "local"
        rank = 0

        # Try creating a client without the required dependency
        client_params = self.make_client_params([
            {"name": "dep1", "type": "module_a",
             "is_required": True, "kind": "provider_handle"}])
        with self.assertRaises(mbs.BedrockException):
            clients.create(**client_params)

        # Try creating a client with the wrong provider ID
        client_params["dependencies"] = {"dep1": "module_a:999@{rank}"}
        with self.assertRaises(mbs.BedrockException):
            clients.create(**client_params)

        # Try creating a client with the wrong module
        client_params["dependencies"] = {"dep1": "module_b:1@{rank}"}
        with self.assertRaises(mbs.BedrockException):
            clients.create(**client_params)

        # Try creating a client with the required dependency
        client_params["dependencies"] = {"dep1": f"module_a:1@{rank}"}
        clients.create(**client_params)
        self.assertEqual(len(clients), 3)

        # Try creating a provider without the required dependency
        provider_params = self.make_provider_params([
            {"name": "dep1", "type": "module_a",
             "is_required": True, "kind": "provider_handle"}])
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with the wrong provider ID
        provider_params["dependencies"] = {"dep1": "module_a:999@{rank}"}
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with the wrong module
        provider_params["dependencies"] = {"dep1": "module_b:1@{rank}"}
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with the required dependency
        provider_params["dependencies"] = {"dep1": f"module_a:1@{rank}"}
        providers.create(**provider_params)
        self.assertEqual(len(providers), 3)


if __name__ == '__main__':
    unittest.main()
