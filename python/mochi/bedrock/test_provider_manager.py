import unittest
import pymargo.logging
import mochi.bedrock.server as mbs
import mochi.bedrock.spec as spec


class TestProviderManager(unittest.TestCase):

    def setUp(self):
        config = {
            "libraries": [
                "./libModuleA.so",
                "./libModuleB.so"
            ],
            "providers": [
                {
                    "name": "my_provider_A",
                    "type": "module_a",
                    "provider_id": 1
                },
                {
                    "name": "my_providerB",
                    "type": "module_b",
                    "provider_id": 2,
                    "__if__": "false"
                }
            ]
        }
        self.server = mbs.Server(address="na+sm", config=config)
        self.server.margo.engine.logger.set_log_level(pymargo.logging.level.critical)

    def tearDown(self):
        self.server.finalize()
        del self.server

    def test_get_provider_manager(self):
        providers = self.server.providers
        self.assertIsInstance(providers, mbs.ProviderManager)
        self.assertEqual(len(providers), 1)
        self.assertIn("my_provider_A", providers)
        self.assertNotIn("not_my_provider_A", providers)
        provider_A = providers[0]
        provider_B = providers["my_provider_A"]
        self.assertEqual(provider_A.name, provider_B.name)
        self.assertEqual(provider_A.type, provider_B.type)
        self.assertEqual(provider_A.provider_id, provider_B.provider_id)
        with self.assertRaises(mbs.BedrockException):
            p = providers[1]
        with self.assertRaises(mbs.BedrockException):
            p = providers["bla"]

    def test_provider_manager_config(self):
        config = self.server.providers.config
        self.assertIsInstance(config, list)
        self.assertEqual(len(config), 1)
        provider_1 = config[0]
        self.assertIsInstance(provider_1, dict)
        for key in ["name", "config", "provider_id", "dependencies", "tags", "type"]:
            self.assertIn(key, provider_1)

    def test_provider_manager_spec(self):
        spec_list = self.server.providers.spec
        self.assertIsInstance(spec_list, list)
        for s in spec_list:
            self.assertIsInstance(s, spec.ProviderSpec)

    def test_add_provider(self):
        providers = self.server.providers
        providers.create(
            name="my_provider_B",
            provider_id=2,
            type="module_b")
        self.assertEqual(len(providers), 2)
        provider_A = providers[1]
        provider_B = providers["my_provider_B"]
        self.assertEqual(provider_A.name, provider_B.name)
        self.assertEqual(provider_A.type, provider_B.type)
        self.assertEqual(provider_A.provider_id, provider_B.provider_id)

    def test_remove_provider(self):
        self.test_add_provider()
        providers = self.server.providers
        del providers["my_provider_B"]
        self.assertEqual(len(providers), 1)
        with self.assertRaises(mbs.BedrockException):
            p = providers[1]
        with self.assertRaises(mbs.BedrockException):
            p = providers["my_provider_B"]


if __name__ == '__main__':
    unittest.main()
