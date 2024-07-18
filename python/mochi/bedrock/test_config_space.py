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
        config = space.sample_configuration()
        spec = ProcSpec.from_config(config=config, address='na+sm')

    def test_provider_config_space(self):
        from ConfigSpace import ConfigurationSpace, Integer
        pools = [PoolSpec(name=f'pool_{i}') for i in range(3)]

        config_cs = ConfigurationSpace()
        config_cs.add(Integer("x", (0,9)))
        config_cs.add(Integer("y", (1,42)))

        space = ProviderSpec.space(
            type='yokan', max_num_pools=5,
            tags=['tag1', 'tag2'],
            provider_config_space=config_cs)

        config = space.sample_configuration()

        def resolve_provider_config(config: 'Configuration', prefix: str) -> dict:
            x = config[f'{prefix}x']
            y = config[f'{prefix}y']
            return {'x':x, 'y':y}

        def resolve_provider_dependencies(config: 'Configuration', prefix: str) -> dict:
            return {'abc': 'def'}

        spec = ProviderSpec.from_config(
            name='my_yokan_provider', provider_id=1,
            pools=pools, config=config,
            resolve_provider_config=resolve_provider_config)

if __name__ == '__main__':
    unittest.main()
