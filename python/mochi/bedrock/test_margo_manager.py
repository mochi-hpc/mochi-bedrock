import unittest
import mochi.bedrock.server as mbs
import mochi.bedrock.spec as spec


class TestMargoInstance(unittest.TestCase):

    def setUp(self):
        self.server = mbs.Server(address="na+sm")

    def tearDown(self):
        self.server.finalize()
        del self.server

    def test_get_margo_manager(self):
        margo = self.server.margo
        self.assertIsInstance(margo, mbs.MargoManager)

    def test_margo_manager_config(self):
        config = self.server.margo.config
        self.assertIsInstance(config, dict)
        for expected_key in ["mercury", "argobots"]:
            self.assertIn(expected_key, config)

    def test_margo_manager_spec(self):
        s = self.server.margo.spec
        self.assertIsInstance(s, spec.MargoSpec)



if __name__ == '__main__':
    unittest.main()
