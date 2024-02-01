import unittest
import pymargo.logging
import mochi.bedrock.server as mbs
import mochi.bedrock.spec as spec


class TestProviderManager(unittest.TestCase):

    def setUp(self):
        config = {
            "libraries": {
                "module_a": "libModuleA.so",
                "module_b": "libModuleB.so",
                "module_c": "libModuleC.so",
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
        params["pool"] = "__primary__"
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


if __name__ == '__main__':
    unittest.main()
