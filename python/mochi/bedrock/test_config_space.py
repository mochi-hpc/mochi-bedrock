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

        def resolve_provider_config(config: 'Configuration', prefix: str) -> dict:
            x = config[f'{prefix}x']
            y = config[f'{prefix}y']
            return {'x':x, 'y':y}

        def resolve_provider_dependencies(config: 'Configuration', prefix: str) -> dict:
            return {'abc': 'def'}

        space = ProviderSpec.space(
            type='yokan', max_num_pools=5,
            tags=['tag1', 'tag2'],
            provider_config_space=config_cs,
            provider_config_resolver=resolve_provider_config,
            dependency_resolver=resolve_provider_dependencies)

        config = space.sample_configuration()

        spec = ProviderSpec.from_config(
            name='my_yokan_provider', provider_id=1,
            pools=pools, config=config)

    def test_proc_with_providers(self):
        from ConfigSpace import ConfigurationSpace, Integer

        max_num_pools = 5

        provider_config_cs = ConfigurationSpace()
        provider_config_cs.add(Integer("x", (0,9)))
        provider_config_cs.add(Integer("y", (1,42)))

        def resolve_provider_config(config: 'Configuration', prefix: str) -> dict:
            x = config[f'{prefix}x']
            y = config[f'{prefix}y']
            return {'x':x, 'y':y}

        def resolve_provider_dependencies(config: 'Configuration', prefix: str) -> dict:
            return {'abc': 'def'}

        provider_space_factories = [
            {
                "family": "databases",
                "space": ProviderSpec.space(
                    type='yokan',
                    max_num_pools=max_num_pools, tags=['tag1', 'tag2'],
                    provider_config_space=provider_config_cs,
                    provider_config_resolver=resolve_provider_config,
                    dependency_resolver=resolve_provider_dependencies),
                "count": (1,3)
            }
        ]

        space = ProcSpec.space(num_pools=(2, max_num_pools), num_xstreams=(2, 3),
                               provider_space_factories=provider_space_factories)
        config = space.sample_configuration()
        spec = ProcSpec.from_config(address='na+sm', config=config)

    def test_service_config_space(self):

        proc_type_a = ProcSpec.space(num_pools=2, num_xstreams=1)
        proc_type_b = ProcSpec.space(num_pools=1, num_xstreams=2)

        space = ServiceSpec.space(
            process_space_factories=[
                {
                    'family': 'proc_type_a',
                    'space': proc_type_a,
                    'count': 2
                },
                {
                    'family': 'proc_type_b',
                    'space': proc_type_b,
                    'count': 2
                }
            ])

        config = space.sample_configuration()

        spec = ServiceSpec.from_config(address='na+sm', config=config)

if __name__ == '__main__':
    unittest.main()
