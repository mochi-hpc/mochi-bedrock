import unittest
import pymargo.logging
import mochi.bedrock.server as mbs
import mochi.bedrock.spec as spec


class TestMargoInstance(unittest.TestCase):

    def setUp(self):
        config = {
            "margo": {
                "argobots": {
                    "pools": [
                        { "name": "my_pool", "type": "fifo_wait", "access": "mpmc" }
                    ],
                    "xstreams": [
                        { "name": "my_xstream",
                          "scheduler": {
                            "type": "basic_wait",
                            "pools": [ "my_pool" ]
                          }
                        }
                    ]
                }
            }
        }
        self.server = mbs.Server(address="na+sm", config=config)
        self.server.margo.engine.logger.set_log_level(pymargo.logging.level.critical)

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
        self.assertIn("pools", config["argobots"])
        self.assertEqual(len(config["argobots"]["pools"]), 2)

    def test_margo_manager_spec(self):
        s = self.server.margo.spec
        self.assertIsInstance(s, spec.MargoSpec)

    def test_default_handler_pool(self):
        pool = self.server.margo.default_handler_pool
        self.assertIsInstance(pool, mbs.Pool)
        self.assertEqual(pool.name, "__primary__")

    def test_pool_list(self):
        pools = self.server.margo.pools
        self.assertIsInstance(pools, mbs.PoolList)
        self.assertEqual(len(pools), 2)
        my_pool = pools["my_pool"]
        self.assertIsInstance(my_pool, mbs.Pool)
        self.assertEqual(my_pool.name, "my_pool")

    def test_add_pool(self):
        self.server.margo.pools.create({
            "name": "my_pool_2",
            "type": "fifo",
            "access": "mpmc"
        })
        pools = self.server.margo.pools
        self.assertEqual(len(pools), 3)
        my_pool_2 = pools["my_pool_2"]
        self.assertIsInstance(my_pool_2, mbs.Pool)
        self.assertEqual(my_pool_2.name, "my_pool_2")

    def test_remove_pool(self):
        self.test_add_pool()
        del self.server.margo.pools["my_pool_2"]
        pools = self.server.margo.pools
        self.assertEqual(len(pools), 2)
        with self.assertRaises(mbs.BedrockException):
            my_pool_2 = pools["my_pool_2"]

    def test_remove_pool_in_use(self):
        with self.assertRaises(mbs.BedrockException):
            del self.server.margo.pools["my_pool"]

    def test_xstream_list(self):
        xstreams = self.server.margo.xstreams
        self.assertIsInstance(xstreams, mbs.XstreamList)
        self.assertEqual(len(xstreams), 2)
        my_xstream = xstreams["my_xstream"]
        self.assertIsInstance(my_xstream, mbs.Xstream)
        self.assertEqual(my_xstream.name, "my_xstream")

    def test_add_xstream(self):
        self.server.margo.xstreams.create({
            "name": "my_xstream_2",
            "scheduler": {
                "type": "basic_wait",
                "pools": [ "my_pool" ]
            }
        })
        xstreams = self.server.margo.xstreams
        self.assertEqual(len(xstreams), 3)
        my_xstream_2 = xstreams["my_xstream_2"]
        self.assertIsInstance(my_xstream_2, mbs.Xstream)
        self.assertEqual(my_xstream_2.name, "my_xstream_2")

    def test_remove_xstream(self):
        self.test_add_xstream()
        del self.server.margo.xstreams["my_xstream_2"]
        xstreams = self.server.margo.xstreams
        self.assertEqual(len(xstreams), 2)
        with self.assertRaises(mbs.BedrockException):
            my_xstream_2 = xstreams["my_xstream_2"]


if __name__ == '__main__':
    unittest.main()
