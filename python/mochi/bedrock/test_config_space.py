import unittest
from mochi.bedrock.spec import *


class TestConfigSpace(unittest.TestCase):

    def test_mercury_config_space(self):
        space = MercurySpec.space().freeze()
        #print(space)
        config = space.sample_configuration()
        #print(config)
        spec = MercurySpec.from_config(address='na+sm', config=config)
        #print(spec.to_json(indent=4))

    def test_pool_config_space(self):
        space = PoolSpec.space().freeze()
        #print(space)
        config = space.sample_configuration()
        #print(config)
        spec = PoolSpec.from_config(name='my_pool', config=config)
        #print(spec.to_json(indent=4))

    def test_sched_config_space(self):
        pool1 = PoolSpec(name='my_pool_1')
        pool2 = PoolSpec(name='my_pool_2')
        space = SchedulerSpec.space().freeze()
        #print(space)
        config = space.sample_configuration()
        #print(config)
        spec = SchedulerSpec.from_config(config=config, pools=[pool1, pool2])
        #print(spec.to_json(indent=4))

    def test_xstream_config_space(self):
        pool1 = PoolSpec(name='my_pool_1')
        pool2 = PoolSpec(name='my_pool_2')
        space = XstreamSpec.space().freeze()
        #print(space)
        config = space.sample_configuration()
        #print(config)
        spec = XstreamSpec.from_config(name="my_xstream",
                                       config=config, pools=[pool1, pool2])
        #print(spec.to_json(indent=4))

    def test_argobots_config_space(self):
        space = ArgobotsSpec.space(num_pools=2, num_xstreams=3).freeze()
        space = ArgobotsSpec.space(num_pools=(2,3), num_xstreams=3).freeze()
        space = ArgobotsSpec.space(num_pools=2, num_xstreams=(2,3)).freeze()
        space = ArgobotsSpec.space(num_pools=(2,3), num_xstreams=(2,3)).freeze()
        #print(space)
        config = space.sample_configuration()
        #print(config)
        spec = ArgobotsSpec.from_config(config=config)
        #print(spec.to_json(indent=4))

    def test_margo_config_space(self):
        space = MargoSpec.space(num_pools=(2,5), num_xstreams=(2,3),
                                allow_more_pools_than_xstreams=True).freeze()
        #print(space)
        config = space.sample_configuration()
        #print(config)
        spec = MargoSpec.from_config(config=config, address='na+sm')
        #print(spec.to_json(indent=4))

    def test_proc_config_space(self):
        space = ProcSpec.space(num_pools=(2,3), num_xstreams=(2,5)).freeze()
        #print(space)
        config = space.sample_configuration()
        #print(config)
        spec = ProcSpec.from_config(config=config, address='na+sm')
        #print(spec.to_json(indent=4))

    def test_provider_config_space(self):
        from .config_space import ConfigurationSpace, Integer
        # pools to select from
        # pools = [PoolSpec(name=f'pool_{i}') for i in range(5)]

        # config space for the provider
        config_cs = ConfigurationSpace()
        config_cs.add(Integer("x", (0,9)))
        config_cs.add(Integer("y", (1,42)))

        # function to resolve the configuration
        def resolve_provider_config(config: 'Configuration', prefix: str) -> dict:
            x = config[f'{prefix}x']
            y = config[f'{prefix}y']
            return {'x':x, 'y':y}

        # function to resolve the dependencies
        def resolve_provider_dependencies(config: 'Configuration', prefix: str) -> dict:
            return {'abc': 'def'}

        space = ProviderSpec.space(
            type='yokan',
            tags=['tag1', 'tag2'],
            provider_config_space=config_cs,
            provider_config_resolver=resolve_provider_config,
            dependency_resolver=resolve_provider_dependencies).freeze()
        #print(space)

        config = space.sample_configuration()
        #print(config)

        spec = ProviderSpec.from_config(
            name='my_yokan_provider', provider_id=1,
            config=config)
        #print(spec.to_json(indent=4))

    def test_proc_with_providers(self):
        from .config_space import ConfigurationSpace, Integer

        max_num_pools = 3

        """
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
                    #max_num_pools=max_num_pools,
                    tags=['tag1', 'tag2'],
                    provider_config_space=provider_config_cs,
                    provider_config_resolver=resolve_provider_config,
                    dependency_resolver=resolve_provider_dependencies),
                "count": (1,3)
            }
        ]
        """

        class MyProviderSpaceBuilder(ProviderConfigSpaceBuilder):

            def set_provider_hyperparameters(self, configuration_space: CS) -> None:
                configuration_space.add(Integer("x", (0,9)))
                configuration_space.add(Integer("y", (1,42)))

            def resolve_to_provider_spec(
                    self, name: str, provider_id: int, config: Config, prefix: str) -> ProviderSpec:
                cfg = {
                    "x" : int(config[prefix + "x"]),
                    "y" : int(config[prefix + "y"])
                }
                return ProviderSpec(
                    name=name, type="yokan", provider_id=provider_id,
                    tags=["tag1", "tag2"], config=cfg, dependencies={})

        provider_space_factories = [
            {
                "family": "databases",
                "builder": MyProviderSpaceBuilder(),
                "count": (1,3)
            }
        ]
        space = ProcSpec.space(num_pools=(1, max_num_pools), num_xstreams=(2, 5),
                               provider_space_factories=provider_space_factories).freeze()
        print(space)
        config = space.sample_configuration()
        print(config)
        spec = ProcSpec.from_config(address='na+sm', config=config)
        print(spec.to_json(indent=4))

    def test_service_config_space(self):

        proc_type_a = ProcSpec.space(num_pools=2, num_xstreams=3)
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
            ]).freeze()
        #print(space)

        config = space.sample_configuration()
        #print(config)

        spec = ServiceSpec.from_config(address='na+sm', config=config)
        #print(spec.to_json(indent=4))

if __name__ == '__main__':
    unittest.main()
