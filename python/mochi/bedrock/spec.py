# (C) 2018 The University of Chicago
# See COPYRIGHT in top-level directory.


"""
.. module:: spec
   :synopsis: This package helps configure a Bedrock deployment

.. moduleauthor:: Matthieu Dorier <mdorier@anl.gov>


"""


import json
import attr
from attr import Factory
from attr.validators import instance_of, in_
from typing import List, NoReturn, Union, Optional, Sequence, Any, Callable, Mapping


class _Configurable:
    """
    For some of the classes bellow, initializing from config simply means extracting
    the parameters from the input Configuration object that has the same name as an
    expected attribute of the class. Inheriting from _Configurable allows to do that.
    """

    @classmethod
    def from_config(cls, *, config: 'Configuration', prefix: str = '', **kwargs):
        expected_attr = set(attribute.name for attribute in cls.__attrs_attrs__)
        expected_kwargs = { k: v for k, v in kwargs.items() if k in expected_attr}
        for param, value in config.items():
            if not param.startswith(prefix):
                continue
            param = param[len(prefix):]
            if not param in expected_attr:
                continue
            if param in expected_kwargs:
                continue
            expected_kwargs[param] = value.item() if hasattr(value, 'item') else value
        return cls(**expected_kwargs)


def _check_validators(instance, attribute, value):
    """Generic hook function called by the attr package when setattr is called,
    to run the validators corresponding to the attribute being modified."""
    attribute.validator(instance, attribute, value)
    return value


def _validate_object_name(instance, attribute, value):
    import re
    if not re.match(r'[a-zA-Z_][a-zA-Z0-9_]*$', value):
        raise NameError(f'"name" field {value} is not a valid identifier')


@attr.s(auto_attribs=True, on_setattr=_check_validators, kw_only=True)
class SpecListDecorator:
    """The SpecListDecorator class is used in various places in this module
    to wrap a list object and provide an identical API, with some checks
    being done when the list is modified:
    - All the elements have the same type (provided in constructor)
    - All the elements have a unique name attribute
    - All the elements appear only once in the list
    Additionally, this class allows looking elements by their name attribute.

    :param list: List to decorate
    :type list: Type allowed in list
    """

    _list: list = attr.ib(validator=instance_of(list))
    _type: type = attr.ib(
        validator=instance_of(type),
        default=Factory(lambda self: type(self._list[0]), takes_self=True))

    def _find_with_name(self, name: str):
        """Find an element with the specified name in the list.

        :param name: Name to find
        :type name: str

        :return: The element found with the specified name, None if not found
        :rtype: None or _type
        """
        match = [e for e in self._list if e.name == name]
        if len(match) == 0:
            return None
        else:
            return match[0]

    def __len__(self):
        return len(self._list)

    def __getitem__(self, key):
        """Get an item using the [] operator. The key may be an integer or
        a string. If the key is a string, this function searches for an
        element with a name corresponding to the key.

        :param key: Key
        :type key: int or str or slice

        :raises KeyError: No element found with specified key
        :raises TypeError: invalid key type

        :return: The element corresponding to the key
        :rtype: _type
        """
        if isinstance(key, int) or isinstance(key, slice):
            return self._list[key]
        elif isinstance(key, str):
            match = self._find_with_name(key)
            if match is None:
                raise KeyError(f'No item found with name "{key}"')
            return match
        else:
            raise TypeError(f'Invalid key type {type(key)}')

    def __setitem__(self, key: int, spec) -> NoReturn:
        """Set an item using the [] operator. The key must be an integer.

        :param key: Index
        :type key: int

        :param spec: Element to set
        :type spec: _type

        :raises KeyError: Key out of range
        :raises TypeError: Invalid element type
        :raises NameError: Conflicting element name
        """
        if isinstance(key, slice):
            if not isinstance(spec, list):
                raise TypeError(f'Invalid type {type(spec)} ' +
                                '(expected list)')
            if len([x for x in spec if type(x) != self._type]):
                raise TypeError(
                    'List contains element(s) of incorrect type(s)')
            names = set([s.name for s in spec])
            if len(names) != len(spec):
                raise ValueError(
                    'List has duplicate elements or duplicate names')
            for name in names:
                if self._find_with_name(name) is not None:
                    raise NameError(
                        f'List already contains element with name {name}')
            self._list[key] = spec
        elif key < 0 or key >= len(self._list):
            KeyError('Invalid index')
        if not isinstance(spec, self._type):
            raise TypeError(f'Invalid type  {type(spec)} ' +
                            f'(expected {self._type})')
        match = self._find_with_name(spec.name)
        if match is not None:
            if self.index(match) != key:
                raise NameError(
                    'Conflicting object with the same name ' +
                    f'"{spec.name}"')
        self._list[key] = spec

    def __delitem__(self, key: Union[int, str]) -> NoReturn:
        """Delete an item using the [] operator. The key must be either an
        integer or a string. If the key is a string, it is considered the name
        of the element to delete.

        :param key: Key
        :type key: int or str or slice

        :rqises KeyError: No element found for this key
        :raises TypeError: Invalid key type
        """
        if isinstance(key, int) or isinstance(key, slice):
            self._list.__delitem__(key)
        elif isinstance(key, str):
            match = self._find_with_name(key)
            if match is None:
                raise KeyError('Invalid name {key}')
            index = self.index(match)
            self._list.__delitem__(index)
        else:
            raise TypeError('Invalid key type')

    def append(self, spec) -> NoReturn:
        """Append a new element at the end of the list.

        :param spec: Element to append
        :type spec: _type

        :raises TypeError: Invalid element type
        :raises NameError: Conflict with an object with the same name
        """
        if not isinstance(spec, self._type):
            raise TypeError(f'Invalid type  {type(spec)} ' +
                            f'(expected {self._type})')
        match = self._find_with_name(spec.name)
        if match is not None:
            raise NameError(
                'Conflicting object with the same name ' +
                f'"{spec.name}"')
        self._list.append(spec)

    def index(self, arg) -> int:
        """Get the index of an element in the list. The argument may be
        an element of the list, or a string name of the element.

        :param arg: Element or name of the element
        :type arg: _type or str

        :raises NameError: No element with the specified name found
        :raises ValueError: Element not found in the list

        :return: The index of the element if found
        :rtype: int
        """
        if isinstance(arg, str):
            match = self._find_with_name(arg.name)
            if match is None:
                raise NameError(f'Object named "{arg}" not in the list')
            else:
                return match
        else:
            return self._list.index(arg)

    def clear(self) -> NoReturn:
        """Clear the content of the list.
        """
        self._list.clear()

    def copy(self) -> list:
        """Copy the content of the list into a new list.

        :return: A copy of the list
        """
        return self._list.copy()

    def count(self, arg) -> int:
        """Count the number of occurences of an element in the list.
        The argment may be an element, or a string, in which case the
        element is looked up by name.

        Note that since the class enforces that an element does not
        appear more than once, the returned value can only be 0 or 1.

        :param arg: Element or name
        :type arg: _type or str

        :return: The number of occurence of an element in the list
        """
        if isinstance(arg, str):
            match = self._find_with_name(arg)
            if match is None:
                return 0
            else:
                return 1
        else:
            return self._list.count(arg)

    def __contains__(self, arg) -> bool:
        """Allows the "x in list" syntax."""
        return self.count(arg) > 0

    def extend(self, to_add) -> NoReturn:
        """Extend the list with new elements. The elements will either
        all be added, or none will be added.

        :param to_add: list of new elements to add.
        :type to_add: list

        :raises TypeError: List contains element(s) of incorrect type(s)
        :raises ValueError: List has duplicate elements or duplicate names
        :raises NameError: List already contains element with specified name
        """
        if len([x for x in to_add if type(x) != self._type]):
            raise TypeError('List contains element(s) of incorrect type(s)')
        names = set([spec.name for spec in to_add])
        if len(names) != len(to_add):
            raise ValueError('List has duplicate elements or duplicate names')
        for name in names:
            if self._find_with_name(name) is not None:
                raise NameError(
                    f'List already contains element with name {name}')
        self._list.extend(to_add)

    def insert(self, index: int, spec) -> NoReturn:
        """Insert an element at a given index.

        :param index: Index
        :type index: int

        :param spec: Element to insert
        :type spec: _type

        :raises TypeError: Element is of incorrect type
        :raises NameError: Element's name conflict with existing element
        """
        if not isinstance(spec, self._type):
            raise TypeError(f'Invalid type  {type(spec)} ' +
                            f'(expected {self._type})')
        match = self._find_with_name(spec.name)
        if match is not None:
            raise NameError(
                'Conflicting object with the same name ' +
                f'"{spec.name}"')
        self._list.insert(index, spec)

    def pop(self, index: int):
        """Pop the element at the provided index.

        :param index: Index
        :type index: int

        :return: The popped element
        :rtype: _type
        """
        return self._list.pop(index)

    def remove(self, spec) -> NoReturn:
        """Remove a particular element. The element may be passed by name.

        :param spec: Element to remove or name of the element
        :type spec: _type or str

        :raises ValueError: Element not in the list
        """
        if isinstance(spec, str):
            match = self._find_with_name(spec)
            if match is None:
                raise ValueError(f'{spec} not in list')
            self.remove(match)
        else:
            self._list.remove(spec)

    def __iter__(self):
        """Returns an iterator to iterate over the elements of the list.
        """
        return iter(self._list)

    def add(self, *kargs, **kwargs):
        """Create a new object using the provided arguments and add
        it to the list, then return it."""
        obj = self._type(*kargs, **kwargs)
        self.append(obj)
        return obj


@attr.s(auto_attribs=True, on_setattr=_check_validators, kw_only=True)
class MercurySpec(_Configurable):
    """Mercury specifications.

    :param address: Address or protocol (e.g. "na+sm")
    :type address: str

    :param listening: Whether the process is listening for RPCs
    :type listening: bool

    :param ip_subnet: IP subnetwork
    :type ip_subnet: str

    :param auth_key: Authentication key
    :type auth_key: str

    :param auto_sm: Automatically used shared-memory when possible
    :type auto_sm: bool

    :param max_contexts: Maximum number of contexts
    :type max_contexts: int

    :param na_no_block: Busy-spin instead of blocking
    :type na_no_block: bool

    :param na_no_retry: Do not retry operations
    :type na_no_retry: bool

    :param no_bulk_eager: Disable eager mode in bulk transfers
    :type no_bulk_eager: bool

    :param no_loopback: Disable RPC to self
    :type no_loopback: bool

    :param request_post_incr: Increment to the number of preposted requests
    :type request_post_incr: int

    :param request_post_init: Initial number of preposted requests
    :type request_post_init: int

    :param stats: Enable statistics
    :type stats: bool

    :param checksum_level: Checksum level
    :type checksum_level: str

    :param version: Version
    :type version: str
    """

    address: str = attr.ib(validator=instance_of(str))
    listening: bool = attr.ib(default=True, validator=instance_of(bool))
    ip_subnet: str = attr.ib(default='', validator=instance_of(str))
    auth_key: str = attr.ib(default='', validator=instance_of(str))
    auto_sm: bool = attr.ib(default=False, validator=instance_of(bool))
    max_contexts: int = attr.ib(default=1, validator=instance_of(int))
    na_no_block: bool = attr.ib(default=False, validator=instance_of(bool))
    na_no_retry: bool = attr.ib(default=False, validator=instance_of(bool))
    no_bulk_eager: bool = attr.ib(default=False, validator=instance_of(bool))
    no_loopback: bool = attr.ib(default=False, validator=instance_of(bool))
    request_post_incr: int = attr.ib(default=256, validator=instance_of(int))
    request_post_init: int = attr.ib(default=256, validator=instance_of(int))
    stats: bool = attr.ib(default=False, validator=instance_of(bool))
    version: str = attr.ib(default='unknown', validator=instance_of(str))
    checksum_level: str = attr.ib(default='none',
                                  validator=in_(['none', 'rpc_headers', 'rpc_payload']))
    input_eager_size: int = attr.ib(default=4080, validator=instance_of(int))
    output_eager_size: int = attr.ib(default=4080, validator=instance_of(int))
    na_addr_format: str = attr.ib(default='unspec',
                                  validator=in_(['unspec', 'ipv4', 'ipv6', 'native']))
    na_max_expected_size: int = attr.ib(default=0, validator=instance_of(int))
    na_max_unexpected_size: int = attr.ib(default=0, validator=instance_of(int))
    na_request_mem_device: bool = attr.ib(default=False, validator=instance_of(bool))

    def to_dict(self) -> dict:
        """Convert the MercurySpec into a dictionary.
        """
        return attr.asdict(self)

    @staticmethod
    def from_dict(data: dict) -> 'MercurySpec':
        """Construct a MercurySpec from a dictionary.
        """
        return MercurySpec(**data)

    def to_json(self, *args, **kwargs) -> str:
        """Convert the MercurySpec into a JSON string.
        """
        return json.dumps(self.to_dict(), *args, **kwargs)

    @staticmethod
    def from_json(json_string: str) -> 'MercurySpec':
        """Construct a MercurySpec from a JSON string.
        """
        data = json.loads(json_string)
        return MercurySpec.from_dict(data)

    def validate(self) -> NoReturn:
        """Validate the state of the MercurySpec, raising an exception
        if the MercurySpec is not valid.
        """
        attr.validate(self)

    @property
    def protocol(self) -> str:
        return self.address.split(':')[0]

    @staticmethod
    def space(*,
              auto_sm: bool|list[bool] = False,
              na_no_block: bool|list[bool] = False,
              no_bulk_eager: bool|list[bool] = False,
              request_post_init: int|tuple[int,int] = 256,
              request_post_incr: int|tuple[int,int] = 256,
              input_eager_size: int|tuple[int,int] = 4080,
              output_eager_size: int|tuple[int,int] = 4080,
              na_max_expected_size: int|tuple[int,int] = 0,
              na_max_unexpected_size: int|tuple[int,int] = 0,
              **kwargs):
        """
        Create a ConfigurationSpace object to generate MercurySpecs.
        Each of the argument can be set to either a single value or a range
        to make them configurable.
        """
        from .config_space import ConfigurationSpace, CategoricalOrConst, IntegerOrConst
        cs = ConfigurationSpace()
        cs.add(CategoricalOrConst('auto_sm', auto_sm, default=True))
        cs.add(CategoricalOrConst('na_no_block', na_no_block, default=False))
        cs.add(CategoricalOrConst('no_bulk_eager', no_bulk_eager, default=False))
        cs.add(IntegerOrConst('request_post_init', request_post_init, default=256))
        cs.add(IntegerOrConst('request_post_incr', request_post_incr, default=256))
        cs.add(IntegerOrConst('input_eager_size', input_eager_size, default=4080))
        cs.add(IntegerOrConst('output_eager_size', output_eager_size, default=4080))
        cs.add(IntegerOrConst('na_max_expected_size', na_max_expected_size, default=0))
        cs.add(IntegerOrConst('na_max_unexpected_size', na_max_unexpected_size, default=0))
        return cs


@attr.s(auto_attribs=True,
        on_setattr=_check_validators,
        kw_only=True,
        hash=False,
        eq=False)
class PoolSpec(_Configurable):
    """Argobots pool specification.

    :param name: Name of the pool
    :type name: str

    :param kind: Kind of pool (fifo or fifo_wait)
    :type kind: str

    :param access: Access type of the pool
    :type access: str
    """

    name: str = attr.ib(
        validator=[instance_of(str), _validate_object_name],
        on_setattr=attr.setters.frozen)
    kind: str = attr.ib(
        default='fifo_wait',
        validator=in_(['fifo', 'fifo_wait', 'prio_wait', 'earliest_first']))
    access: str = attr.ib(
        default='mpmc',
        validator=in_(['private', 'spsc', 'mpsc', 'spmc', 'mpmc']))

    def to_dict(self) -> dict:
        """Convert the PoolSpec into a dictionary.
        """
        return attr.asdict(self)

    @staticmethod
    def from_dict(data) -> 'PoolSpec':
        """Construct a PoolSpec from a dictionary.
        """
        return PoolSpec(**data)

    def to_json(self, *args, **kwargs) -> str:
        """Convert the PoolSpec into a JSON string.
        """
        return json.dumps(self.to_dict(), *args, **kwargs)

    @staticmethod
    def from_json(json_string: str) -> 'PoolSpec':
        """Construct a PoolSpec from a JSON string.
        """
        data = json.loads(json_string)
        return PoolSpec.from_dict(data)

    def __hash__(self) -> int:
        """Hash function.
        """
        return id(self)

    def __eq__(self, other) -> bool:
        """Equality check.
        """
        return id(self) == id(other)

    def __ne__(self, other) -> bool:
        """Inequality check.
        """
        return id(self) != id(other)

    def validate(self) -> NoReturn:
        """Validate the PoolSpec, raising an error if the PoolSpec
        is not valid.
        """
        attr.validate(self)

    @staticmethod
    def space(*,
              pool_kinds: str|list[str] = ['fifo_wait', 'fifo', 'prio_wait', 'earliest_first'],
              **kwargs):
        """
        Create a ConfigurationSpace to generate PoolSpec objects.
        pool_kinds can be specified as a string or a list of strings to force the pool kind
        to be picked from a set different from the default one.
        """
        from .config_space import ConfigurationSpace, CategoricalOrConst
        cs = ConfigurationSpace()
        default = pool_kinds[0] if isinstance(pool_kinds, list) else pool_kinds
        cs.add(CategoricalOrConst('kind', pool_kinds, default=default))
        return cs


@attr.s(auto_attribs=True, on_setattr=_check_validators, kw_only=True)
class SchedulerSpec(_Configurable):
    """Argobots scheduler specification.

    :param type: Type of scheduler
    :type type: str

    :param pools: List of pools this scheduler is linked to
    :type pools: list
    """

    type: str = attr.ib(
        default='basic_wait',
        validator=in_(['default', 'basic', 'prio', 'randws', 'basic_wait']))
    pools: List[PoolSpec] = attr.ib()

    @pools.validator
    def _check_pools(self, attribute, value):
        """Validator called by attr to validate that the scheduler has at
        least one pool and that all the elements in the pools list are
        PoolSpec instances. This function will raise an exception if the
        pools doesn't match these conditions.
        """
        if len(value) == 0:
            raise ValueError('Scheduler must have at least one pool')
        for pool in value:
            if not isinstance(pool, PoolSpec):
                raise TypeError(f'Invalid type {type(value)} in list of pools')

    def to_dict(self) -> dict:
        """Convert the SchedulerSpec into a dictionary.
        """
        return {'type': self.type, 'pools': [pool.name for pool in self.pools]}

    @staticmethod
    def from_dict(data: dict, abt_spec: 'ArgobotsSpec') -> 'SchedulerSpec':
        """Construct a SchedulerSpec from a dictionary.
        Since the pools are referenced by name or index in the dictionary,
        an ArgobotsSpec is required to resolve the references.

        :param data: Dictionary to convert
        :type data: dict

        :param abt_spec: ArgobotsSpec to lookup PoolSpecs
        :type abt_spec: ArgobotsSpec

        :return: A new SchedulerSpec
        :rtype: SchedulerSpec
        """
        pools = []
        for pool_ref in data['pools']:
            pools.append(abt_spec.pools[pool_ref])
        return SchedulerSpec(pools=pools, type=data["type"])

    def to_json(self, *args, **kwargs) -> str:
        """Convert the SchedulerSpec into a JSON string.
        """
        return json.dumps(self.to_dict(), *args, **kwargs)

    @staticmethod
    def from_json(json_string: str,
                  abt_spec: 'ArgobotsSpec') -> 'SchedulerSpec':
        """Construct a SchedulerSpec from a JSON string.
        Since the pools are referenced by name or index in the JSON string,
        an ArgobotsSpec is required to resolve the references.

        :param json_string: JSON string to convert
        :type json_string: str

        :param abt_spec: ArgobotsSpec to lookup PoolSpecs
        :type abt_spec: ArgobotsSpec

        :return: A new SchedulerSpec
        :rtype: SchedulerSpec
        """
        data = json.loads(json_string)
        return SchedulerSpec.from_dict(data, abt_spec)

    def validate(self) -> NoReturn:
        """Validate the SchedulerSpec, raising an exception if the
        state of the SchedulerSpec is not valid.
        """
        attr.validate(self)

    @staticmethod
    def space(*,
              scheduler_types: str|list[str] = ['basic_wait', 'default', 'basic', 'prio', 'randws'],
              **kwargs):
        """
        Create a ConfigurationSpace to generate SchedulerSpec objects.
        """
        from .config_space import ConfigurationSpace, CategoricalOrConst, FloatOrConst
        cs = ConfigurationSpace()
        default_scheduler_types = scheduler_types[0] if isinstance(scheduler_types, list) else scheduler_types
        cs.add(CategoricalOrConst('type', scheduler_types, default=default_scheduler_types))
        """
        for i in range(0, max_num_pools):
            if isinstance(pool_association_weights, list):
                weight = pool_association_weights[i]
            else:
                weight = pool_association_weights
            cs.add(FloatOrConst(f'pool_association_weight[{i}]', weight, default=-1.0))
        """
        return cs


@attr.s(auto_attribs=True,
        on_setattr=_check_validators,
        kw_only=True,
        hash=False,
        eq=False)
class XstreamSpec:
    """Argobots xstream specification.

    :param name: Name of the Xstream
    :type name: str

    :param scheduler: Scheduler
    :type scheduler: SchedulerSpec

    :param cpubind: Binding to a specific CPU
    :type cpubind: int

    :param affinity: Affinity with CPU ids
    :type affinity: list[int]
    """

    name: str = attr.ib(
        on_setattr=attr.setters.frozen,
        validator=[instance_of(str), _validate_object_name])
    scheduler: SchedulerSpec = attr.ib(
        validator=instance_of(SchedulerSpec),
        factory=SchedulerSpec)
    cpubind: int = attr.ib(default=-1, validator=instance_of(int))
    affinity: List[int] = attr.ib(
        factory=list, validator=instance_of(list))

    def to_dict(self) -> dict:
        """Convert the XstreamSpec into a dictionary.
        """
        filter = attr.filters.exclude(attr.fields(type(self)).scheduler)
        data = attr.asdict(self, filter=filter)
        data['scheduler'] = self.scheduler.to_dict()
        return data

    @staticmethod
    def from_dict(data: dict, abt_spec: 'ArgobotsSpec') -> 'XstreamSpec':
        """Construct an XstreamSpec from a dictionary. Since the underlying
        Scheduler needs an ArgobotsSpec to be initialized from a dictionary,
        an ArgobotsSpec argument needs to be provided.

        :param data: Dictionary
        :type data: dict

        :param abt_spec: ArgobotsSpec from which to look for pools
        :type abt_spec: ArgobotsSpec

        :return: A new XstreamSpec
        :rtype: XstreamSpec
        """
        scheduler_args = data['scheduler']
        scheduler = SchedulerSpec.from_dict(scheduler_args, abt_spec)
        args = data.copy()
        del args['scheduler']
        xstream = XstreamSpec(**args, scheduler=scheduler)
        return xstream

    def to_json(self, *args, **kwargs) -> str:
        """Convert the XstreamSpec into a JSON string.
        """
        return json.dumps(self.to_dict(), *args, **kwargs)

    @staticmethod
    def from_json(json_string: str, abt_spec: 'ArgobotsSpec') -> 'XstreamSpec':
        """Construct an XstreamSpec from a JSON string. Since the underlying
        Scheduler needs an ArgobotsSpec to be initialized from a dictionary,
        an ArgobotsSpec argument needs to be provided.

        :param json_string: JSON string
        :type json_string: str

        :param abt_spec: ArgobotsSpec from which to look for pools
        :type abt_spec: ArgobotsSpec

        :return: A new XstreamSpec
        :rtype: XstreamSpec
        """
        data = json.loads(json_string)
        return XstreamSpec.from_dict(data, abt_spec)

    def __hash__(self) -> int:
        """Hash function.
        """
        return id(self)

    def __eq__(self, other) -> bool:
        """Equality function.
        """
        return id(self) == id(other)

    def __ne__(self, other) -> bool:
        """Inequality function.
        """
        return id(self) != id(other)

    def validate(self) -> NoReturn:
        """Validate the state of an XstreamSpec, raising an exception
        if the state is not valid.
        """
        attr.validate(self)
        self.scheduler.validate()

    @staticmethod
    def space(*args, **kwargs):
        """
        Create a ConfigurationSpace object from which to generate XstreamSpecs.
        This function essentially forwards its arguments to the underlying
        SchedulerSpec.space ConfigurationSpace.
        """
        from .config_space import ConfigurationSpace
        cs = ConfigurationSpace()
        cs.add_configuration_space(
            prefix='scheduler', delimiter='.',
            configuration_space=SchedulerSpec.space(*args, **kwargs))
        return cs

    @staticmethod
    def from_config(*,
                    name: str,
                    pools: list[PoolSpec],
                    config: 'Configuration',
                    prefix: str = ''):
        """
        Create an XstreamSpec from a Configuration object.
        """
        sched_spec = SchedulerSpec.from_config(
            pools=pools, config=config, prefix=f'{prefix}scheduler.')
        return XstreamSpec(name=name, scheduler=sched_spec)


def _default_pools_list() -> List[PoolSpec]:
    """Build a default list of PoolSpec for the ArgobotsSpec class.

    :return: A list with a single __primary__ PoolSpec
    :rtype: list
    """
    return [PoolSpec(name='__primary__')]


def _default_xstreams_list(self) -> List[XstreamSpec]:
    """Build a default list of XstreamSpec for the ArgobotsSpec class.

    :param self: ArgobotsSpec under construction
    :type self: ArgobotsSpec

    :return: A list with a single __primary__ XstreamSpec
    :rtype: list
    """
    pool = None
    try:
        pool = self.pools['__primary__']
    except KeyError:
        pool = self.pools[0]
    scheduler = SchedulerSpec(pools=[pool])
    return [XstreamSpec(name='__primary__', scheduler=scheduler)]


@attr.s(auto_attribs=True, on_setattr=_check_validators, kw_only=True)
class ArgobotsSpec:
    """Argobots specification.

    :param abt_mem_max_num_stacks: Max number of pre-allocated stacks
    :type abt_mem_max_num_stacks: int

    :param abt_thread_stacksize: Default thread stack size
    :type abt_thread_stacksize: int

    :param version: Version of Argobots
    :type version: str

    :param pools: List of PoolSpecs to use
    :type pools: list

    :param xstreams: List of XstreamSpecs to use
    :type xstreams: list

    :param lazy_stack_alloc: Lazy allocation of stacks
    :type lazy_stack_alloc: boolean

    :param profiling_dir: Output directory for Argobots profiles
    :type profiling_dir: str
    """

    abt_mem_max_num_stacks: int = attr.ib(
        default=8, validator=instance_of(int))
    abt_thread_stacksize: int = attr.ib(
        default=2097152, validator=instance_of(int))
    version: str = attr.ib(default='unknown', validator=instance_of(str))
    _pools: List[PoolSpec] = attr.ib(
        default=Factory(_default_pools_list),
        validator=instance_of(list))
    _xstreams: List[XstreamSpec] = attr.ib(
        default=Factory(_default_xstreams_list, takes_self=True),
        validator=instance_of(list))
    lazy_stack_alloc: bool = attr.ib(
        default=False, validator=instance_of(bool))
    profiling_dir: str = attr.ib(
        default=".", validator=instance_of(str))

    @abt_mem_max_num_stacks.validator
    def _check_abt_mem_max_num_stacks(self, attribute, value) -> NoReturn:
        """Check that the value of abt_mem_max_num_stacks is >= 0.
        """
        if value < 0:
            raise ValueError('Invalid abt_mem_max_num_stacks (should be >= 0)')

    @abt_thread_stacksize.validator
    def _abt_thread_stacksize(self, attribute, value) -> NoReturn:
        """Check that the value of abt_thread_stacksize is > 0.
        """
        if value <= 0:
            raise ValueError('Invalid abt_thread_stacksize (should be > 0)')

    @_pools.validator
    def _check_pools(self, attribute, value) -> NoReturn:
        """Check the list of pools.
        """
        for pool in value:
            if not isinstance(pool, PoolSpec):
                raise TypeError(
                    'pools should contain only PoolSpec objects')
        names = set([p.name for p in value])
        if len(names) != len(value):
            raise ValueError('Duplicate name found in list of pools')

    @_xstreams.validator
    def _check_xstreams(self, attribute, value) -> NoReturn:
        """Check the list of xstreams.
        """
        for es in value:
            if not isinstance(es, XstreamSpec):
                raise TypeError(
                    'xstreams should contain only XstreamSpec objects')
        names = set([es.name for es in value])
        if len(names) != len(value):
            raise ValueError('Duplicate name found in list of xstreams')

    @property
    def xstreams(self) -> SpecListDecorator:
        """Return a decorated list enabling access to the list of XstreamSpecs.
        """
        return SpecListDecorator(list=self._xstreams, type=XstreamSpec)

    @property
    def pools(self) -> SpecListDecorator:
        """Return a decorated list enabling access to the list of PoolSpecs.
        """
        return SpecListDecorator(list=self._pools, type=PoolSpec)

    def add(self, thing: Union[PoolSpec, XstreamSpec]) -> NoReturn:
        """Add an existing PoolSpec or XstreamSpec that was created externally.

        :param thing: A PoolSpec or an XstreamSpec to add
        :type thing: PoolSpec or XstreamSpec

        :raises TypeError: Object is not of type PoolSpec or XstreamSpec
        """
        if isinstance(thing, PoolSpec):
            self.pools.append(thing)
        elif isinstance(thing, XstreamSpec):
            self.xstreams.append(thing)
        else:
            raise TypeError(f'Cannot add object of type {type(thing)}')

    def to_dict(self) -> dict:
        """Convert the ArgobotsSpec into a dictionary.
        """
        filter = attr.filters.exclude(attr.fields(type(self))._pools,
                                      attr.fields(type(self))._xstreams)
        data = attr.asdict(self, filter=filter)
        data['pools'] = [p.to_dict() for p in self.pools]
        data['xstreams'] = [x.to_dict() for x in self.xstreams]
        return data

    @staticmethod
    def from_dict(data: dict) -> 'ArgobotsSpec':
        """Construct an ArgobotsSpec from a dictionary.
        """
        args = data.copy()
        del args['pools']
        del args['xstreams']
        abt = ArgobotsSpec(**args)
        del abt.xstreams[0]
        del abt.pools[0]
        pool_list = data['pools']
        for pool_args in pool_list:
            abt.pools.add(**pool_args)
        xstream_list = data['xstreams']
        for xstream_args in xstream_list:
            abt.xstreams.append(XstreamSpec.from_dict(xstream_args, abt_spec=abt))
        return abt

    def to_json(self, *args, **kwargs) -> str:
        """Convert the ArgobotsSpec into a JSON string.
        """
        return json.dumps(self.to_dict(), *args, **kwargs)

    @staticmethod
    def from_json(json_string: str) -> 'ArgobotsSpec':
        """Construct an ArgobotsSpec from a JSON string.
        """
        return ArgobotsSpec.from_dict(json.loads(json_string))

    def validate(self) -> NoReturn:
        """Validate the ArgobotsSpec, raising an exception if
        the ArgobotsSpec's state is not valid.
        """
        attr.validate(self)
        if len(self._xstreams) == 0:
            raise ValueError('No XstreamSpec found')
        for es in self._xstreams:
            es.validate()
            if len(es.scheduler.pools) == 0:
                raise ValueError(
                    f'Scheduler in XstreamSpec "{es.name}" has no pool')
            for p in es.scheduler.pools:
                if p not in self._pools:
                    raise ValueError(
                        f'Pool "{p.name}" in XstreamSpec "{es.name}" was ' +
                        'not added to the ArgobotsSpec')
        for p in self._pools:
            p.validate()

    @staticmethod
    def space(*,
              num_pools : int|tuple[int,int] = 1,
              num_xstreams : int|tuple[int,int] = 1,
              allow_more_pools_than_xstreams: bool = False,
              pool_kinds : list[str] = ['fifo_wait', 'fifo', 'prio_wait', 'earliest_first'],
              scheduler_types: str|list[str]|None = ['basic_wait', 'default', 'basic', 'prio', 'randws'],
              mapping_weight_range: tuple[float,float] = (-1.0, 1.0),
              **kwargs):
        """
        Create the ConfigurationSpace of an ArgobotsSpec.

        - num_pools : number of pools or a range for the number of pools to generate.
        - num_xstreams : number of xstreams or a range for the number of xstreams to generate.
        - allow_more_pools_than_xstreams : whether to allow more pools than xstreams.
        - mapping_weight_range : range in which to draw the mapping between pools and xstreams.
        """
        from .config_space import (
                ConfigurationSpace,
                IntegerOrConst,
                GreaterThanCondition,
                ForbiddenGreaterThanRelation,
                Float)
        import itertools
        cs = ConfigurationSpace()
        min_num_pools = num_pools if isinstance(num_pools, int) else num_pools[0]
        max_num_pools = num_pools if isinstance(num_pools, int) else num_pools[1]
        min_num_xstreams = num_xstreams if isinstance(num_xstreams, int) else num_xstreams[0]
        max_num_xstreams = num_xstreams if isinstance(num_xstreams, int) else num_xstreams[1]
        if (not allow_more_pools_than_xstreams):
            if max_num_pools > max_num_xstreams:
                raise ValueError(
                    'The maximum number of pool cannot exceed the maximum number of xstreams,'
                    ' unless allow_more_pools_than_xstreams is explicitely set to True.')
        # number of pools
        hp_num_pools = IntegerOrConst("num_pools", num_pools)
        cs.add(hp_num_pools)
        # number of xstreams
        hp_num_xstreams = IntegerOrConst("num_xstreams", num_xstreams)
        cs.add(hp_num_xstreams)
        if not allow_more_pools_than_xstreams:
            cs.add(ForbiddenGreaterThanRelation(hp_num_pools, hp_num_xstreams))
        # add each xstream's sub-space
        for i in range(0, max_num_xstreams):
            xstream_cs = XstreamSpec.space(**kwargs)
            cs.add_configuration_space(
                prefix=f'xstreams[{i}]', delimiter='.',
                configuration_space=xstream_cs)
            if i >= min_num_xstreams and not isinstance(num_xstreams, int):
                for param in xstream_cs:
                    cs.add(GreaterThanCondition(cs[f'xstreams[{i}].{param}'], hp_num_xstreams, i))
        # add each pool's sub-space
        for i in range(0, max_num_pools):
            pool_cs = PoolSpec.space(pool_kinds=pool_kinds)
            cs.add_configuration_space(
                prefix=f'pools[{i}]', delimiter='.',
                configuration_space=pool_cs)
            if i >= min_num_pools and not isinstance(num_pools, int):
                for param in pool_cs:
                    cs.add(GreaterThanCondition(cs[f'pools[{i}].{param}'], hp_num_pools, i))
        # add the mapping between pools and xstreams
        for i, j in itertools.product(range(max_num_pools),
                                      range(max_num_xstreams)):
            hp_weight = Float(f'mapping[{i}][{j}]', mapping_weight_range)
            cs.add(hp_weight)
            if i >= min_num_pools and not isinstance(num_pools, int):
                cs.add(GreaterThanCondition(hp_weight, hp_num_pools, i))
            if j >= min_num_xstreams and not isinstance(num_xstreams, int):
                cs.add(GreaterThanCondition(hp_weight, hp_num_xstreams, j))
        return cs

    @staticmethod
    def from_config(*,
                    config: 'Configuration',
                    prefix: str = '',
                    pool_name_format: str = '__pool_{}__',
                    xstream_name_format: str = '__xstream_{}__',
                    use_progress_thread: bool = False,
                    **kwargs):
        """
        Create an ArgobotsSpec from a Configuration object.
        pool_name_format and xstream_name_format are used to format the names
        of pools and xstreams respectively.
        """
        import itertools
        num_xstreams = int(config[f'{prefix}num_xstreams'])
        num_pools = int(config[f'{prefix}num_pools'])
        # create pool specs
        pool_specs = []
        for i in range(0, num_pools):
            pool_name = '__primary__' if i == 0 else pool_name_format.format(i)
            pool_spec = PoolSpec.from_config(
                name=pool_name, access='mpmc',
                config=config, prefix=f'{prefix}pools[{i}].')
            pool_specs.append(pool_spec)
        # if there is only one xstream, that xstream will use all the pools
        if num_xstreams == 1:
            xstream_specs = [
                XstreamSpec.from_config(
                    name='__primary__', pools=pool_specs,
                    config=config, prefix=f'{prefix}xstreams[0].')
            ]
            return ArgobotsSpec(pools=pool_specs, xstreams=xstream_specs)
        # here we are sure there are at least 2 xstreams
        # extract the mapping weights and sort it
        mapping = [ (float(config[f'{prefix}mapping[{i}][{j}]']), i, j) \
                    for i, j in itertools.product(range(num_pools), range(num_xstreams)) ]
        mapping = sorted(mapping, reverse=True)
        # we will create the list of pools used by each xstream
        pools_for_xstream = [[] for i in range(num_xstreams)]
        # without loss of generality, we can make xstream[i] use pool[i]
        for i in range(min(num_pools, num_xstreams)):
            pools_for_xstream[i].append(i)
        progress_pool = 1 if use_progress_thread and num_pools > 1 else 0
        progress_xstream = 1 if use_progress_thread else 0
        # if we have more xstreams than pools, use the mapping
        if num_xstreams > num_pools:
            for j in range(num_pools, num_xstreams):
                # we prefer not to use the progress pool
                candidates = [m for m in mapping if m[2] == j and m[1] != progress_pool]
                i = candidates[0][1] if len(candidates) != 0 else progress_pool
                pools_for_xstream[j].append(i)
        else: # we have more pools than xstreams (or the same number)
            for i in range(num_xstreams, num_pools):
                # we know we have more than 1 xstream so at least one is't progress xstream
                candidates = [m for m in mapping if m[1] == i and m[2] != progress_xstream]
                j = candidates[0][2]
                pools_for_xstream[j].append(i)
        # here all the pools are associated with at least one xstream
        # and each xstream is associated with at least one pool, we can
        # now "sprinkle" more connections based on the weights
        for m in mapping:
            weight, i, j = m
            if weight <= 0:
                break
            if i == progress_pool or j == progress_xstream or i in pools_for_xstream[j]:
                continue
            pools_for_xstream[j].append(i)
        # finally, create the xstream specs
        xstream_specs = []
        for j in range(num_xstreams):
            xstream_name = '__primary__' if j == 0 else xstream_name_format.format(j)
            if j == 1 and use_progress_thread:
                xstream_name = '__progress__'
            pools = [pool_specs[i] for i in pools_for_xstream[j]]
            xstream_spec = XstreamSpec.from_config(
                name=xstream_name, pools=pools,
                config=config, prefix=f'{prefix}xstreams[{j}].')
            xstream_specs.append(xstream_spec)
        return ArgobotsSpec(pools=pool_specs, xstreams=xstream_specs)


def _mercury_from_args(arg) -> MercurySpec:
    """Convert an argument into a MercurySpec. The argument may be a string
    representing the address, or a dictionary of arguments, or an already
    constructed MercurySpec.
    """
    if isinstance(arg, MercurySpec):
        return arg
    elif isinstance(arg, dict):
        return MercurySpec(**arg)
    elif isinstance(arg, str):
        return MercurySpec(address=arg)
    else:
        raise TypeError(f'cannot convert type {type(arg)} into a MercurySpec')


@attr.s(auto_attribs=True, on_setattr=_check_validators, kw_only=True)
class MargoSpec:
    """Margo specification.

    :param mercury: Mercury specification
    :type mercury: MercurySpec

    :param argobots: Argobots specification
    :type argobots: ArgobotsSpec

    :param progress_timeout_ub_msec: Progress timeout
    :type progress_timeout_ub_msec: int

    :param enable_abt_profiling: Enable Argobots profiling
    :type enable_abt_profiling: bool

    :param handle_cache_size: Handle cache size
    :type handle_cache_size: int

    :param progress_pool: Progress pool
    :type progress_pool: PoolSpec

    :param rpc_pool: RPC pool
    :type rpc_pool: PoolSpec

    :param version: Version of Margo
    :type version: str
    """

    mercury: MercurySpec = attr.ib(
        converter=_mercury_from_args,
        validator=instance_of(MercurySpec))
    argobots: ArgobotsSpec = attr.ib(
        factory=ArgobotsSpec, validator=instance_of(ArgobotsSpec))
    progress_timeout_ub_msec: int = attr.ib(
        default=100, validator=instance_of(int))
    enable_abt_profiling: bool = attr.ib(
        default=False, validator=instance_of(bool))
    handle_cache_size: int = attr.ib(
        default=32, validator=instance_of(int))
    progress_pool: PoolSpec = attr.ib(
        default=Factory(lambda self: self.argobots.pools[0],
                        takes_self=True),
        validator=instance_of(PoolSpec))
    rpc_pool: PoolSpec = attr.ib(
        default=Factory(lambda self: self.argobots.pools[0],
                        takes_self=True),
        validator=instance_of(PoolSpec))
    version: str = attr.ib(default='unknown',
                           validator=instance_of(str))

    def to_dict(self) -> dict:
        """Convert a MargoSpec into a dictionary.
        """
        filter = attr.filters.exclude(attr.fields(type(self)).argobots,
                                      attr.fields(type(self)).mercury,
                                      attr.fields(type(self)).progress_pool,
                                      attr.fields(type(self)).rpc_pool)
        data = attr.asdict(self, filter=filter)
        data['argobots'] = self.argobots.to_dict()
        data['mercury'] = self.mercury.to_dict()
        if self.progress_pool is None:
            data['progress_pool'] = None
        else:
            data['progress_pool'] = self.progress_pool.name
        if self.rpc_pool is None:
            data['rpc_pool'] = None
        else:
            data['rpc_pool'] = self.rpc_pool.name
        return data

    @staticmethod
    def from_dict(data: dict) -> 'MargoSpec':
        """Construct a MargoSpec from a dictionary.
        """
        abt_args = data['argobots']
        hg_args = data['mercury']
        argobots = ArgobotsSpec.from_dict(abt_args)
        mercury = MercurySpec.from_dict(hg_args)
        rpc_pool = None
        progress_pool = None
        if data['rpc_pool'] is not None:
            rpc_pool = argobots.pools[data['rpc_pool']]
        if data['progress_pool'] is not None:
            progress_pool = argobots.pools[data['progress_pool']]
        args = data.copy()
        args['argobots'] = argobots
        args['mercury'] = mercury
        args['rpc_pool'] = rpc_pool
        args['progress_pool'] = progress_pool
        return MargoSpec(**args)

    def to_json(self, *args, **kwargs) -> str:
        """Convert the MargoSpec into a JSON string.
        """
        return json.dumps(self.to_dict(), *args, **kwargs)

    @staticmethod
    def from_json(json_string: str) -> 'MargoSpec':
        """Construct a MargoSpec from a JSON string.
        """
        return MargoSpec.from_dict(json.loads(json_string))

    def validate(self) -> NoReturn:
        """Validate the MargoSpec, raising an exception if
        the MargoSpec's state is not valid.
        """
        attr.validate(self)
        self.mercury.validate()
        self.argobots.validate()
        if self.progress_pool is None:
            raise ValueError('progress_pool not set in MargoSpec')
        if self.rpc_pool is None:
            raise ValueError('rpc_pool not set in MargoSpec')
        if self.progress_pool not in self.argobots._pools:
            raise ValueError(
                f'progress_pool "{self.progress_pool.name}" not found' +
                ' in ArgobotsSpec')
        if self.rpc_pool not in self.argobots._pools:
            raise ValueError(
                f'rpc_pool "{self.rpc_pool.name}" not found' +
                ' in ArgobotsSpec')

    @staticmethod
    def space(*,
              handle_cache_size: int|tuple[int,int] = 32,
              progress_timeout_ub_msec: int|tuple[int,int] = 100,
              use_progress_thread: bool|tuple[bool,bool] = [True, False],
              rpc_pool: int|tuple[int,int]|None = None,
              **kwargs):
        """
        Create the ConfigurationSpace for a MargoSpec.

        handle_cache_size, progress_timeout_ub_msec, num_pools, and num_xstreams
        have a default integer value that can be replaced by another value or by
        a range to become a parameter of the space.

        progress_pool and rpc_pool can be an int to force them to a specific value,
        a range to make them random within that range, or None for the range to be
        automatically inferred from the maximum number of pools.
        """
        from .config_space import (
                ConfigurationSpace,
                CategoricalOrConst,
                IntegerOrConst,
                ForbiddenAndConjunction,
                ForbiddenInClause,
                ForbiddenEqualsClause)
        cs = ConfigurationSpace()
        cs.add(CategoricalOrConst('use_progress_thread', use_progress_thread))
        cs.add(IntegerOrConst('handle_cache_size', handle_cache_size))
        cs.add(IntegerOrConst('progress_timeout_ub_msec', progress_timeout_ub_msec))
        argobots_cs = ArgobotsSpec.space(**kwargs)
        mercury_cs = MercurySpec.space(**kwargs)
        cs.add_configuration_space(
            prefix='argobots', delimiter='.',
            configuration_space=argobots_cs)
        cs.add_configuration_space(
            prefix='mercury', delimiter='.',
            configuration_space=mercury_cs)
        hp_num_pools = cs['argobots.num_pools']
        # Note: rpc_pool and progress_pool are categorical because AI tools should not
        # make the assumption that adding/removing 1 to the value will lead to a smaller
        # change than adding/removing a larger value.
        hp_rpc_pool = CategoricalOrConst('rpc_pool', list(range(hp_num_pools.upper)), default=0)
        cs.add(hp_rpc_pool)
        # add conditions on the possible values of rpc_pool and progress_pool
        for i in range(hp_num_pools.lower, hp_num_pools.upper):
            cs.add(ForbiddenAndConjunction(
                ForbiddenInClause(hp_rpc_pool, list(range(i, hp_num_pools.upper))),
                ForbiddenEqualsClause(hp_num_pools, i)))
        return cs

    @staticmethod
    def from_config(*, config: 'Configuration', prefix: str = '', **kwargs):
        """
        Create a MargoSpec from a Configuration object.

        Note that this function needs at least the address parameter as
        a mandatory parameter that will be passed down to Mercury.
        kwargs arguments will be propagated to the underlying MercurySpec and
        ArgobotsSpec.
        """
        use_progress_thread = bool(config[f'{prefix}use_progress_thread'])
        mercury_spec = MercurySpec.from_config(
            prefix=f'{prefix}mercury.', config=config, **kwargs)
        argobots_spec = ArgobotsSpec.from_config(
            prefix=f'{prefix}argobots.', config=config,
            use_progress_thread=use_progress_thread, **kwargs)
        extra = {}
        if 'handle_cache_size' not in kwargs:
            extra['handle_cache_size'] = int(config[f'{prefix}handle_cache_size'])
        else:
            extra['handle_cache_size'] = kwargs['handle_cache_size']
        if 'progress_timeout_ub_msec' not in kwargs:
            extra['progress_timeout_ub_msec'] = int(config[f'{prefix}progress_timeout_ub_msec'])
        else:
            extra['progress_timeout_ub_msec'] = kwargs[f'progress_timeout_ub_msec']
        use_progress_thread = bool(config[f'{prefix}use_progress_thread'])
        if len(argobots_spec.pools) == 1 or not use_progress_thread:
            progress_pool = argobots_spec.pools[0]
        else:
            progress_pool = argobots_spec.pools[1]
        rpc_pool = argobots_spec.pools[int(config[f'{prefix}rpc_pool'])]
        return MargoSpec(
            mercury=mercury_spec,
            argobots=argobots_spec,
            progress_pool=progress_pool,
            rpc_pool=rpc_pool,
            **extra)


@attr.s(auto_attribs=True, on_setattr=_check_validators, kw_only=True)
class AbtIOSpec:
    """ABT-IO instance specification.

    :param name: Name of the ABT-IO instance
    :type name: str

    :param pool: Pool associated with the instance
    :type pool: PoolSpec

    :param config: Configuration
    :type config: dict
    """

    name: str = attr.ib(
        validator=[instance_of(str), _validate_object_name],
        on_setattr=attr.setters.frozen)
    pool: PoolSpec = attr.ib(validator=instance_of(PoolSpec))
    config: dict = attr.ib(validator=instance_of(dict),
                           factory=dict)

    def to_dict(self) -> dict:
        """Convert the AbtIOSpec into a dictionary.
        """
        return {'name': self.name,
                'pool': self.pool.name,
                'config': self.config}

    @staticmethod
    def from_dict(data: dict, abt_spec: ArgobotsSpec) -> 'AbtIOSpec':
        """Construct an AbtIOSpec from a dictionary. Since the dictionary
        references the pool by name or index, an ArgobotsSpec is necessary
        to resolve the reference.

        :param data: Dictionary
        :type data: dict

        :param abt_spec: ArgobotsSpec in which to look for the PoolSpec
        :type abt_spec: ArgobotsSpec
        """
        name = data['name']
        config = data['config']
        pool = abt_spec.pools[data['pool']]
        abtio = AbtIOSpec(name=name, pool=pool, config=config)
        return abtio

    def to_json(self, *args, **kwargs) -> str:
        """Convert the AbtIOSpec into a JSON string.
        """
        return json.dumps(self.to_dict(), *args, **kwargs)

    @staticmethod
    def from_json(json_string: str,  abt_spec: ArgobotsSpec) -> 'AbtIOSpec':
        """Construct an AbtIOSpec from a JSON string. Since the JSON string
        references the pool by name or index, an ArgobotsSpec is necessary
        to resolve the reference.

        :param json_string: JSON string
        :type json_string: str

        :param abt_spec: ArgobotsSpec in which to look for the PoolSpec
        :type abt_spec: ArgobotsSpec
        """
        return AbtIOSpec.from_dict(json.loads(json_string), abt_spec)


@attr.s(auto_attribs=True, on_setattr=_check_validators, kw_only=True)
class MonaSpec:
    """MoNA instance specification.

    :param name: Name of the MoNA instance
    :type name: str

    :param pool: Pool associated with the instance
    :type pool: PoolSpec

    :param config: Configuration
    :type config: dict
    """

    name: str = attr.ib(
        validator=[instance_of(str), _validate_object_name],
        on_setattr=attr.setters.frozen)
    pool: PoolSpec = attr.ib(validator=instance_of(PoolSpec))
    config: dict = attr.ib(validator=instance_of(dict),
                           factory=dict)

    def to_dict(self) -> dict:
        """Convert the MonaSpec into a dictionary.
        """
        return {'name': self.name,
                'pool': self.pool.name,
                'config': self.config}

    @staticmethod
    def from_dict(data: dict, abt_spec: ArgobotsSpec) -> 'MonaSpec':
        """Construct a MonaSpec from a dictionary. Since the dictionary
        references the pool by name or index, an ArgobotsSpec is necessary
        to resolve the reference.

        :param data: Dictionary
        :type data: dict

        :param abt_spec: ArgobotsSpec in which to look for the PoolSpec
        :type abt_spec: ArgobotsSpec
        """
        name = data['name']
        if 'config' in data:
            config = data['config']
        else:
            config = {}
        pool = abt_spec.pools[data['pool']]
        mona = MonaSpec(name=name, pool=pool, config=config)
        return mona

    def to_json(self, *args, **kwargs) -> str:
        """Convert the MonaSpec into a JSON string.
        """
        return json.dumps(self.to_dict(), *args, **kwargs)

    @staticmethod
    def from_json(json_string: str,  abt_spec: ArgobotsSpec) -> 'MonaSpec':
        """Construct an MonaSpec from a JSON string. Since the JSON string
        references the pool by name or index, an ArgobotsSpec is necessary
        to resolve the reference.

        :param json_string: JSON string
        :type json_string: str

        :param abt_spec: ArgobotsSpec in which to look for the PoolSpec
        :type abt_spec: ArgobotsSpec
        """
        return MonaSpec.from_dict(json.loads(json_string), abt_spec)


@attr.s(auto_attribs=True, on_setattr=_check_validators, kw_only=True)
class ProviderSpec:
    """Provider specification.

    :param name: Name of the provider
    :type name: str

    :param type: Type of provider
    :type type: str

    :param pool: Pool associated with the group
    :type pool: PoolSpec

    :param provider_id: Provider id
    :type provider_id: int

    :param config: Configuration
    :type config: dict

    :param dependencies: Dependencies
    :type dependencies: dict

    :param tags: Tags
    :type tags: List[str]
    """

    name: str = attr.ib(
        validator=[instance_of(str), _validate_object_name],
        on_setattr=attr.setters.frozen)
    type: str = attr.ib(
        validator=instance_of(str),
        on_setattr=attr.setters.frozen)
    pool: PoolSpec = attr.ib(
        validator=instance_of(PoolSpec))
    provider_id: int = attr.ib(
        default=0,
        validator=instance_of(int))
    config: dict = attr.ib(
        validator=instance_of(dict),
        factory=dict)
    dependencies: dict = attr.ib(
        validator=instance_of(dict),
        factory=dict)
    tags: List[str] = attr.ib(
        validator=instance_of(List),
        factory=list)

    def to_dict(self) -> dict:
        """Convert the ProviderSpec into a dictionary.
        """
        return {'name': self.name,
                'type': self.type,
                'pool': self.pool.name,
                'provider_id': self.provider_id,
                'dependencies': self.dependencies,
                'config': self.config,
                'tags': self.tags}

    @staticmethod
    def from_dict(data: dict, abt_spec: ArgobotsSpec) -> 'ProviderSpec':
        """Construct a ProviderSpec from a dictionary. Since the dictionary
        references the pool by name or index, an ArgobotsSpec is necessary
        to resolve the reference.

        :param data: Dictionary
        :type data: dict

        :param abt_spec: ArgobotsSpec in which to look for the PoolSpec
        :type abt_spec: ArgobotsSpec
        """
        args = data.copy()
        args['pool'] = abt_spec.pools[data['pool']]
        provider = ProviderSpec(**args)
        return provider

    def to_json(self, *args, **kwargs) -> str:
        """Convert the ProviderSpec into a JSON string.
        """
        return json.dumps(self.to_dict(), *args, **kwargs)

    @staticmethod
    def from_json(json_string: str,  abt_spec: ArgobotsSpec) -> 'ProviderSpec':
        """Construct a ProviderSpec from a JSON string. Since the JSON string
        references the pool by name or index, an ArgobotsSpec is necessary
        to resolve the reference.

        :param json_string: JSON string
        :type json_string: str

        :param abt_spec: ArgobotsSpec in which to look for the PoolSpec
        :type abt_spec: ArgobotsSpec
        """
        return ProviderSpec.from_dict(json.loads(json_string), abt_spec)

    @staticmethod
    def space(*, type: str, max_num_pools: int, tags: list[str] = [],
              provider_config_space: Optional['ConfigurationSpace'] = None,
              provider_config_resolver: Callable[['Configuration', str], dict]|None = None,
              dependency_config_space: Optional['ConfigurationSpace'] = None,
              dependency_resolver: Callable[['Configuration', str], dict]|None = None):
        """
        Create a ConfigurationSpace for a ProviderSpec.

        - type: type of provider.
        - max_num_pools: maximum number of pools in the underlying process.
        - tags: list of tags the provider will use.
        - pool_association_weights: range in which to draw the weight of association between
          the provider and each pool. The provider will use the pool with the largest weight.
        - provider_config_space: a ConfigurationSpace for the "config" field of the provider.
        - provider_config_resolver: a function (or callable) taking a Configuration and a prefix
          and returning the provider's "config" field (dict) from the Configuration.
        - dependency_config_space: a ConfigurationSpace for the "dependencies" field of the provider.
        - dependency_resolver: a function (or callable) taking a Configuration and a prefix
          and returning the provider's "dependencies" field (dict) from the Configuration.
        """
        from .config_space import ConfigurationSpace, FloatOrConst, Categorical, Constant
        cs = ConfigurationSpace()
        cs.add(Constant('type', type))
        cs.add(Constant('tags', tags))
        if provider_config_space is not None:
            cs.add_configuration_space(
                prefix='config', delimiter='.',
                configuration_space=provider_config_space)
        cs.add(Constant('config_resolver', provider_config_resolver))
        if dependency_config_space is not None:
            cs.add_configuration_space(
                prefix='dependencies', delimiter='.',
                configuration_space=dependency_config_space)
        cs.add(Constant('dependency_resolver', dependency_resolver))
        cs.add(Categorical('pool', list(range(max_num_pools))))
        return cs

    @staticmethod
    def from_config(*, name: str, provider_id: int, pools: list[PoolSpec],
                    config: 'Configuration', prefix: str = ''):
        """
        Create a ProviderSpec from a given Configuration object.

        This function must be also given the name and provider Id to give the provider,
        as well as the list of pools of the underlying ProcSpec.
        """
        from .config_space import Configuration
        type = config[f'{prefix}type']
        tags = config[f'{prefix}tags']
        provider_config_resolver = config[f'{prefix}config_resolver']
        dependency_resolver = config[f'{prefix}dependency_resolver']
        pool = pools[int(config[f'{prefix}pool'])]
        if provider_config_resolver is None:
            provider_config = {}
        else:
            provider_config = provider_config_resolver(config, f'{prefix}config.')
        if dependency_resolver is None:
            provider_dependencies = {}
        else:
            provider_dependencies = dependency_resolver(config, f'{prefix}dependencies.')
        return ProviderSpec(
            name=name, type=type, provider_id=provider_id,
            pool=pool, config=provider_config, tags=tags,
            dependencies=provider_dependencies)



@attr.s(auto_attribs=True, on_setattr=_check_validators, kw_only=True)
class BedrockSpec:
    """Bedrock specification.

    :param pool: Pool in which to execute Bedrock-specific RPCs
    :type pool: PoolSpec

    :param provider_id: Provider id at which to register Bedrock RPCs
    :type provider_id: int
    """

    pool: PoolSpec = attr.ib(
        validator=instance_of(PoolSpec))
    provider_id: int = attr.ib(
        validator=instance_of(int),
        default=0)

    def to_dict(self) -> dict:
        """Convert the BedrockSpec into a dictionary.
        """
        return {'pool': self.pool.name,
                'provider_id': self.provider_id}

    @staticmethod
    def from_dict(data: dict, abt_spec: ArgobotsSpec) -> 'BedrockSpec':
        """Construct a BedrockSpec from a dictionary. Since the dictionary
        references the pool by name or index, an ArgobotsSpec is necessary
        to resolve the reference.

        :param data: Dictionary
        :type data: dict

        :param abt_spec: ArgobotsSpec in which to look for the PoolSpec
        :type abt_spec: ArgobotsSpec
        """
        args = data.copy()
        args['pool'] = abt_spec.pools[data['pool']]
        bedrock = BedrockSpec(**args)
        return bedrock

    def to_json(self, *args, **kwargs) -> str:
        """Convert the BedrockSpec into a JSON string.
        """
        return json.dumps(self.to_dict(), *args, **kwargs)

    @staticmethod
    def from_json(json_string: str,  abt_spec: ArgobotsSpec) -> 'BedrockSpec':
        """Construct an BedrockSpec from a JSON string. Since the JSON string
        references the pool by name or index, an ArgobotsSpec is necessary
        to resolve the reference.

        :param json_string: JSON string
        :type json_string: str

        :param abt_spec: ArgobotsSpec in which to look for the PoolSpec
        :type abt_spec: ArgobotsSpec
        """
        return BedrockSpec.from_dict(json.loads(json_string), abt_spec)


def _margo_from_args(arg) -> MargoSpec:
    """Construct a MargoSpec from a single argument. If the argument
    is a string it considers it as the Mercury address. If the argument
    if a dict, its content if forwarded to the MargoSpec constructor.
    """
    if isinstance(arg, MargoSpec):
        return arg
    elif isinstance(arg, dict):
        return MargoSpec(**arg)
    elif isinstance(arg, str):
        return MargoSpec(mercury=arg)
    else:
        raise TypeError(f'cannot convert type {type(arg)} into a MargoSpec')


@attr.s(auto_attribs=True, on_setattr=_check_validators, kw_only=True)
class ProcSpec:
    """Process specification.

    :param margo: Margo specification
    :type margo: MargoSpec

    :param libraries: Dictionary of libraries
    :type libraries: dict

    :param providers: List of ProviderSpec
    :type providers: list
    """

    margo: MargoSpec = attr.ib(
        validator=instance_of(MargoSpec),
        converter=_margo_from_args)
    libraries: dict = attr.ib(
        factory=dict,
        validator=instance_of(dict))
    _providers: list = attr.ib(
        factory=list,
        validator=instance_of(list))
    bedrock: BedrockSpec = attr.ib(
        default=Factory(lambda self: BedrockSpec(pool=self.margo.rpc_pool),
                        takes_self=True),
        validator=instance_of(BedrockSpec))

    @property
    def providers(self) -> SpecListDecorator:
        """Return a decorator to access the internal list of ProviderSpec
        and validate changes to this list.
        """
        return SpecListDecorator(list=self._providers, type=ProviderSpec)

    def to_dict(self) -> dict:
        """Convert the ProcSpec into a dictionary.
        """
        data = {'margo': self.margo.to_dict(),
                'libraries': self.libraries,
                'providers': [p.to_dict() for p in self._providers],
                'bedrock': self.bedrock.to_dict()}
        return data

    @staticmethod
    def from_dict(data: dict) -> 'ProcSpec':
        """Construct a ProcSpec from a dictionary.
        """
        margo = MargoSpec.from_dict(data['margo'])
        libraries = dict()
        providers = []
        bedrock = {}
        if 'libraries' in data:
            libraries = data['libraries']
        if 'providers' in data:
            for p in data['providers']:
                providers.append(ProviderSpec.from_dict(p,  margo.argobots))
        if 'bedrock' in data:
            bedrock = BedrockSpec.from_dict(data['bedrock'], margo.argobots)
        return ProcSpec(margo=margo,
                        libraries=libraries,
                        providers=providers,
                        bedrock=bedrock)

    def to_json(self, *args, **kwargs) -> str:
        """Convert the ProcSpec into a JSON string.
        """
        return json.dumps(self.to_dict(), *args, **kwargs)

    @staticmethod
    def from_json(json_string: str) -> 'ProcSpec':
        """Construct a ProcSpec from a JSON string.
        """
        return ProcSpec.from_dict(json.loads(json_string))

    def validate(self) -> NoReturn:
        """Validate the state of the ProcSpec.
        """
        attr.validate(self)
        for a in self._abt_io:
            p = a.pool
            if p not in self.margo.argobots.pools:
                raise ValueError(f'Pool "{p.name}" used by ABT-IO instance' +
                                 ' not found in margo.argobots.pools')
        for m in self._mona:
            p = m.pool
            if p not in self.margo.argobots.pools:
                raise ValueError(f'Pool "{p.name}" used by MoNA instance' +
                                 ' not found in margo.argobots.pools')
        for g in self._ssg:
            p = g.pool
            if p not in self.margo.argobots.pools:
                raise ValueError(f'Pool "{p.name}" used by SSG group' +
                                 ' not found in margo.argobots.pool')
        for k, v in self.libraries.items():
            if not isinstance(k, str):
                raise TypeError('Invalid key type found in libraries' +
                                ' (expected string)')
            if not isinstance(v, str):
                raise TypeError('Invalid value type found in libraries' +
                                ' (expected string')
        for p in self._providers:
            if p.type not in self._libraries:
                raise ValueError('Could not find module library for' +
                                 f'module type {p.name}')
        for c in self._clients:
            if c.type not in self._libraries:
                raise ValueError('Could not find module library for' +
                                 f'module type {p.name}')

    @staticmethod
    def space(*, provider_space_factories: list[dict] = [],
              **kwargs):
        """
        The provider_space_factories argument is a list of dictionaries with the following format.
        ```
        {
            "family": "<family-name>",
            "space" : ConfigurationSpace,
            "count" : int or tuple[int,int]
        }
        ```
        - "family" is a name to use in the configuration space to represent a family of providers
          using the same ConfigurationSpace;
        - "space" is a ConfigurationSpace generated by ProviderSpec.space();
        - "count" is either an int, or a pair (int, int) (if ommitted, will default to 1).
        """
        from .config_space import (
                ConfigurationSpace,
                GreaterThanCondition,
                ForbiddenInClause,
                ForbiddenAndConjunction,
                ForbiddenEqualsClause,
                Constant,
                IntegerOrConst)
        margo_space = MargoSpec.space(**kwargs)
        families = [f['family'] for f in provider_space_factories]
        if len(set(families)) != len(provider_space_factories):
            raise ValueError('Duplicate provider family in provider_space_factories')
        cs = ConfigurationSpace()
        cs.add_configuration_space(
            prefix='margo', delimiter='.',
            configuration_space=margo_space)
        hp_num_pools = cs['margo.argobots.num_pools']
        max_num_pools = hp_num_pools.upper
        min_num_pools = hp_num_pools.lower
        cs.add(Constant('providers.families', families))
        # for each family of providers...
        for provider_group in provider_space_factories:
            family = provider_group['family']
            provider_cs = provider_group['space']
            count = provider_group.get('count', 1)
            default_count = count if isinstance(count, int) else count[0]
            # number of providers
            hp_num_providers = IntegerOrConst(f'providers.{family}.num_providers',
                                              count, default=default_count)
            cs.add(hp_num_providers)
            # add each provider's sub-space
            for i in range(0, hp_num_providers.upper):
                space_prefix = f'providers.{family}[{i}]'
                cs.add_configuration_space(
                    prefix=space_prefix, delimiter='.',
                    configuration_space=provider_cs)
                # get the hyperparameter for the pool used by this provider
                hp_pool = cs[f'{space_prefix}.pool']
                # add a constraint on it (cannot have pool > num_pools)
                # Note: pool is a categorical hyperparameter so we need a
                # ForbiddenInClause for each subset of values it can take
                for j in range(min_num_pools, max_num_pools):
                    cs.add(ForbiddenAndConjunction(
                        ForbiddenEqualsClause(hp_num_pools, j),
                        ForbiddenInClause(hp_pool, list(range(j, max_num_pools)))))
                # add conditions on all the hyperparameters of the sub-space,
                # they exist only if i < num_providers.
                if i <= hp_num_providers.lower:
                    continue
                for param in provider_cs:
                    param_key = f'{space_prefix}.{param}'
                    cs.add(GreaterThanCondition(cs[param_key], hp_num_providers, i))
        return cs

    @staticmethod
    def from_config(*, config: 'Configuration', prefix: str = '', **kwargs):
        """
        Create a ProcSpec from the provided Configuration object.
        Extra parameters (**kwargs) will be propagated to the underlying
        MargoSpec.from_config().
        """
        margo_spec = MargoSpec.from_config(
            config=config, prefix=f'{prefix}margo.', **kwargs)
        pools = margo_spec.argobots.pools
        families = config[f'{prefix}providers.families']
        current_provider_id = 1
        provider_specs = []
        for family in families:
            num_providers = int(config[f'{prefix}providers.{family}.num_providers'])
            for i in range(num_providers):
                provider_specs.append(
                    ProviderSpec.from_config(
                        name=f'{family}_{i}', pools=margo_spec.argobots.pools,
                        provider_id=current_provider_id,
                        config=config, prefix=f'{prefix}providers.{family}[{i}].'))
                current_provider_id += 1
        return ProcSpec(margo=margo_spec, providers=provider_specs)


@attr.s(auto_attribs=True, on_setattr=_check_validators, kw_only=True)
class ServiceSpec:
    """Service specification.

    :param processes: List of ProcSpec
    :type processes: list
    """

    _processes: list = attr.ib(
        factory=list,
        validator=instance_of(list))

    @property
    def processes(self) -> SpecListDecorator:
        """Return a decorator to access the internal list of ProcSpec
        and validate changes to this list.
        """
        return SpecListDecorator(list=self._processes, type=ProcSpec)

    def to_dict(self) -> dict:
        """Convert the ServiceSpec into a dictionary.
        """
        data = {'processes': [p.to_dict() for p in self._processes]}
        return data

    @staticmethod
    def from_dict(data: dict) -> 'ProcSpec':
        """Construct a ServiceSpec from a dictionary.
        """
        processes = []
        if 'processes' in data:
            for p in data['processes']:
                processes.append(ProcSpec.from_dict(p))
        return ProcSpec(processes=processes)

    def to_json(self, *args, **kwargs) -> str:
        """Convert the ProcSpec into a JSON string.
        """
        return json.dumps(self.to_dict()['processes'], *args, **kwargs)

    @staticmethod
    def from_json(json_string: str) -> 'ServiceSpec':
        """Construct a ServiceSpec from a JSON string.
        """
        cfg = {'processes': json.loads(json_string)}
        return ServiceSpec.from_dict(cfg)

    def validate(self) -> NoReturn:
        """Validate the state of the ServiceSpec.
        """
        def check_provider_dependency(dep: str):
            if '@' in value and dep.split('@')[1].isnumeric():
                rank = int(dep.split('@')[1].isnumeric())
                if rank < 0 or rank >= len(self._processes):
                    raise ValueError(f'Dependency {dep} refers to a non-existing rank')

        attr.validate(self)
        for p in self._processes:
            p.validate()
            for provider in p.providers:
                for key, value in provider.dependencies.items():
                    if isinstance(value, str):
                        check_provider_dependency(value)
                    elif isinstance(value, list):
                        for dep in value:
                            check_provider_dependency(dep)

    @staticmethod
    def space(*, process_space_factories: list[dict] = [],
              **kwargs):
        """
        The process_space_factories argument is a list of dictionaries with the following format.
        ```
        {
            "family": "<family-name>",
            "space" : ConfigurationSpace,
            "count" : int or tuple[int,int]
        }
        ```
        - "family" is a name to use in the configuration space to represent a family of processes
          using the same ConfigurationSpace;
        - "space" is a ConfigurationSpace generated by ProcSpec.space();
        - "count" is either an int, or a pair (int, int) (if ommitted, will default to 1).
        """
        from .config_space import (
                ConfigurationSpace,
                IntegerOrConst,
                GreaterThanCondition,
                AndConjunction,
                Constant)
        families = [f['family'] for f in process_space_factories]
        if len(set(families)) != len(process_space_factories):
            raise ValueError('Duplicate provider family in provider_space_factories')
        cs = ConfigurationSpace()
        cs.add(Constant('processes.families', families))
        for process_group in process_space_factories:
            family = process_group['family']
            process_cs = process_group['space']
            count = process_group.get('count', 1)
            default_count = count if isinstance(count, int) else count[0]
            hp_num_processes = IntegerOrConst(f'processes.{family}.num_processes',
                                              count, default=default_count)
            cs.add(hp_num_processes)
            for i in range(0, hp_num_processes.upper):
                cs.add_configuration_space(
                    prefix=f'processes.{family}[{i}]', delimiter='.',
                    configuration_space=process_cs)
                if i <= hp_num_processes.lower:
                    continue
                for param in provider_cs:
                    param_key = f'processes.{family}[{i}].{param}'
                    cs.add(GreaterThanCondition(cs[param_key], hp_num_providers, i))
        return cs

    @staticmethod
    def from_config(config: 'Configuration',
                    prefix: str = '', **kwargs):
        """
        Create a ServiceSpec from the provided Configuration object.
        Extra parameters (**kwargs) will be propagated to the underlying
        ProcSpec.from_config().
        """
        families = config[f'{prefix}processes.families']
        proc_specs = []
        for family in families:
            if len(family) == 0:
                continue
            proc_count = int(config[f'{prefix}processes.{family}.num_processes'])
            for i in range(proc_count):
                proc_specs.append(
                    ProcSpec.from_config(
                        config=config,
                        prefix=f'{prefix}processes.{family}[{i}].',
                        **kwargs))
        return ServiceSpec(processes=proc_specs)


attr.resolve_types(MercurySpec, globals(), locals())
attr.resolve_types(PoolSpec, globals(), locals())
attr.resolve_types(SchedulerSpec, globals(), locals())
attr.resolve_types(XstreamSpec, globals(), locals())
attr.resolve_types(ArgobotsSpec, globals(), locals())
attr.resolve_types(MargoSpec, globals(), locals())
attr.resolve_types(ProviderSpec, globals(), locals())
attr.resolve_types(BedrockSpec, globals(), locals())
attr.resolve_types(ProcSpec, globals(), locals())
attr.resolve_types(ServiceSpec, globals(), locals())
