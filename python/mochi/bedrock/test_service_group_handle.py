import unittest
import mochi.bedrock.server as mbs
import mochi.bedrock.client as mbc
import mochi.bedrock.spec as spec
import tempfile
import os.path


class TestServiceGroupHandleInit(unittest.TestCase):

    def setUp(self):
        self.tempdir = tempfile.TemporaryDirectory()
        self.groupfile = os.path.join(self.tempdir.name, "group.ssg")
        config = {
            "ssg": [{
                "name": "my_group",
                "bootstrap": "init",
                "group_file": self.groupfile
            }]
        }
        self.server = mbs.Server(address="na+sm", config=config)
        self.client = mbc.Client(self.server.margo.engine)

    def tearDown(self):
        self.tempdir.cleanup()
        del self.client
        self.server.finalize()
        del self.server

    def test_make_service_group_handle_from_file(self):
        """
        Note: because the server and client are on the same process,
        trying to open the group file will lead to an SSG error
        about the group ID already existing.
        """
        # sgh = self.client.make_service_group_handle(self.groupfile)
        # self.assertIsInstance(sgh, mbc.ServiceGroupHandle)

    def test_make_service_group_handle_from_gid(self):
        ssg_group = self.server.ssg["my_group"]
        gid = ssg_group.handle
        sgh = self.client.make_service_group_handle(gid)
        self.assertIsInstance(sgh, mbc.ServiceGroupHandle)

    def test_make_service_group_handle_from_address(self):
        address = str(self.server.margo.engine.address)
        sgh = self.client.make_service_group_handle([address])
        self.assertIsInstance(sgh, mbc.ServiceGroupHandle)


class TestServiceGroupHandle(unittest.TestCase):

    def setUp(self):
        self.tempdir = tempfile.TemporaryDirectory()
        self.groupfile = os.path.join(self.tempdir.name, "group.ssg")
        config = {
            "ssg": [{
                "name": "my_group",
                "bootstrap": "init",
                "group_file": self.groupfile
            }]
        }
        self.server = mbs.Server(address="na+sm", config=config)
        self.client = mbc.Client(self.server.margo.engine)
        self.sgh = self.client.make_service_group_handle(
            self.server.ssg["my_group"].handle)

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
        for k in ["margo", "providers", "clients", "ssg", "abt_io", "bedrock"]:
            self.assertIn(k, config[self_address])

    def test_spec(self):
        s = self.sgh.spec
        self.assertIsInstance(s, dict)
        self.assertEqual(len(s.keys()), 1)
        self_address = str(self.server.margo.engine.address)
        self.assertIn(self_address, s)
        self.assertIsInstance(s[self_address], spec.ProcSpec)

if __name__ == '__main__':
    unittest.main()
