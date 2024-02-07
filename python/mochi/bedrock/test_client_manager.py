import unittest
import pymargo.logging
import mochi.bedrock.server as mbs
import mochi.bedrock.spec as spec


class TestClientManager(unittest.TestCase):

    def setUp(self):
        config = {
            "libraries": {
                "module_a": "libModuleA.so",
                "module_b": "libModuleB.so"
            },
            "clients": [
                {
                    "name": "my_client_A",
                    "type": "module_a"
                }
            ]
        }
        self.server = mbs.Server(address="na+sm", config=config)
        self.server.margo.engine.logger.set_log_level(pymargo.logging.level.critical)

    def tearDown(self):
        self.server.finalize()
        del self.server

    def test_get_client_manager(self):
        clients = self.server.clients
        self.assertIsInstance(clients, mbs.ClientManager)
        self.assertEqual(len(clients), 1)
        self.assertIn("my_client_A", clients)
        self.assertNotIn("not_my_client_A", clients)
        client_A = clients[0]
        client_B = clients["my_client_A"]
        self.assertEqual(client_A.name, client_B.name)
        self.assertEqual(client_A.type, client_B.type)
        self.assertEqual(client_A.handle, client_B.handle)
        with self.assertRaises(IndexError):
            c = clients[1]
        with self.assertRaises(mbs.BedrockException):
            c = clients["bla"]

    def test_client_manager_config(self):
        config = self.server.clients.config
        self.assertIsInstance(config, list)
        self.assertEqual(len(config), 1)
        client_1 = config[0]
        self.assertIsInstance(client_1, dict)
        for key in ["name", "config", "dependencies", "tags", "type"]:
            self.assertIn(key, client_1)

    def test_client_manager_spec(self):
        spec_list = self.server.clients.spec
        self.assertIsInstance(spec_list, list)
        for s in spec_list:
            self.assertIsInstance(s, spec.ClientSpec)

    def test_add_client(self):
        clients = self.server.clients
        clients.create(
            name="my_client_B",
            type="module_b")
        self.assertEqual(len(clients), 2)
        client_A = clients[1]
        client_B = clients["my_client_B"]
        self.assertEqual(client_A.name, client_B.name)
        self.assertEqual(client_A.type, client_B.type)
        self.assertEqual(client_A.handle, client_B.handle)

    def test_remove_client(self):
        self.test_add_client()
        clients = self.server.clients
        del clients["my_client_B"]
        self.assertEqual(len(clients), 1)
        with self.assertRaises(IndexError):
            c = clients[1]
        with self.assertRaises(mbs.BedrockException):
            c = clients["my_provider_B"]


if __name__ == '__main__':
    unittest.main()
