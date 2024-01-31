import unittest
import pymargo.logging
import mochi.bedrock.server as mbs
import mochi.bedrock.client as mbc
import mochi.bedrock.spec as spec
import tempfile
import json
import os.path


class TestServiceHandleInit(unittest.TestCase):

    def setUp(self):
        self.server = mbs.Server(address="na+sm")
        self.server.margo.engine.logger.set_log_level(pymargo.logging.level.critical)
        self.client = mbc.Client(self.server.margo.engine)

    def tearDown(self):
        del self.client
        self.server.finalize()
        del self.server

    def test_make_service_handle_from_str(self):
        address = str(self.server.margo.engine.address)
        sgh = self.client.make_service_handle(address)
        self.assertIsInstance(sgh, mbc.ServiceHandle)

    def test_make_service_handle_from_addr(self):
        address = self.server.margo.engine.address
        sgh = self.client.make_service_handle(address)
        self.assertIsInstance(sgh, mbc.ServiceHandle)


class TestServiceHandle(unittest.TestCase):

    def setUp(self):
        self.server = mbs.Server(address="na+sm")
        self.server.margo.engine.logger.set_log_level(pymargo.logging.level.critical)
        self.client = mbc.Client(self.server.margo.engine)
        self.sh = self.client.make_service_handle(
            self.server.margo.engine.address)

    def tearDown(self):
        del self.sh
        del self.client
        self.server.finalize()
        del self.server

    def test_client(self):
        self.assertEqual(self.sh.client, self.client)

    def test_config(self):
        config = self.sh.config
        self.assertIsInstance(config, dict)
        for k in ["margo", "providers", "clients", "ssg", "abt_io", "bedrock"]:
            self.assertIn(k, config)

    def test_spec(self):
        s = self.sh.spec
        self.assertIsInstance(s, spec.ProcSpec)

    def test_query(self):
        script = "return $__config__.margo.argobots;"
        result = self.sh.query(script)
        self.assertIsInstance(result, dict)
        # check that we can convert the result into an ArgobotsSpec
        s = spec.ArgobotsSpec.from_dict(result)

    def test_load_module(self):
        self.sh.load_module("module_a", "libModuleA.so")
        self.sh.load_module("module_b", "libModuleB.so")
        with self.assertRaises(mbc.ClientException):
            self.sh.load_module("module_c", "libModuleC.so")

    def add_pool(self, config):
        initial_num_pools = len(self.server.margo.pools)
        self.sh.add_pool(config)
        self.assertEqual(len(self.server.margo.pools), initial_num_pools+1)
        with self.assertRaises(mbc.ClientException):
            self.sh.add_pool(config)

    def test_add_pool_from_dict(self):
        config = {
            "name": "my_pool",
            "kind": "fifo_wait",
            "access": "mpmc"
        }
        self.add_pool(config)

    def test_add_pool_from_str(self):
        config = {
            "name": "my_pool",
            "kind": "fifo_wait",
            "access": "mpmc"
        }
        self.add_pool(json.dumps(config))

    def test_add_pool_from_spec(self):
        config = {
            "name": "my_pool",
            "kind": "fifo_wait",
            "access": "mpmc"
        }
        self.add_pool(spec.PoolSpec.from_dict(config))

    def test_remove_pool(self):
        config = {
            "name": "my_pool",
            "kind": "fifo_wait",
            "access": "mpmc"
        }
        self.add_pool(config)
        initial_num_pools = len(self.server.margo.pools)
        self.sh.remove_pool("my_pool")
        self.assertEqual(len(self.server.margo.pools), initial_num_pools - 1)
        with self.assertRaises(mbc.ClientException):
            self.server.margo.pools["my_pool"]

    def add_xstream(self, config):
        initial_num_xstreams = len(self.server.margo.xstreams)
        self.sh.add_xstream(config)
        self.assertEqual(len(self.server.margo.xstreams), initial_num_xstreams+1)
        with self.assertRaises(mbc.ClientException):
            self.sh.add_xstream(config)

    def test_add_xstream_from_dict(self):
        pool_config = {
            "name": "my_pool",
            "kind": "fifo_wait",
            "access": "mpmc"
        }
        self.add_pool(pool_config)
        config = {
            "name": "my_xstream",
            "scheduler": {
                "type": "basic_wait",
                "pools": ["my_pool"]
            }
        }
        self.add_xstream(config)

    def test_add_xstream_from_str(self):
        pool_config = {
            "name": "my_pool",
            "kind": "fifo_wait",
            "access": "mpmc"
        }
        self.add_pool(pool_config)
        config = {
            "name": "my_xstream",
            "scheduler": {
                "type": "basic_wait",
                "pools": ["my_pool"]
            }
        }
        self.add_xstream(json.dumps(config))

    def test_add_xstream_from_spec(self):
        pool_config = {
            "name": "my_pool",
            "kind": "fifo_wait",
            "access": "mpmc"
        }
        self.add_pool(pool_config)
        proc = self.server.spec
        proc.margo.argobots.xstreams.add(
            name="my_xstream",
            scheduler=spec.SchedulerSpec(
                type="basic_wait",
                pools=[proc.margo.argobots.pools["my_pool"]]
            )
        )
        self.add_xstream(proc.margo.argobots.xstreams["my_xstream"])

    def test_remove_xstream(self):
        self.test_add_xstream_from_dict()
        initial_num_xstreams = len(self.server.margo.xstreams)
        self.sh.remove_xstream("my_xstream")
        self.assertEqual(len(self.server.margo.xstreams), initial_num_xstreams - 1)
        with self.assertRaises(mbc.ClientException):
            self.server.margo.xstreams["my_xstream"]


if __name__ == '__main__':
    unittest.main()
