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
from typing import List, NoReturn, Union


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
class MercurySpec:
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


@attr.s(auto_attribs=True,
        on_setattr=_check_validators,
        kw_only=True,
        hash=False,
        eq=False)
class PoolSpec:
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
        validator=in_(['fifo', 'fifo_wait']))
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
        scheduler = SchedulerSpec()
        scheduler.type = data['type']
        pools = []
        for pool_ref in data['pools']:
            pools.append(abt_spec.pools[pool_ref])
        scheduler.pools = pools
        return scheduler

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
        xstream = XstreamSpec(**args)
        xstream.scheduler = scheduler
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
        pool_list = data['pools']
        for pool_args in pool_list:
            abt.pools.add(PoolSpec.from_dict(pool_args))
        xstream_list = data['xstreams']
        for xstream_args in xstream_list:
            abt.xstreams.add(XstreamSpec.from_dict(xstream_args, abt))
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
        if len(self._xstreams):
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

    :param enable_profiling: Enable profiling
    :type enable_profiling: bool

    :param enable_diagnostics: Enable diagnostics
    :type enable_diagnostics: bool

    :param handle_cache_size: Handle cache size
    :type handle_cache_size: int

    :param profile_sparkline_timeslice_msec: Time slice for sparklines
    :type profile_sparkline_timeslice_msec: int

    :param progress_pool: Progress pool
    :type progress_pool: PoolSpec

    :param rpc_pool: RPC pool
    :type rpc_pool: PoolSpec
    """

    mercury: MercurySpec = attr.ib(
        converter=_mercury_from_args,
        validator=instance_of(MercurySpec))
    argobots: ArgobotsSpec = attr.ib(
        factory=ArgobotsSpec, validator=instance_of(ArgobotsSpec))
    progress_timeout_ub_msec: int = attr.ib(
        default=100, validator=instance_of(int))
    enable_profiling: bool = attr.ib(
        default=False, validator=instance_of(bool))
    enable_diagnostics: bool = attr.ib(
        default=False, validator=instance_of(bool))
    handle_cache_size: int = attr.ib(
        default=32, validator=instance_of(int))
    profile_sparkline_timeslice_msec: int = attr.ib(
        default=1000, validator=instance_of(int))
    progress_pool: PoolSpec = attr.ib(
        default=Factory(lambda self: self.argobots.pools[0],
                        takes_self=True),
        validator=instance_of(PoolSpec))
    rpc_pool: PoolSpec = attr.ib(
        default=Factory(lambda self: self.argobots.pools[0],
                        takes_self=True),
        validator=instance_of(PoolSpec))

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
    def from_dict(self, data: dict) -> 'MargoSpec':
        """Construct a MargoSpec from a dictionary.
        """
        abt_args = data['argobots']
        hg_args = data['mercury']
        argobots = ArgobotsSpec.from_dict(abt_args)
        mercury = MercurySpec.from_dict(hg_args)
        rpc_pool = None
        progress_pool = None
        if data['rpc_pool'] is not None:
            rpc_pool = argobots.find_pool(data['rpc_pool'])
        if data['progress_pool'] is not None:
            progress_pool = argobots.find_pool(data['progress_pool'])
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


@attr.s(auto_attribs=True, on_setattr=_check_validators, kw_only=True)
class AbtIOSpec:
    """ABT-IO instance specification.

    :param name: Name of the ABT-IO instance
    :type name: str

    :param pool: Pool associated with the instance
    :type pool: PoolSpec
    """

    name: str = attr.ib(
        validator=[instance_of(str), _validate_object_name],
        on_setattr=attr.setters.frozen)
    pool: PoolSpec = attr.ib(validator=instance_of(PoolSpec))

    def to_dict(self) -> dict:
        """Convert the AbtIOSpec into a dictionary.
        """
        return {'name': self.name,
                'pool': self.pool.name}

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
        pool = abt_spec.find_pool(data['pool'])
        abtio = AbtIOSpec(name=name, pool=pool)
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
        return SwimSpec(disable=True)
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
        validator=in_(['init', 'join', 'mpi', 'pmix']))
    group_file: str = attr.ib(
        validator=instance_of(str),
        default='')
    swim: SwimSpec = attr.ib(
        validator=instance_of(SwimSpec),
        converter=_swim_from_args,
        factory=SwimSpec)

    def to_dict(self) -> dict:
        """Convert the SSGSpec into a dictionary.
        """
        return {'name': self.name,
                'pool': self.pool.name,
                'credential': self.credential,
                'bootstrap': self.bootstrap,
                'group_file': self.group_file,
                'swim': self.swim.to_dict()}

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
        args['pool'] = abt_spec.find_pool(data['pool'])
        args['swim'] = SwimSpec.from_dict(**args['swim'])
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

    def to_dict(self) -> dict:
        """Convert the SSGSpec into a dictionary.
        """
        return {'name': self.name,
                'type': self.type,
                'pool': self.pool.name,
                'provider_id': self.provider_id,
                'dependencies': self.dependencies,
                'config': self.config}

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
        args['poo'] = abt_spec.find_pool(data['pool'])
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
        args['pool'] = abt_spec.find_pool(data['pool'])
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

    :param ssg: List of SSGSpec
    :type ssg: list

    :param libraries: Dictionary of libraries
    :type libraries: dict

    :param providers: List of ProviderSpec
    :type providers: list
    """

    margo: MargoSpec = attr.ib(
        validator=instance_of(MargoSpec),
        converter=_margo_from_args)
    _abt_io: List[AbtIOSpec] = attr.ib(
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

    def to_dict(self) -> dict:
        """Convert the ProcSpec into a dictionary.
        """
        data = {'margo': self.margo.to_dict(),
                'abt_io': [a.to_dict() for a in self._abt_io],
                'ssg': [g.to_dict() for g in self._ssg],
                'libraries': self.libraries,
                'providers': [p.to_dict() for p in self._providers],
                'bedrock': self.bedrock.to_dict()}
        return data

    @staticmethod
    def from_dict(data: dict) -> 'ProcSpec':
        """Construct a ProcSpec from a dictionary.
        """
        margo = MargoSpec.from_dict(data['margo'])
        abt_io = []
        ssg = []
        libraries = dict()
        providers = []
        bedrock = {}
        if 'libraries' in data:
            libraries = data['libraries']
        if 'abt_io' in data:
            for a in data['abt_io']:
                abt_io.append(AbtIOSpec.from_dict(a, margo.argobots))
        if 'ssg' in data:
            for g in data['ssg']:
                ssg.append(SSGSpec.from_dict(g, margo.argobots))
        if 'providers' in data:
            for p in data['providers']:
                providers.append(ProviderSpec.from_dict(p,  margo.argobots))
        if 'bedrock' in data:
            bedrock = BedrockSpec.from_dict(data['bedrock'], margo.argobots)
        return ProcSpec(margo=margo,
                        abt_io=abt_io,
                        ssg=ssg,
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


attr.resolve_types(MercurySpec, globals(), locals())
attr.resolve_types(PoolSpec, globals(), locals())
attr.resolve_types(SchedulerSpec, globals(), locals())
attr.resolve_types(XstreamSpec, globals(), locals())
attr.resolve_types(ArgobotsSpec, globals(), locals())
attr.resolve_types(MargoSpec, globals(), locals())
attr.resolve_types(AbtIOSpec, globals(), locals())
attr.resolve_types(SwimSpec, globals(), locals())
attr.resolve_types(SSGSpec, globals(), locals())
attr.resolve_types(BedrockSpec, globals(), locals())
attr.resolve_types(ProcSpec, globals(), locals())
