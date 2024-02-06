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
            self.sh.load_module("module_x", "libModuleX.so")

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

    def test_add_ssg_group_from_dict(self):
        group_config = {
            "name": "my_group",
            "pool": "__primary__",
            "bootstrap": "init",
            "swim": {
                "disabled": True
            }
        }
        self.sh.add_ssg_group(group_config)
        group = self.server.ssg["my_group"]
        self.assertIsInstance(group, mbs.SSGGroup)

    def test_add_ssg_group_from_str(self):
        group_config = {
            "name": "my_group",
            "pool": "__primary__",
            "bootstrap": "init",
            "swim": {
                "disabled": True
            }
        }
        self.sh.add_ssg_group(json.dumps(group_config))
        group = self.server.ssg["my_group"]
        self.assertIsInstance(group, mbs.SSGGroup)

    def test_add_ssg_group_from_spec(self):
        group_config = spec.SSGSpec(
            name="my_group",
            pool=spec.PoolSpec(name="__primary__", kind="fifo_wait", access="mpmc"),
            bootstrap="init",
            swim=spec.SwimSpec(disabled=True)
        )
        self.sh.add_ssg_group(group_config)
        group = self.server.ssg["my_group"]
        self.assertIsInstance(group, mbs.SSGGroup)

    def test_add_abtio_instance_from_dict(self):
        abtio_config = {
            "name": "my_abtio",
            "pool": "__primary__",
            "config": {}
        }
        self.sh.add_abtio_instance(abtio_config)
        abtio = self.server.abtio["my_abtio"]
        self.assertIsInstance(abtio, mbs.AbtIOInstance)

    def test_add_abtio_instance_from_src(self):
        abtio_config = {
            "name": "my_abtio",
            "pool": "__primary__",
            "config": {}
        }
        self.sh.add_abtio_instance(json.dumps(abtio_config))
        abtio = self.server.abtio["my_abtio"]
        self.assertIsInstance(abtio, mbs.AbtIOInstance)

    def test_add_abtio_instance_from_spec(self):
        abtio_config = spec.AbtIOSpec(
            name="my_abtio",
            pool=spec.PoolSpec(name="__primary__", kind="fifo_wait", access="mpmc"),
            config={}
        )
        self.sh.add_abtio_instance(abtio_config)
        abtio = self.server.abtio["my_abtio"]
        self.assertIsInstance(abtio, mbs.AbtIOInstance)

    def test_add_client_from_dict(self):
        self.test_load_module()
        client_config = {
            "name": "my_client",
            "type": "module_a",
            "config": {},
            "dependencies": {},
            "tags": ["my_tag_1", "my_tag_2"]
        }
        self.sh.add_client(client_config)
        proc_spec = self.server.spec
        client_spec = proc_spec.clients["my_client"]

    def test_add_client_from_src(self):
        self.test_load_module()
        client_config = {
            "name": "my_client",
            "type": "module_a",
            "config": {},
            "dependencies": {},
            "tags": ["my_tag_1", "my_tag_2"]
        }
        self.sh.add_client(json.dumps(client_config))
        proc_spec = self.server.spec
        client_spec = proc_spec.clients["my_client"]

    def test_add_client_from_spec(self):
        self.test_load_module()
        client_config = spec.ClientSpec(
            name="my_client",
            type="module_a",
            config={},
            dependencies={},
            tags=["my_tag_1", "my_tag_2"]
        )
        self.sh.add_client(client_config)
        proc_spec = self.server.spec
        client_spec = proc_spec.clients["my_client"]

    def test_add_provider_from_dict(self):
        self.test_load_module()
        provider_config = {
            "name": "my_provider",
            "type": "module_a",
            "pool": "__primary__",
            "provider_id": 42,
            "config": {},
            "dependencies": {},
            "tags": ["my_tag_1", "my_tag_2"]
        }
        self.sh.add_provider(provider_config)
        proc_spec = self.server.spec
        provider_spec = proc_spec.providers["my_provider"]

    def test_add_provider_from_str(self):
        self.test_load_module()
        provider_config = {
            "name": "my_provider",
            "type": "module_a",
            "pool": "__primary__",
            "provider_id": 42,
            "config": {},
            "dependencies": {},
            "tags": ["my_tag_1", "my_tag_2"]
        }
        self.sh.add_provider(json.dumps(provider_config))
        proc_spec = self.server.spec
        provider_spec = proc_spec.providers["my_provider"]

    def test_add_provider_from_spec(self):
        self.test_load_module()
        provider_config = spec.ProviderSpec(
            name="my_provider",
            type="module_a",
            pool=spec.PoolSpec(name="__primary__"),
            provider_id=42,
            config={},
            dependencies={},
            tags=["my_tag_1", "my_tag_2"]
        )
        self.sh.add_provider(provider_config)
        proc_spec = self.server.spec
        provider_spec = proc_spec.providers["my_provider"]



if __name__ == '__main__':
    unittest.main()
