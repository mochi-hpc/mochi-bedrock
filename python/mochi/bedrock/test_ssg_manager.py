import unittest
import pymargo.logging
import mochi.bedrock.server as mbs
import mochi.bedrock.spec as spec


class TestSSGManager(unittest.TestCase):

    def setUp(self):
        config = {
            "ssg": [
                {
                    "name": "my_group",
                    "pool": "__primary__",
                    "bootstrap": "init",
                    "config": {}
                }
            ]
        }
        self.server = mbs.Server(address="na+sm", config=config)
        self.server.margo.engine.logger.set_log_level(pymargo.logging.level.critical)

    def tearDown(self):
        self.server.finalize()
        del self.server

    def test_get_ssg_manager(self):
        ssg = self.server.ssg
        self.assertIsInstance(ssg, mbs.SSGManager)
        self.assertEqual(len(ssg), 1)
        ssg_A = ssg[0]
        ssg_B = ssg["my_group"]
        self.assertEqual(ssg_A.name, ssg_B.name)
        self.assertEqual(ssg_A.type, ssg_B.type)
        self.assertEqual(ssg_A.handle, ssg_B.handle)
        with self.assertRaises(mbs.BedrockException):
            ssg[1]
        with self.assertRaises(mbs.BedrockException):
            ssg["bla"]


    def test_ssg_manager_config(self):
        config = self.server.ssg.config
        self.assertIsInstance(config, list)
        self.assertEqual(len(config), 1)
        ssg_1 = config[0]
        self.assertIsInstance(ssg_1, dict)
        for key in ["name", "pool", "swim", "credential", "bootstrap", "group_file"]:
            self.assertIn(key, ssg_1)

    def test_ssg_manager_spec(self):
        spec_list = self.server.ssg.spec
        self.assertIsInstance(spec_list, list)
        for s in spec_list:
            self.assertIsInstance(s, spec.SSGSpec)

    def test_add_ssg_group(self):
        ssg = self.server.ssg
        ssg.create(
            name="my_group_2",
            pool="__primary__")
        self.assertEqual(len(ssg), 2)
        ssg_A = ssg[1]
        ssg_B = ssg["my_group_2"]
        self.assertEqual(ssg_A.name, ssg_B.name)
        self.assertEqual(ssg_A.type, ssg_B.type)
        self.assertEqual(ssg_A.handle, ssg_B.handle)

    def test_resolve(self):
        ssg = self.server.ssg
        addr = ssg.resolve("ssg://my_group/0")
        self.assertIsInstance(addr, pymargo.core.Address)
        self.assertEqual(addr, self.server.margo.engine.address)


if __name__ == '__main__':
    unittest.main()
