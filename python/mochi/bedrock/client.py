# (C) 2018 The University of Chicago
# See COPYRIGHT in top-level directory.


"""
.. module:: client
   :synopsis: This package provides access to the Bedrock C++ wrapper

.. moduleauthor:: Matthieu Dorier <mdorier@anl.gov>


"""


import pybedrock_client
import pymargo.core
import pymargo
import json
from .spec import ProcSpec, XstreamSpec, PoolSpec, AbtIOSpec, SSGSpec, ProviderSpec, ClientSpec


ClientException = pybedrock_client.Exception


class ServiceHandle:

    def __init__(self, internal, client):
        self._internal = internal
        self._client = client

    @property
    def client(self):
        return self._client

    @property
    def config(self):
        return json.loads(self._internal.get_config())

    @property
    def spec(self):
        return ProcSpec.from_dict(self.config)

    def query(self, script: str):
        return json.loads(self._internal.query_config(script))

    def load_module(self, name: str, path: str):
        self._internal.load_module(name, path)

    def _ensure_config_str(self, config):
        if isinstance(config, str):
            return config
        elif isinstance(config, dict):
            return json.dumps(config)
        else:
            return config.to_json()

    def add_pool(self, config: str|dict|PoolSpec):
        config = self._ensure_config_str(config)
        self._internal.add_pool(config)

    def remove_pool(self, name: str):
        self._internal.remove_pool(name)

    def add_xstream(self, config: str|dict|XstreamSpec):
        config = self._ensure_config_str(config)
        self._internal.add_xstream(config)

    def remove_xstream(self, name: str):
        self._internal.remove_xstream(name)

    def add_ssg_group(self, config: str|dict|SSGSpec):
        config = self._ensure_config_str(config)
        self._internal.add_ssg_group(config)

    def add_abtio_instance(self, config: str|dict|AbtIOSpec):
        if isinstance(config, str):
            config = json.loads(config)
        elif isinstance(config, AbtIOSpec):
            config = config.to_dict()
        if "config" not in config:
            config["config"] = {}
        self._internal.add_abtio_instance(
            name=config["name"], pool=config["pool"],
            config=json.dumps(config["config"]))

    def add_client(self, config: str|dict|ClientSpec):
        if isinstance(config, str):
            config = json.loads(config)
        elif isinstance(config, ClientSpec):
            config = config.to_dict()
        if "config" not in config:
            config["config"] = {}
        if "dependencies" not in config:
            config["dependencies"] = {}
        if "tags" not in config:
            config["tags"] = []
        self._internal.add_client(
            name=config["name"],
            type=config["type"],
            config=json.dumps(config["config"]),
            dependencies=config["dependencies"],
            tags=config["tags"])

    def start_provider(self, config: str|dict|ProviderSpec):
        if isinstance(config, str):
            config = json.loads(config)
        elif isinstance(config, ProviderSpec):
            config = config.to_dict()
        if "config" not in config:
            config["config"] = {}
        if "dependencies" not in config:
            config["dependencies"] = {}
        if "tags" not in config:
            config["tags"] = []
        if "pool" not in config:
            config["pool"] = "__primary__"
        return self._internal.start_provider(
            name=config["name"],
            type=config["type"],
            provider_id=config["provider_id"],
            pool=config["pool"],
            config=json.dumps(config["config"]),
            dependencies=config["dependencies"],
            tags=config["tags"])

    def change_provider_pool(self, provider_name: str, pool_name: str):
        self._internal.change_provider_pool(
            provider_name, pool_name)


class ServiceGroupHandle:

    def __init__(self, internal, client):
        self._internal = internal
        self._client = client

    @property
    def client(self):
        return self._client

    def refresh(self):
        self._internal.refresh()

    @property
    def size(self):
        return self._internal.size

    def __len__(self):
        return self.size

    def __getitem__(self, index: int):
        return ServiceHandle(self._internal[index], self.client)

    @property
    def config(self):
        return json.loads(self._internal.get_config())

    @property
    def spec(self):
        config = self.config
        spec = {}
        for k, v in config.items():
            spec[k] = ProcSpec.from_dict(v)
        return spec

    def query(self, script: str):
        return json.loads(self._internal.query_config(script))


class Client:

    def __init__(self, arg):
        if isinstance(arg, pymargo.core.Engine):
            self._engine = arg
            self._owns_engine = False
        elif isinstance(arg, str):
            self._engine = pymargo.core.Engine(arg, pymargo.client)
            self._owns_engine = True
        else:
            raise TypeError(f'Invalid argument type {type(arg)}')
        self._internal = pybedrock_client.Client(self._engine.get_internal_mid())

    def __del__(self):
        if self._owns_engine:
            self._engine.finalize()
            del self._engine

    @property
    def mid(self):
        return self._internal.margo_instance_id

    @property
    def engine(self):
        return self._engine

    def make_service_handle(self, address: str|pymargo.core.Address, provider_id: int = 0):
        if isinstance(address, pymargo.core.Address):
            address = str(address)
        return ServiceHandle(
            self._internal.make_service_handle(address=address, provider_id=provider_id),
            self)

    def make_service_group_handle(self, group: str|int|list[str], provider_id: int = 0):
        return ServiceGroupHandle(
            self._internal.make_service_group_handle(group, provider_id),
            self)
