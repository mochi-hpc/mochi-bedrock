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


def _CategoricalOrConst(name: str, items: Sequence[Any]|Any, *,
                        default: Any|None = None, weights: Sequence[float]|None = None,
                        ordered: bool = False, meta: dict|None = None):
    """
    ConfigSpace helper function to create either a Categorical or a Constant hyperparameter.
    """
    from ConfigSpace import Categorical, Constant
    try:
        dflt = default
        if dflt is None:
            if isinstance(items, list):
                dflt = items[0]
            else:
                dflt = items
        return Categorical(name=name, items=items, default=dflt,
                           weights=weights, ordered=ordered, meta=meta)
    except TypeError:
        return Constant(name=name, value=items, meta=meta)


def _IntegerOrConst(name: str, bounds: int|tuple[int, int], *,
                    distribution: Any = None, default: int|None = None,
                    log: bool = False, meta: dict|None = None):
    """
    ConfigSpace helper function to create either an Integer or a Constant hyperparameter.
    """
    from ConfigSpace import Integer, Constant
    if isinstance(bounds, int):
        c = Constant(name=name, value=bounds, meta=meta)
        setattr(c, "upper", bounds)
        setattr(c, "lower", bounds)
        return c
    elif bounds[0] == bounds[1]:
        c = Constant(name=name, value=bounds[0], meta=meta)
        setattr(c, "upper", bounds[0])
        setattr(c, "lower", bounds[0])
        return c
    else:
        return Integer(name=name, bounds=bounds, distribution=distribution,
                       default=default, log=log, meta=meta)

def _FloatOrConst(name: str, bounds: float|tuple[float, float], *,
                  distribution: Any = None, default: float|None = None,
                  log: bool = False, meta: dict|None = None):
    """
    ConfigSpace helper function to create either a Float or a Constant hyperparameter.
    """
    from ConfigSpace import Float, Constant
    if isinstance(bounds, float):
        c = Constant(name=name, value=bounds, meta=meta)
        setattr(c, "upper", bounds)
        setattr(c, "lower", bounds)
        return c
    elif bounds[0] == bounds[1]:
        c = Constant(name=name, value=bounds[0], meta=meta)
        setattr(c, "upper", bounds[0])
        setattr(c, "lower", bounds[0])
        return c
    else:
        return Float(name=name, bounds=bounds, distribution=distribution,
                     default=default, log=log, meta=meta)


class _Configurable:

    @classmethod
    def from_config(cls, config: 'Configuration', prefix: str = '', **kwargs):
        expected = set(attribute.name for attribute in cls.__attrs_attrs__)
        for param, value in config.items():
            if not param.startswith(prefix):
                continue
            param = param[len(prefix):]
            if not param in expected:
                continue
            if param in kwargs:
                continue
            kwargs[param] = value.item() if hasattr(value, 'item') else value
        return cls(**kwargs)


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
    def space(auto_sm: bool|list[bool] = [True, False],
              na_no_block: bool|list[bool] = [True, False],
              no_bulk_eager: bool|list[bool] = [True, False],
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
        from ConfigSpace import ConfigurationSpace
        cs = ConfigurationSpace()
        cs.add(_CategoricalOrConst('auto_sm', auto_sm, default=True))
        cs.add(_CategoricalOrConst('na_no_block', na_no_block, default=False))
        cs.add(_CategoricalOrConst('no_bulk_eager', no_bulk_eager, default=False))
        cs.add(_IntegerOrConst('request_post_init', request_post_init, default=256))
        cs.add(_IntegerOrConst('request_post_incr', request_post_incr, default=256))
        cs.add(_IntegerOrConst('input_eager_size', input_eager_size, default=4080))
        cs.add(_IntegerOrConst('output_eager_size', output_eager_size, default=4080))
        cs.add(_IntegerOrConst('na_max_expected_size', na_max_expected_size, default=0))
        cs.add(_IntegerOrConst('na_max_unexpected_size', na_max_unexpected_size, default=0))
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
        validator=in_(['fifo', 'fifo_wait', 'prio', 'prio_wait', 'earliest_first']))
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
    def space(pool_kinds: str|list[str] = ['fifo_wait', 'fifo', 'prio', 'prio_wait', 'earliest_first']):
        """
        Create a ConfigurationSpace to generate PoolSpec objects.
        pool_kinds can be specified as a string or a list of strings to force the pool kind
        to be picked from a set different from the default one.
        """
        from ConfigSpace import ConfigurationSpace
        cs = ConfigurationSpace()
        default = pool_kinds[0] if isinstance(pool_kinds, list) else pool_kinds
        cs.add(_CategoricalOrConst('kind', pool_kinds, default=default))
        return cs


@attr.s(auto_attribs=True, on_setattr=_check_validators, kw_only=True)
class SchedulerSpec:
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
    def space(max_num_pools: int,
              scheduler_types: str|list[str] = ['basic_wait', 'default', 'basic', 'prio', 'randws'],
              pool_association_weights: tuple[float,float]|list[float|tuple[float,float]] = (-1.0, 1.0)):
        """
        Create a ConfigurationSpace to generate SchedulerSpec objects.

        This function requires to be provided with the maximum number of pools that a
        scheduler can be associated with.

        pool_association_weights is a tuple[float,float] or a list of N tuple[float,float]
        where N = max_num_pools, specifying how to draw the scheduler <-> pool associations.
        For each pool i, pool_association_weight[i] will be drawn randomly and specify the weight
        of the connection between this scheduler and pool i. This weight will be used to select
        which pools are effectively used by this scheduler (pools with a weight > 0) and in which
        order (in order of weights).
        """
        from ConfigSpace import ConfigurationSpace, Float
        cs = ConfigurationSpace()
        default_scheduler_types = scheduler_types[0] if isinstance(scheduler_types, list) else scheduler_types
        cs.add(_CategoricalOrConst('type', scheduler_types, default=default_scheduler_types))
        for i in range(0, max_num_pools):
            if isinstance(pool_association_weights, list):
                weight = pool_association_weights[i]
            else:
                weight = pool_association_weights
            cs.add(_FloatOrConst(f'pool_association_weight[{i}]', weight, default=-1.0))
        return cs

    @staticmethod
    def from_config(pools: list[PoolSpec],
                    config: 'Configuration', prefix: str = ''):
        """
        Create a SchedulerSpec from a Configuration object containing the scheduler's parameters.
        pools is the list of pools available to used by the scheduler.
        prefix is the prefix of the keys in the Configuration object.
        """
        type = config[f'{prefix}type']
        pool_weights = []
        for i in range(0, len(pools)):
            weight_name = f'{prefix}pool_association_weight[{i}]'
            weight = config[weight_name]
            pool_weights.append((weight, i))
        pool_weights = sorted(pool_weights)
        if pool_weights[-1][0] <= 0:
            pools = [pools[pool_weights[-1][1]]]
        else:
            pools = list(reversed([pools[i] for w, i in pool_weights if w > 0]))
        return SchedulerSpec(type=type, pools=pools)



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
        from ConfigSpace import ConfigurationSpace
        cs = ConfigurationSpace()
        cs.add_configuration_space(
            prefix='scheduler', delimiter='.',
            configuration_space=SchedulerSpec.space(*args, **kwargs))
        return cs

    @staticmethod
    def from_config(name: str, pools: list[PoolSpec],
                    config: 'Configuration',
                    prefix: str = ''):
        """
        Create an XstreamSpec from a Configuration object. See SchedulerSpec.from_config
        for more information.
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
    def space(num_pools : int|tuple[int,int] = 1,
              num_xstreams : int|tuple[int,int] = 1,
              pool_kinds : list[str] = ['fifo_wait', 'fifo', 'prio', 'prio_wait', 'earliest_first'],
              scheduler_types: str|list[str]|None = ['basic_wait', 'default', 'basic', 'prio', 'randws'],
              pool_association_weights: tuple[float,float]|list[float|tuple[float,float]] = (-1.0, 1.0),
              **kwargs):
        """
        Create the ConfigurationSpace of an ArgobotsSpec.

        - num_pools is the number of pools or a range for the number of pools to generate.
        - num_xstreams is the number of xstreams or a range for the number of xstreams to generate.
        - pool_kinds is the list of possible pool kinds to draw from.
        - scheduler_types is the list of possible scheduler types to draw from.
        - pool_association_weights : see SchedulerSpec.space.
        """
        from ConfigSpace import ConfigurationSpace, GreaterThanCondition, AndConjunction
        import itertools
        cs = ConfigurationSpace()
        min_num_pools = num_pools if isinstance(num_pools, int) else num_pools[0]
        max_num_pools = num_pools if isinstance(num_pools, int) else num_pools[1]
        hp_num_pools = _IntegerOrConst("num_pools", num_pools)
        min_num_xstreams = num_xstreams if isinstance(num_xstreams, int) else num_xstreams[0]
        max_num_xstreams = num_xstreams if isinstance(num_xstreams, int) else num_xstreams[1]
        hp_num_xstreams = _IntegerOrConst("num_xstreams", num_xstreams)
        cs = ConfigurationSpace()
        cs.add(hp_num_pools)
        cs.add(hp_num_xstreams)
        for i in range(0, max_num_xstreams):
            xstream_cs = XstreamSpec.space(
                max_num_pools=max_num_pools,
                scheduler_types=scheduler_types,
                pool_association_weights=pool_association_weights)
            cs.add_configuration_space(
                prefix=f'xstreams[{i}]', delimiter='.',
                configuration_space=xstream_cs)
            if i >= min_num_xstreams and not isinstance(num_xstreams, int):
                for param in xstream_cs:
                    if 'pool_association_weight' in  param:
                        continue
                    cs.add(GreaterThanCondition(cs[f'xstreams[{i}].{param}'], hp_num_xstreams, i))
        for i in range(0, max_num_pools):
            pool_cs = PoolSpec.space(pool_kinds=pool_kinds)
            cs.add_configuration_space(
                prefix=f'pools[{i}]', delimiter='.',
                configuration_space=pool_cs)
            if i >= min_num_pools and not isinstance(num_pools, int):
                for param in pool_cs:
                    cs.add(GreaterThanCondition(cs[f'pools[{i}].{param}'], hp_num_pools, i))
        for i, j in itertools.product(range(0, max_num_pools),
                                      range(0, max_num_xstreams)):
            hp_weight = cs[f'xstreams[{j}].scheduler.pool_association_weight[{i}]']
            conditions = []
            if i > min_num_pools-1 and not isinstance(num_pools, int):
                conditions.append(GreaterThanCondition(hp_weight, hp_num_pools, i))
            if j > min_num_xstreams-1 and not isinstance(num_xstreams, int):
                conditions.append(GreaterThanCondition(hp_weight, hp_num_xstreams, j))
            if len(conditions) == 1:
                cs.add(conditions[0])
            elif len(conditions) > 1:
                cs.add(AndConjunction(*conditions))
        return cs

    @staticmethod
    def from_config(config: 'Configuration',
                    prefix: str = '',
                    pool_name_prefix: str = 'pool_',
                    xstream_name_prefix: str = 'xstream_',
                    **kwargs):
        """
        Create an ArgobotsSpec from a Configuration object.
        pool_name_prefix and xstream_name_prefix are used to prefix the names
        of pools and xstreams respectively.
        """
        # create pool specs
        num_pools = config[f'{prefix}num_pools']
        pool_specs = []
        for i in range(0, num_pools):
            pool_name = f'{pool_name_prefix}{i}'
            pool_spec = PoolSpec.from_config(
                name=pool_name, access='mpmc',
                config=config, prefix=f'{prefix}pools[{i}].')
            pool_specs.append(pool_spec)
        # create xstream specs
        num_xstreams = config[f'{prefix}num_xstreams']
        xstream_specs = []
        used_pools = set()
        for i in range(0, num_xstreams):
            xstream_name = '__primary__' if i == 0 else f'{xstream_name_prefix}{i}'
            xstream_spec = XstreamSpec.from_config(
                name=xstream_name, pools=pool_specs,
                config=config, prefix=f'{prefix}xstreams[{i}].')
            xstream_specs.append(xstream_spec)
            for pool in xstream_spec.scheduler.pools:
                used_pools.add(pool)
        # deal with unused pools
        for i in range(0, num_pools):
            if pool_specs[i] in used_pools:
                continue
            weights = [
                (config[f'{prefix}xstreams[{j}].scheduler.pool_association_weight[{i}]'], j) \
                for j in range(0, num_xstreams)]
            weights = sorted(weights)
            xstream_specs[weights[-1][1]].scheduler.pools.append(pool_specs[i])
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
    def space(handle_cache_size: int|tuple[int,int] = 32,
              progress_timeout_ub_msec: int|tuple[int,int] = 100,
              progress_pool: int|tuple[int,int]|None = None,
              rpc_pool: int|tuple[int,int]|None = None,
              num_pools : int|tuple[int,int] = 1,
              num_xstreams : int|tuple[int,int] = 1,
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
        from ConfigSpace import (
                ConfigurationSpace,
                ForbiddenAndConjunction,
                ForbiddenInClause,
                ForbiddenEqualsClause)
        min_num_pools = num_pools if isinstance(num_pools, int) else num_pools[0]
        max_num_pools = num_pools if isinstance(num_pools, int) else num_pools[1]
        cs = ConfigurationSpace()
        cs.add(_IntegerOrConst('handle_cache_size', handle_cache_size))
        cs.add(_IntegerOrConst('progress_timeout_ub_msec', progress_timeout_ub_msec))
        argobots_cs = ArgobotsSpec.space(
            num_pools=num_pools, num_xstreams=num_xstreams, **kwargs)
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
        hp_rpc_pool = _CategoricalOrConst('rpc_pool', list(range(max_num_pools)), default=0)
        hp_progress_pool = _CategoricalOrConst('progress_pool', list(range(max_num_pools)), default=0)
        cs.add(hp_rpc_pool)
        cs.add(hp_progress_pool)
        for i in range(min_num_pools, max_num_pools):
            cs.add(ForbiddenAndConjunction(
                ForbiddenInClause(hp_rpc_pool, list(range(i, max_num_pools))),
                ForbiddenEqualsClause(hp_num_pools, i)))
            cs.add(ForbiddenAndConjunction(
                ForbiddenInClause(hp_progress_pool, list(range(i, max_num_pools))),
                ForbiddenEqualsClause(hp_num_pools, i)))
        return cs

    @staticmethod
    def from_config(config: 'Configuration', prefix: str = '', **kwargs):
        """
        Create a MargoSpec from a Configuration object.

        Note that this function needs at least the address parameter as
        a mandatory parameter that will be passed down to Mercury.
        kwargs arguments will be propagated to the underlying MercurySpec and
        ArgobotsSpec.
        """
        mercury_spec = MercurySpec.from_config(
            prefix=f'{prefix}mercury.', config=config, **kwargs)
        argobots_spec = ArgobotsSpec.from_config(
            prefix=f'{prefix}argobots.', config=config, **kwargs)
        extra = {}
        if 'handle_cache_size' not in kwargs:
            extra['handle_cache_size'] = int(config[f'{prefix}handle_cache_size'])
        else:
            extra['handle_cache_size'] = kwargs['handle_cache_size']
        if 'progress_timeout_ub_msec' not in kwargs:
            extra['progress_timeout_ub_msec'] = int(config[f'{prefix}progress_timeout_ub_msec'])
        else:
            extra['progress_timeout_ub_msec'] = kwargs[f'progress_timeout_ub_msec']
        progress_pool = argobots_spec.pools[int(config[f'{prefix}progress_pool'])]
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
class SwimSpec:
    """Swim specification for SSG.

    :param period_length_ms: Period length in milliseconds
    :type period_length_ms: int

    :param suspect_timeout_periods: Number of suspect timeout periods
    :type suspect_timeout_periods: int

    :param subgroup_member_count: Subgroup member count
    :type subgroup_member_count: int

    :param disabled: Disable Swim
    :type disabled: bool
    """

    period_length_ms: int = attr.ib(
        validator=instance_of(int),
        default=0)
    suspect_timeout_periods: int = attr.ib(
        validator=instance_of(int),
        default=-1)
    subgroup_member_count: int = attr.ib(
        validator=instance_of(int),
        default=-1)
    disabled: bool = attr.ib(
        validator=instance_of(bool),
        default=False)

    def to_dict(self) -> dict:
        """Convert the SwimSpec into a dictionary.
        """
        return attr.asdict(self)

    @staticmethod
    def from_dict(data: dict) -> 'SwimSpec':
        """Construct a SwimSpec from a dictionary.
        """
        return SwimSpec(**data)

    def to_json(self, *args, **kwargs) -> str:
        """Convert the SwimSpec into a JSON string.
        """
        return json.dumps(self.to_dict(), *args, **kwargs)

    @staticmethod
    def from_json(json_string: str) -> 'SwimSpec':
        """Construct a SwimSpec from a JSON string.
        """
        data = json.loads(json_string)
        return SwimSpec.from_dict(data)

    def validate(self) -> NoReturn:
        """Validate the state of the MercurySpec, raising an exception
        if the MercurySpec is not valid.
        """
        attr.validate(self)


def _swim_from_args(arg) -> SwimSpec:
    """Construct a SwimSpec from a single argument. If the argument
    if a dict, its content if forwarded to the SwimSpec constructor.
    """
    if isinstance(arg, SwimSpec):
        return arg
    elif isinstance(arg, dict):
        return MargoSpec(**arg)
    elif arg is None:
        return SwimSpec(disabled=True)
    else:
        raise TypeError(f'cannot convert type {type(arg)} into a SwimSpec')


@attr.s(auto_attribs=True, on_setattr=_check_validators, kw_only=True)
class SSGSpec:
    """SSG group specification.

    :param name: Name of the SSG group
    :type name: str

    :param pool: Pool associated with the group
    :type pool: PoolSpec

    :param credential: Credentials
    :type credential: long

    :param bootstrap: Bootstrap method
    :type bootstrap: str

    :param group_file: Group file
    :type group_file: str

    :param swim: Swim parameters
    :type swim: SwimSpec
    """

    name: str = attr.ib(
        validator=[instance_of(str), _validate_object_name],
        on_setattr=attr.setters.frozen)
    pool: PoolSpec = attr.ib(
        validator=instance_of(PoolSpec))
    credential: int = attr.ib(
        validator=instance_of(int),
        default=-1)
    bootstrap: str = attr.ib(
        validator=in_(['init', 'join', 'mpi', 'pmix', 'init|join', 'mpi|join', 'pmix|join']))
    group_file: str = attr.ib(
        validator=instance_of(str),
        default='')
    swim: Optional[SwimSpec] = attr.ib(
        validator=instance_of(SwimSpec),
        converter=_swim_from_args,
        default=None)

    def to_dict(self) -> dict:
        """Convert the SSGSpec into a dictionary.
        """
        result = {'name': self.name,
                  'pool': self.pool.name,
                  'credential': self.credential,
                  'bootstrap': self.bootstrap,
                  'group_file': self.group_file}
        if self.swim is not None:
            result['swim'] = self.swim.to_dict()
        return result

    @staticmethod
    def from_dict(data: dict, abt_spec: ArgobotsSpec) -> 'SSGSpec':
        """Construct an SSGSpec from a dictionary. Since the dictionary
        references the pool by name or index, an ArgobotsSpec is necessary
        to resolve the reference.

        :param data: Dictionary
        :type data: dict

        :param abt_spec: ArgobotsSpec in which to look for the PoolSpec
        :type abt_spec: ArgobotsSpec
        """
        args = data.copy()
        args['pool'] = abt_spec.pools[data['pool']]
        if 'swim' in args:
            args['swim'] = SwimSpec.from_dict(args['swim'])
        ssg = SSGSpec(**args)
        return ssg

    def to_json(self, *args, **kwargs) -> str:
        """Convert the SSGSpec into a JSON string.
        """
        return json.dumps(self.to_dict(), *args, **kwargs)

    @staticmethod
    def from_json(json_string: str,  abt_spec: ArgobotsSpec) -> 'SSGSpec':
        """Construct an SSGSpec from a JSON string. Since the JSON string
        references the pool by name or index, an ArgobotsSpec is necessary
        to resolve the reference.

        :param json_string: JSON string
        :type json_string: str

        :param abt_spec: ArgobotsSpec in which to look for the PoolSpec
        :type abt_spec: ArgobotsSpec
        """
        return SSGSpec.from_dict(json.loads(json_string), abt_spec)


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
    def space(type: str, max_num_pools: int, tags: list[str] = [],
              pool_association_weights: tuple[float,float]|list[float|tuple[float,float]] = (0.0, 1.0),
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
        from ConfigSpace import ConfigurationSpace, Categorical, Constant
        cs = ConfigurationSpace()
        cs.add(Constant('type', type))
        # TODO: once https://github.com/automl/ConfigSpace/issues/381 is fixed,
        # switch to storing the tags as a single constant of type list.
        # cs.add(Constant('tags', tags))
        cs.add(Constant('tags', ','.join(tags)))
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
        for i in range(0, max_num_pools):
            if isinstance(pool_association_weights, list):
                weight = pool_association_weights[i]
            else:
                weight = pool_association_weights
            cs.add(_FloatOrConst(f'pool_association_weight[{i}]', weight))
        return cs

    @staticmethod
    def from_config(name: str, provider_id: int, pools: list[PoolSpec],
                    config: 'Configuration', prefix: str = ''):
        """
        Create a ProviderSpec from a given Configuration object.

        This function must be also given the name and provider Id to give the provider,
        as well as the list of pools of the underlying ProcSpec.
        """
        from ConfigSpace import Configuration
        type = config[f'{prefix}type']
        # TODO: once https://github.com/automl/ConfigSpace/issues/381 is fixed,
        # switch to getting the tags from a single constant of type list.
        # tags = config[f'{prefix}tags']
        tags = config[f'{prefix}tags'].split(',')
        provider_config_resolver = config[f'{prefix}config_resolver']
        dependency_resolver = config[f'{prefix}dependency_resolver']
        pool_weights = [(
            float(config[f'{prefix}pool_association_weight[{i}]']), i
        ) for i in range(len(pools))]
        pool = pools[max(pool_weights)[1]]
        if provider_config_resolver is None:
            provider_config = {}
        else:
            provider_config = provider_config_resolver(config, f'{prefix}config.')
        if dependency_resolver is None:
            provider_dependencies = {}
        else:
            provider_dependencies = dependency_resolver(config, f'{prefix}dependencies.')
        return ProviderSpec(
            name=f'{name}', type=type, provider_id=provider_id,
            pool=pool, config=provider_config, tags=tags,
            dependencies=provider_dependencies)


@attr.s(auto_attribs=True, on_setattr=_check_validators, kw_only=True)
class ClientSpec:
    """Client specification.

    :param name: Name of the client
    :type name: str

    :param type: Type of client
    :type type: str

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
        """Convert the ClientSpec into a dictionary.
        """
        return {'name': self.name,
                'type': self.type,
                'dependencies': self.dependencies,
                'config': self.config,
                'tags': self.tags}

    @staticmethod
    def from_dict(data: dict) -> 'ClientSpec':
        """Construct a ClientSpec from a dictionary.

        :param data: Dictionary
        :type data: dict
        """
        return ClientSpec(**data)

    def to_json(self, *args, **kwargs) -> str:
        """Convert the ClientSpec into a JSON string.
        """
        return json.dumps(self.to_dict(), *args, **kwargs)

    @staticmethod
    def from_json(json_string: str) -> 'ClientSpec':
        """Construct a ClientSpec from a JSON string.

        :param json_string: JSON string
        :type json_string: str
        """
        return ClientSpec.from_dict(json.loads(json_string))


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

    :param abt_io: List of AbtIOSpec
    :type abt_io: list

    :param mona: List of MonaSpec
    :type mona: list

    :param ssg: List of SSGSpec
    :type ssg: list

    :param libraries: Dictionary of libraries
    :type libraries: dict

    :param providers: List of ProviderSpec
    :type providers: list

    :param clients: List of ClientSpec
    :type clients: list
    """

    margo: MargoSpec = attr.ib(
        validator=instance_of(MargoSpec),
        converter=_margo_from_args)
    _abt_io: List[AbtIOSpec] = attr.ib(
        factory=list,
        validator=instance_of(list))
    _mona: List[MonaSpec] = attr.ib(
        factory=list,
        validator=instance_of(list))
    _ssg: List[SSGSpec] = attr.ib(
        factory=list,
        validator=instance_of(list))
    libraries: dict = attr.ib(
        factory=dict,
        validator=instance_of(dict))
    _providers: list = attr.ib(
        factory=list,
        validator=instance_of(list))
    _clients: list = attr.ib(
        factory=list,
        validator=instance_of(list))
    bedrock: BedrockSpec = attr.ib(
        default=Factory(lambda self: BedrockSpec(pool=self.margo.rpc_pool),
                        takes_self=True),
        validator=instance_of(BedrockSpec))

    @property
    def abt_io(self) -> SpecListDecorator:
        """Return a decorator to access the internal list of AbtIOSpec
        and validate changes to this list.
        """
        return SpecListDecorator(list=self._abt_io, type=AbtIOSpec)

    @property
    def mona(self) -> SpecListDecorator:
        """Return a decorator to access the internal list of MonaSpec
        and validate changes to this list.
        """
        return SpecListDecorator(list=self._mona, type=MonaSpec)

    @property
    def ssg(self) -> SpecListDecorator:
        """Return a decorator to access the internal list of SSGSpec
        and validate changes to this list.
        """
        return SpecListDecorator(list=self._ssg, type=SSGSpec)

    @property
    def providers(self) -> SpecListDecorator:
        """Return a decorator to access the internal list of ProviderSpec
        and validate changes to this list.
        """
        return SpecListDecorator(list=self._providers, type=ProviderSpec)

    @property
    def clients(self) -> SpecListDecorator:
        """Return a decorator to access the internal list of ClientSpec
        and validate changes to this list.
        """
        return SpecListDecorator(list=self._clients, type=ClientSpec)

    def to_dict(self) -> dict:
        """Convert the ProcSpec into a dictionary.
        """
        data = {'margo': self.margo.to_dict(),
                'abt_io': [a.to_dict() for a in self._abt_io],
                'ssg': [g.to_dict() for g in self._ssg],
                'mona': [m.to_dict() for m in self._mona],
                'libraries': self.libraries,
                'providers': [p.to_dict() for p in self._providers],
                'clients': [c.to_dict() for c in self._clients],
                'bedrock': self.bedrock.to_dict()}
        return data

    @staticmethod
    def from_dict(data: dict) -> 'ProcSpec':
        """Construct a ProcSpec from a dictionary.
        """
        margo = MargoSpec.from_dict(data['margo'])
        abt_io = []
        mona = []
        ssg = []
        libraries = dict()
        providers = []
        bedrock = {}
        clients = []
        if 'libraries' in data:
            libraries = data['libraries']
        if 'abt_io' in data:
            for a in data['abt_io']:
                abt_io.append(AbtIOSpec.from_dict(a, margo.argobots))
        if 'ssg' in data:
            for g in data['ssg']:
                ssg.append(SSGSpec.from_dict(g, margo.argobots))
        if 'mona' in data:
            for m in data['mona']:
                mona.append(MonaSpec.from_dict(m, margo.argobots))
        if 'providers' in data:
            for p in data['providers']:
                providers.append(ProviderSpec.from_dict(p,  margo.argobots))
        if 'clients' in data:
            for c in data['clients']:
                clients.append(ClientSpec.from_dict(c))
        if 'bedrock' in data:
            bedrock = BedrockSpec.from_dict(data['bedrock'], margo.argobots)
        return ProcSpec(margo=margo,
                        abt_io=abt_io,
                        ssg=ssg,
                        mona=mona,
                        libraries=libraries,
                        providers=providers,
                        clients=clients,
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
    def space(provider_space_factories: list[dict] = [],
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
        from ConfigSpace import ConfigurationSpace, GreaterThanCondition, AndConjunction, Constant
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
        # FIXME we are serializing the families into a string because of
        # https://github.com/automl/ConfigSpace/issues/381
        # When it is fixed, just do:
        # cs.add(Constant('providers.families', families))
        cs.add(Constant('providers.families', ','.join(families)))
        for provider_group in provider_space_factories:
            family = provider_group['family']
            provider_cs = provider_group['space']
            count = provider_group.get('count', 1)
            default_count = count if isinstance(count, int) else count[0]
            hp_num_providers = _IntegerOrConst(f'providers.{family}.count', count, default=default_count)
            cs.add(hp_num_providers)
            conditions_to_add = {}
            for i in range(0, hp_num_providers.upper):
                cs.add_configuration_space(
                    prefix=f'providers.{family}[{i}]', delimiter='.',
                    configuration_space=provider_cs)
                for j in range(min_num_pools, max_num_pools):
                    param_key = f'providers.{family}[{i}].pool_association_weight[{j}]'
                    if param_key not in conditions_to_add:
                        conditions_to_add[param_key] = []
                    conditions_to_add[param_key].append(
                        GreaterThanCondition(cs[param_key], hp_num_pools, j))
                if i <= hp_num_providers.lower:
                    continue
                for param in provider_cs:
                    param_key = f'providers.{family}[{i}].{param}'
                    if param_key not in conditions_to_add:
                        conditions_to_add[param_key] = []
                    conditions_to_add[param_key].append(
                        GreaterThanCondition(cs[param_key], hp_num_providers, i))
            for param_key, conditions in conditions_to_add.items():
                if len(conditions) == 1:
                    cs.add(conditions[0])
                else:
                    cs.add(AndConjunction(*conditions))
        return cs

    @staticmethod
    def from_config(config: 'Configuration',
                    prefix: str = '', **kwargs):
        """
        Create a ProcSpec from the provided Configuration object.
        Extra parameters (**kwargs) will be propagated to the underlying
        MargoSpec.from_config().
        """
        margo_spec = MargoSpec.from_config(
            config=config, prefix=f'{prefix}margo.', **kwargs)
        pools = margo_spec.argobots.pools
        # FIXME we are serializing the families into a string because of
        # https://github.com/automl/ConfigSpace/issues/381
        # When it is fixed, just do:
        # families = config[f'{prefix}providers.families']
        families = config[f'{prefix}providers.families'].split(',')
        current_provider_id = 1
        provider_specs = []
        for family in families:
            if len(family) == 0:
                continue
            provider_count = int(config[f'{prefix}providers.{family}.count'])
            for i in range(provider_count):
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
    def space(process_space_factories: list[tuple[str, 'ConfigurationSpace', int|tuple[int,int]]] = [],
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
        from ConfigSpace import ConfigurationSpace, GreaterThanCondition, AndConjunction, Constant
        families = [f['family'] for f in process_space_factories]
        if len(set(families)) != len(process_space_factories):
            raise ValueError('Duplicate provider family in provider_space_factories')
        cs = ConfigurationSpace()
        # FIXME we are serializing the families into a string because of
        # https://github.com/automl/ConfigSpace/issues/381
        # When it is fixed, just do:
        # cs.add(Constant('processes.families', families))
        cs.add(Constant('processes.families', ','.join(families)))
        for process_group in process_space_factories:
            family = process_group['family']
            process_cs = process_group['space']
            count = process_group.get('count', 1)
            default_count = count if isinstance(count, int) else count[0]
            hp_num_processes = _IntegerOrConst(f'processes.{family}.count', count, default=default_count)
            cs.add(hp_num_processes)
            conditions_to_add = {}
            for i in range(0, hp_num_processes.upper):
                cs.add_configuration_space(
                    prefix=f'processes.{family}[{i}]', delimiter='.',
                    configuration_space=process_cs)
                if i <= hp_num_processes.lower:
                    continue
                # FIXME: the code bellow will not work because some of the parameters
                # already have a condition attached to them and can't have more added
                # see https://github.com/automl/ConfigSpace/issues/380
                for param in provider_cs:
                    param_key = f'processes.{family}[{i}].{param}'
                    if param_key not in conditions_to_add:
                        conditions_to_add[param_key] = []
                    conditions_to_add[param_key].append(
                        GreaterThanCondition(cs[param_key], hp_num_providers, i))
            for param_key, conditions in conditions_to_add.items():
                if len(conditions) == 1:
                    cs.add(conditions[0])
                else:
                    cs.add(AndConjunction(*conditions))
        return cs

    @staticmethod
    def from_config(config: 'Configuration',
                    prefix: str = '', **kwargs):
        """
        Create a ServiceSpec from the provided Configuration object.
        Extra parameters (**kwargs) will be propagated to the underlying
        ProcSpec.from_config().
        """
        # FIXME we are serializing the families into a string because of
        # https://github.com/automl/ConfigSpace/issues/381
        # When it is fixed, just do:
        # families = config[f'{prefix}processes.families']
        families = config[f'{prefix}processes.families'].split(',')
        proc_specs = []
        for family in families:
            if len(family) == 0:
                continue
            proc_count = int(config[f'{prefix}processes.{family}.count'])
            for i in range(proc_count):
                proc_specs.append(
                    ProcSpec.from_config(config, prefix=f'{prefix}processes.{family}[{i}].', **kwargs))
        return ServiceSpec(processes=proc_specs)


attr.resolve_types(MercurySpec, globals(), locals())
attr.resolve_types(PoolSpec, globals(), locals())
attr.resolve_types(SchedulerSpec, globals(), locals())
attr.resolve_types(XstreamSpec, globals(), locals())
attr.resolve_types(ArgobotsSpec, globals(), locals())
attr.resolve_types(MargoSpec, globals(), locals())
attr.resolve_types(ProviderSpec, globals(), locals())
attr.resolve_types(ClientSpec, globals(), locals())
attr.resolve_types(AbtIOSpec, globals(), locals())
attr.resolve_types(SwimSpec, globals(), locals())
attr.resolve_types(SSGSpec, globals(), locals())
attr.resolve_types(BedrockSpec, globals(), locals())
attr.resolve_types(ProcSpec, globals(), locals())
attr.resolve_types(ServiceSpec, globals(), locals())
