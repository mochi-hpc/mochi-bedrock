import unittest
from mochi.bedrock.spec import *


class TestConfigSpace(unittest.TestCase):

    def test_mercury_config_space(self):
        space = MercurySpec.space()
        config = space.sample_configuration()
        spec = MercurySpec.from_config(address='na+sm', config=config)

    def test_pool_config_space(self):
        space = PoolSpec.space()
        config = space.sample_configuration()
        spec = PoolSpec.from_config(name='my_pool', config=config)

    def test_sched_config_space(self):
        pool1 = PoolSpec(name='my_pool_1')
        pool2 = PoolSpec(name='my_pool_2')
        space = SchedulerSpec.space(max_num_pools=2)
        config = space.sample_configuration()
        spec = SchedulerSpec.from_config(config=config, pools=[pool1, pool2])

    def test_xstream_config_space(self):
        pool1 = PoolSpec(name='my_pool_1')
        pool2 = PoolSpec(name='my_pool_2')
        space = XstreamSpec.space(max_num_pools=2)
        config = space.sample_configuration()
        spec = XstreamSpec.from_config(name="my_xstream",
                                       config=config, pools=[pool1, pool2])

    def test_argobots_config_space(self):
        space = ArgobotsSpec.space(num_pools=2, num_xstreams=3)
        space = ArgobotsSpec.space(num_pools=(2,3), num_xstreams=3)
        space = ArgobotsSpec.space(num_pools=2, num_xstreams=(2,3))
        space = ArgobotsSpec.space(num_pools=(2,3), num_xstreams=(2,3))
        config = space.sample_configuration()
        spec = ArgobotsSpec.from_config(config)

    def test_margo_config_space(self):
        space = MargoSpec.space(num_pools=(2,5), num_xstreams=(2,3))
        config = space.sample_configuration()
        spec = MargoSpec.from_config(config=config, address='na+sm')

    def test_proc_config_space(self):
        space = ProcSpec.space(num_pools=(2,5), num_xstreams=(2,3))
        print(space)
        config = space.sample_configuration()
        print(config)
        spec = ProcSpec.from_config(config=config, address='na+sm')
        print(spec.to_json(indent=4))

if __name__ == '__main__':
    unittest.main()
