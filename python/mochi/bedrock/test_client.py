import unittest
import mochi.bedrock.server as mbs
import mochi.bedrock.client as mbc
import mochi.bedrock.spec as spec


class TestClient(unittest.TestCase):

    def test_init_client_from_address(self):
        client = mbc.Client("na+sm")

    def test_init_client_from_engine(self):
        server = mbs.Server(address="na+sm")
        client = mbc.Client(server.margo.engine)
        del client
        server.finalize()
        del server


if __name__ == '__main__':
    unittest.main()
