# (C) 2018 The University of Chicago
# See COPYRIGHT in top-level directory.


"""
.. module:: server
   :synopsis: This package provides access to the Bedrock C++ wrapper

.. moduleauthor:: Matthieu Dorier <mdorier@anl.gov>


"""


import pybedrock_server
import pymargo.core
import pymargo
from typing import Mapping, List
from .spec import ProcSpec, MargoSpec, PoolSpec, XstreamSpec, AbtIOSpec, ProviderSpec, ClientSpec
import json


BedrockException = pybedrock_server.Exception


class ConfigType:
    JSON = pybedrock_server.Server.JSON
    JX9 = pybedrock_server.Server.JX9


class NamedDependency:

    def __init__(self, internal):
        self._internal = internal

    @property
    def name(self):
        return self._internal.name

    @property
    def type(self):
        return self._internal.type

    @property
    def handle(self):
        return self._internal.handle


Pool = NamedDependency
Xstream = NamedDependency
AbtIOInstance = NamedDependency
Client = NamedDependency


class ProviderDependency(NamedDependency):

    def __init__(self, manager, internal: pybedrock_server.ProviderDependency):
        super().__init__(internal)
        self._manager = manager

    @property
    def provider_id(self) -> int:
        return self._internal.provider_id

    def change_pool(self, pool: str|Pool) -> None:
        if isinstance(pool, Pool):
            self._manager.change_pool(self.name, pool)


Provider = ProviderDependency


class PoolList:

    def __init__(self, internal: pybedrock_server.MargoManager):
        self._internal = internal

    def __len__(self) -> int:
        return self._internal.num_pools

    def __getitem__(self, key: int|str) -> Pool:
        return Pool(self._internal.get_pool(key))

    def __delitem__(self, key: int|str) -> None:
        self._internal.remove_pool(key)

    def __contains__(self, key: str) -> bool:
        try:
            self.__getitem__(key)
            return True
        except BedrockException:
            return False

    def create(self, config: str|dict|PoolSpec) -> Pool:
        if isinstance(config, dict):
            config = json.dumps(config)
        elif isinstance(config, PoolSpec):
            config = config.to_json()
        return Pool(self._internal.add_pool(config))


class XstreamList:

    def __init__(self, internal: pybedrock_server.MargoManager):
        self._internal = internal

    def __len__(self) -> int:
        return self._internal.num_xstreams

    def __getitem__(self, key: int|str) -> Xstream:
        return Xstream(self._internal.get_xstream(key))

    def __delitem__(self, key: int|str) -> None:
        self._internal.remove_xstream(key)

    def __contains__(self, key: str) -> bool:
        try:
            self.__getitem__(key)
            return True
        except BedrockException:
            return False

    def create(self, config: str|dict|XstreamSpec) -> Xstream:
        if isinstance(config, dict):
            config = json.dumps(config)
        elif isinstance(config, XstreamSpec):
            config = config.to_json()
        return Xstream(self._internal.add_xstream(config))


class MargoManager:

    def __init__(self, internal: pybedrock_server.MargoManager, server: 'Server'):
        self._internal = internal
        self._server = server

    @property
    def mid(self):
        return self._internal.margo_instance_id

    @property
    def engine(self):
        return pymargo.core.Engine.from_margo_instance_id(self.mid)

    @property
    def config(self):
        return json.loads(self._internal.config)

    @property
    def spec(self):
        return MargoSpec.from_dict(self.config)

    @property
    def pools(self):
        return PoolList(self._internal)

    @property
    def xstreams(self):
        return XstreamList(self._internal)

    @property
    def default_handler_pool(self):
        return Pool(self._internal.default_handler_pool)


class AbtIOManager:

    def __init__(self, internal: pybedrock_server.ABTioManager, server: 'Server'):
        self._internal = internal
        self._server = server

    @property
    def config(self):
        return json.loads(self._internal.config)

    @property
    def spec(self) -> list[AbtIOSpec]:
        abt_spec = self._server.spec.margo.argobots
        return [AbtIOSpec.from_dict(instance, abt_spec=abt_spec) for instance in self.config]

    def __len__(self) -> int:
        return self._internal.num_abtio_instances

    def __getitem__(self, key: int|str) -> AbtIOInstance:
        return AbtIOInstance(self._internal.get_abtio_instance(key))

    def __contains__(self, key: str) -> bool:
        try:
            self.__getitem__(key)
            return True
        except BedrockException:
            return False

    def create(self, name: str, pool: str|int|Pool, config: str|dict = {}) -> AbtIOInstance:
        if isinstance(config, str):
            config = json.loads(config)
        if not isinstance(pool, Pool):
            pool = self._server.margo.pools[pool]
        pool = pool._internal
        return AbtIOInstance(self._internal.add_abtio_instance(name, pool, config))


class ClientManager:

    def __init__(self, internal: pybedrock_server.ClientManager, server: 'Server'):
        self._internal = internal
        self._server = server

    @property
    def config(self) -> dict:
        return self._internal.config

    @property
    def spec(self) -> list[ClientSpec]:
        return [ClientSpec.from_dict(client) for client in self.config]

    def __len__(self):
        return self._internal.num_clients

    def __getitem__(self, key: int|str) -> Client:
        return self._internal.get_client(key)

    def __delitem__(self, key: int|str) -> None:
        self._internal.remove_client(key)

    def __contains__(self, key: str) -> bool:
        try:
            self.__getitem__(key)
            return True
        except BedrockException:
            return False

    def get_or_create_anonymous(self, type: str) -> Client:
        return Client(self._internal.get_client_or_create(type))

    def create(self, name: str, type: str, config: str|dict = "{}",
               dependencies: dict[str,str|list[str]] = {},
               tags: list[str] = []) -> Client:
        if isinstance(config, str):
            config = json.loads(config)
        info = {
            "name": name,
            "type": type,
            "dependencies": dependencies,
            "tags": tags,
            "config": config
        }
        return Client(self._internal.add_client(info))


class ProviderManager:

    def __init__(self, internal: pybedrock_server.ProviderManager, server: 'Server'):
        self._internal = internal
        self._server = server

    @property
    def config(self) -> dict:
        return self._internal.config

    @property
    def spec(self) -> list[ProviderSpec]:
        abt_spec = self._server.margo.spec.argobots
        return [ProviderSpec.from_dict(provider, abt_spec) for provider in self.config]

    def __len__(self):
        return self._internal.num_providers

    def __getitem__(self, key: int|str) -> Provider:
        return Provider(self, self._internal.get_provider(key))

    def __delitem__(self, key: str) -> None:
        self._internal.deregister_provider(key)

    def __contains__(self, key: str) -> bool:
        try:
            self.__getitem__(key)
            return True
        except BedrockException:
            return False

    def lookup(self, locator: str):
        return Provider(self, self._internal.lookup_provider(locator))

    def create(self, name: str, type: str, provider_id: int = 65535,
               pool: str|int|Pool = "__primary__",
               config: str|dict = {}, dependencies: dict[str,str|list[str]] = {},
               tags: list[str] = []) -> Client:
        if isinstance(pool, Pool):
            pool = pool.name
        if isinstance(config, str):
            config = json.loads(config)
        info = {
            "name": name,
            "type": type,
            "provider_id": provider_id,
            "dependencies": dependencies,
            "tags": tags,
            "config": config
        }
        return Provider(self, self._internal.add_provider(info))

    def migrate(self, provider: str, dest_addr: str,
                dest_provider_id: str, migration_config: str|dict = "{}",
                remove_source: bool = True):
        if isinstance(migration_config, dict):
            migration_config = json.dumps(migration_config)
        self._internal.migrate_provider(
                provider, dest_addr, dest_provider_id,
                migration_config, remove_source)

    def snapshot(self, provider: str, dest_path: str,
                 snapshot_config: str|dict = "{}",
                 remove_source: bool = True):
        if isinstance(snapshot_config, dict):
            snapshot_config = json.dumps(snapshot_config)
        self._internal.snapshot_provider(
                provider, dest_path,
                snapshot_config, remove_source)

    def restore(self, provider: str, src_path: str,
                restore_config: str|dict = "{}"):
        if isinstance(restore_config, dict):
            restore_config = json.dumps(restore_config)
        self._internal.restore_config(
                provider, src_path, restore_config)


class Server:

    def __init__(self, address: str,
                 config: str|dict|ProcSpec = "{}",
                 config_type = ConfigType.JSON,
                 jx9_params: dict[str,str] = {}):
        if isinstance(config, dict):
            config = json.dumps(config)
        elif isinstance(config, ProcSpec):
            config = config.to_json()
        self._internal = pybedrock_server.Server(address, config, config_type, jx9_params)

    @staticmethod
    def from_spec(spec: ProcSpec):
        config = spec.to_json()
        return Server(spec.margo.mercury.address,
                      config, ConfigType.JSON)

    def finalize(self) -> None:
        self._internal.finalize()

    def wait_for_finalize(self) -> None:
        self._internal.wait_for_finalize()

    @property
    def config(self) -> dict:
        return json.loads(self._internal.config)

    @property
    def spec(self) -> ProcSpec:
        return ProcSpec.from_dict(self.config)

    @property
    def margo(self) -> MargoManager:
        return MargoManager(self._internal.margo_manager, self)

    @property
    def abtio(self) -> AbtIOManager:
        return AbtIOManager(self._internal.abtio_manager, self)

    @property
    def clients(self) -> ClientManager:
        return ClientManager(self._internal.client_manager, self)

    @property
    def providers(self) -> ProviderManager:
        return ProviderManager(self._internal.provider_manager, self)
