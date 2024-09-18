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
            "libraries": [
                "./libModuleA.so",
                "./libModuleB.so",
                "./libModuleC.so",
            ],
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
            ]
        }
        self.server = mbs.Server(address="na+sm", config=config)
        self.server.margo.engine.logger.set_log_level(pymargo.logging.level.critical)

    def tearDown(self):
        self.server.finalize()
        del self.server

    def make_provider_params(self, expected_dependencies: dict={}):
        params = {
            "name": "my_provider_C",
            "type": "module_c",
            "config": {
                "expected_client_dependencies": expected_dependencies
            }
        }
        params["provider_id"] = 3
        params["config"]["expected_provider_dependencies"] = expected_dependencies
        return params

    def test_no_dependency(self):
        providers = self.server.providers
        self.assertEqual(len(providers), 2)

        provider_params = self.make_provider_params()
        providers.create(**provider_params)

        self.assertEqual(len(providers), 3)

    def test_optional_dependency(self):
        providers = self.server.providers
        self.assertEqual(len(providers), 2)

        provider_params = self.make_provider_params([
            {"name": "dep1",
             "type": "module_a",
             "is_array": False,
             "is_required": False,
            }])
        providers.create(**provider_params)

        self.assertEqual(len(providers), 3)

    def test_required_dependency(self):
        providers = self.server.providers
        self.assertEqual(len(providers), 2)

        # Try creating a provider without the required dependency
        provider_params = self.make_provider_params([
            {"name": "dep1",
             "type": "module_a",
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

    def test_dependency_on_pool_by_index(self):
        providers = self.server.providers
        self.assertEqual(len(providers), 2)

        # Try creating a provider without the required dependency
        provider_params = self.make_provider_params([
            {"name": "dep1", "type": "pool", "is_required": True}])
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with a wrong dependency
        provider_params["dependencies"] = {"dep1": 123}
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with the required dependency
        provider_params["dependencies"] = {"dep1": 1}
        providers.create(**provider_params)
        self.assertEqual(len(providers), 3)

    def test_dependency_on_xstream(self):
        providers = self.server.providers
        self.assertEqual(len(providers), 2)

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

    def test_dependency_on_xstream_by_index(self):
        providers = self.server.providers
        self.assertEqual(len(providers), 2)

        # Try creating a provider without the required dependency
        provider_params = self.make_provider_params([
            {"name": "dep1", "type": "xstream", "is_required": True}])
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with the wrong dependency
        provider_params["dependencies"] = {"dep1": 123}
        with self.assertRaises(mbs.BedrockException):
            providers.create(**provider_params)

        # Try creating a provider with the required dependency
        provider_params["dependencies"] = {"dep1": 1}
        providers.create(**provider_params)
        self.assertEqual(len(providers), 3)

    def test_dependency_on_provider(self):
        providers = self.server.providers
        self.assertEqual(len(providers), 2)

        # Try creating a provider without the required dependency
        provider_params = self.make_provider_params([
            {"name": "dep1", "type": "module_a",
             "is_required": True}])
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

        provider_params = self.make_provider_params([
            {"name": "dep1", "type": "module_a",
             "is_required": True}])

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

        # Get the address of this process to use instead of "local"
        address = str(self.server.margo.engine.address)

        # Try creating a provider without the required dependency
        provider_params = self.make_provider_params([
            {"name": "dep1", "type": "module_a",
             "is_required": True}])
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

        # Get the rank of the process to use instead of "local"
        rank = 0

        # Try creating a provider without the required dependency
        provider_params = self.make_provider_params([
            {"name": "dep1", "type": "module_a",
             "is_required": True}])
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
