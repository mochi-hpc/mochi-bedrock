import unittest
import pymargo.logging
import mochi.bedrock.server as mbs
import mochi.bedrock.spec as spec


class TestAbtIOManager(unittest.TestCase):

    def setUp(self):
        config = {
            "abt_io": [
                {
                    "name": "my_abtio",
                    "pool": "__primary__",
                    "config": {}
                }
            ]
        }
        self.server = mbs.Server(address="na+sm", config=config)
        self.server.margo.engine.logger.set_log_level(pymargo.logging.level.critical)

    def tearDown(self):
        self.server.finalize()
        del self.server

    def test_get_abtio_manager(self):
        abtio = self.server.abtio
        self.assertIsInstance(abtio, mbs.AbtIOManager)
        self.assertEqual(len(abtio), 1)
        self.assertIn("my_abtio", abtio)
        self.assertNotIn("not_my_abtio", abtio)
        abtio_A = abtio[0]
        abtio_B = abtio["my_abtio"]
        self.assertEqual(abtio_A.name, abtio_B.name)
        self.assertEqual(abtio_A.type, abtio_B.type)
        self.assertEqual(abtio_A.handle, abtio_B.handle)
        with self.assertRaises(mbs.BedrockException):
            abtio[1]
        with self.assertRaises(mbs.BedrockException):
            abtio["bla"]

    def test_abtio_manager_config(self):
        config = self.server.abtio.config
        self.assertIsInstance(config, list)
        self.assertEqual(len(config), 1)
        abt_io_1 = config[0]
        self.assertIsInstance(abt_io_1, dict)
        for key in ["name", "pool", "config"]:
            self.assertIn(key, abt_io_1)

    def test_abtio_manager_spec(self):
        spec_list = self.server.abtio.spec
        self.assertIsInstance(spec_list, list)
        for s in spec_list:
            self.assertIsInstance(s, spec.AbtIOSpec)

    def test_add_abtio_instance(self):
        abtio = self.server.abtio
        abtio.create(
            name="my_abtio_2",
            pool="__primary__")
        self.assertEqual(len(abtio), 2)
        abtio_A = abtio[1]
        abtio_B = abtio["my_abtio_2"]
        self.assertEqual(abtio_A.name, abtio_B.name)
        self.assertEqual(abtio_A.type, abtio_B.type)
        self.assertEqual(abtio_A.handle, abtio_B.handle)


if __name__ == '__main__':
    unittest.main()
