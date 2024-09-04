import unittest
import mochi.bedrock.server as mbs
import mochi.bedrock.spec as spec


class TestServer(unittest.TestCase):

    def test_init_server(self):
        server = mbs.Server(address="na+sm")
        server.finalize()

    def test_init_from_config(self):
        config = {}
        server = mbs.Server(address="na+sm", config=config)
        server.finalize()

    def test_init_from_jx9(self):
        config = "return {};"
        server = mbs.Server(address="na+sm", config=config, config_type=mbs.ConfigType.JX9)
        server.finalize()

    def test_init_from_spec(self):
        p = spec.ProcSpec(margo="na+sm")
        server = mbs.Server.from_spec(p)
        server.finalize()

    def test_init_server_invalid_config(self):
        with self.assertRaises(mbs.BedrockException):
            server = mbs.Server(address="na+sm", config="{[")

    def test_init_server_invalid_jx9(self):
        with self.assertRaises(mbs.BedrockException):
            server = mbs.Server(address="na+sm", config="{[", config_type=mbs.ConfigType.JX9)

    def test_server_config(self):
        server = mbs.Server(address="na+sm")
        config = server.config
        self.assertIsInstance(config, dict)
        for expected_key in ["margo", "abt_io", "bedrock", "providers", "clients", "libraries", "mona"]:
            self.assertIn(expected_key, config)
        server.finalize()

    def test_server_spec(self):
        server = mbs.Server(address="na+sm")
        s = server.spec
        self.assertIsInstance(s, spec.ProcSpec)


if __name__ == '__main__':
    unittest.main()
