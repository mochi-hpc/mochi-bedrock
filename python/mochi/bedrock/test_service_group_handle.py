import unittest
import mochi.bedrock.server as mbs
import mochi.bedrock.client as mbc
import mochi.bedrock.spec as spec
import tempfile
import os.path


class TestServiceGroupHandleInit(unittest.TestCase):

    def setUp(self):
        self.tempdir = tempfile.TemporaryDirectory()
        self.groupfile = os.path.join(self.tempdir.name, "group.flock")
        config = {
            "libraries": {
                "flock": "libflock-bedrock-module.so"
            },
            "providers": [{
                "name": "my_group",
                "type": "flock",
                "provider_id" : 1,
                "config": {
                  "bootstrap": "self",
                  "file": self.groupfile,
                  "group": {
                      "type": "static"
                  }
              }
            }]
        }
        self.server = mbs.Server(address="na+sm", config=config)
        self.client = mbc.Client(self.server.margo.engine)

    def tearDown(self):
        self.tempdir.cleanup()
        del self.client
        self.server.finalize()
        del self.server

    def test_make_service_group_handle_from_address(self):
        address = str(self.server.margo.engine.address)
        sgh = self.client.make_service_group_handle([address])
        self.assertIsInstance(sgh, mbc.ServiceGroupHandle)

    def test_make_service_group_handle_from_file(self):
        sgh = self.client.make_service_group_handle_from_flock(self.groupfile)
        self.assertIsInstance(sgh, mbc.ServiceGroupHandle)


class TestServiceGroupHandle(unittest.TestCase):

    def setUp(self):
        self.tempdir = tempfile.TemporaryDirectory()
        self.groupfile = os.path.join(self.tempdir.name, "group.flock")
        config = {
            "libraries": {
                "flock": "libflock-bedrock-module.so"
            },
            "providers": [{
                "name": "my_group",
                "type": "flock",
                "provider_id" : 1,
                "config": {
                  "bootstrap": "self",
                  "file": self.groupfile,
                  "group": {
                      "type": "static"
                  }
              }
            }]
        }
        self.server = mbs.Server(address="na+sm", config=config)
        self.client = mbc.Client(self.server.margo.engine)
        self.sgh = self.client.make_service_group_handle_from_flock(self.groupfile)

    def tearDown(self):
        del self.sgh
        self.tempdir.cleanup()
        del self.client
        self.server.finalize()
        del self.server

    def test_len(self):
        self.assertEqual(len(self.sgh), 1)
        self.assertEqual(self.sgh.size, 1)

    def test_getitem(self):
        self.assertIsInstance(self.sgh[0], mbc.ServiceHandle)
        with self.assertRaises(mbc.ClientException):
            self.sgh[1]

    def test_config(self):
        config = self.sgh.config
        self.assertIsInstance(config, dict)
        self.assertEqual(len(config.keys()), 1)
        self_address = str(self.server.margo.engine.address)
        self.assertIn(self_address, config)
        for k in ["margo", "providers", "bedrock"]:
            self.assertIn(k, config[self_address])

    def test_spec(self):
        s = self.sgh.spec
        self.assertIsInstance(s, dict)
        self.assertEqual(len(s.keys()), 1)
        self_address = str(self.server.margo.engine.address)
        self.assertIn(self_address, s)
        self.assertIsInstance(s[self_address], spec.ProcSpec)

    def test_query(self):
        script = "return $__config__.margo.argobots;"
        result = self.sgh.query(script)
        self.assertIsInstance(result, dict)
        self.assertEqual(len(result.keys()), 1)
        self_address = str(self.server.margo.engine.address)
        self.assertIn(self_address, result)
        # check that we can convert the result into an ArgobotsSpec
        s = spec.ArgobotsSpec.from_dict(result[self_address])


if __name__ == '__main__':
    unittest.main()
