# (C) 2024 The University of Chicago
# See COPYRIGHT in top-level directory.


"""
.. module:: config_space
   :synopsis: This package contains helper classes and functions
   to help write config spaces.

.. moduleauthor:: Matthieu Dorier <mdorier@anl.gov>


"""


import ConfigSpace
from ConfigSpace.hyperparameters import Hyperparameter
from ConfigSpace.conditions import Condition, Conjunction, ConditionLike
from ConfigSpace.forbidden import ForbiddenLike
from typing import Sequence, Any, Mapping, Hashable


# Configuration
Configuration = ConfigSpace.Configuration
# Hyperparameters
CategoricalHyperparameter = ConfigSpace.CategoricalHyperparameter
UniformFloatHyperparameter = ConfigSpace.UniformFloatHyperparameter
UniformIntegerHyperparameter = ConfigSpace.UniformIntegerHyperparameter
BetaFloatHyperparameter = ConfigSpace.BetaFloatHyperparameter
BetaIntegerHyperparameter = ConfigSpace.BetaIntegerHyperparameter
NormalFloatHyperparameter = ConfigSpace.NormalFloatHyperparameter
NormalIntegerHyperparameter = ConfigSpace.NormalIntegerHyperparameter
Constant = ConfigSpace.Constant
UnParametrizedHyperparameter = ConfigSpace.UnParametrizedHyperparameter
OrdinalHyperparameter = ConfigSpace.OrdinalHyperparameter
IntegerHyperparameter = ConfigSpace.hyperparameters.IntegerHyperparameter
# Conditions
AndConjunction = ConfigSpace.AndConjunction
OrConjunction = ConfigSpace.OrConjunction
EqualsCondition = ConfigSpace.EqualsCondition
NotEqualsCondition = ConfigSpace.NotEqualsCondition
InCondition = ConfigSpace.InCondition
GreaterThanCondition = ConfigSpace.GreaterThanCondition
LessThanCondition = ConfigSpace.LessThanCondition
# Forbidden
ForbiddenAndConjunction = ConfigSpace.ForbiddenAndConjunction
ForbiddenEqualsClause = ConfigSpace.ForbiddenEqualsClause
ForbiddenInClause = ConfigSpace.ForbiddenInClause
ForbiddenLessThanRelation = ConfigSpace.ForbiddenLessThanRelation
ForbiddenEqualsRelation = ConfigSpace.ForbiddenEqualsRelation
ForbiddenGreaterThanRelation = ConfigSpace.ForbiddenGreaterThanRelation
# Distributions
Beta = ConfigSpace.Beta
Categorical = ConfigSpace.Categorical
Distribution = ConfigSpace.Distribution
Float = ConfigSpace.Float
Integer = ConfigSpace.Integer
Normal = ConfigSpace.Normal
Uniform = ConfigSpace.Uniform


class ConfigurationSpace:

    def __init__(self, name: str|None = None,
                 seed: int|None = None,
                 meta: dict|None = None):
        self._frozen = False
        self._inner = ConfigSpace.ConfigurationSpace(name=name, seed=seed, meta=meta)
        self._conditions = {} # associates the name of the condition's child to the tuple
                              # (condition_type, parent_name, value)

    def add(self, arg: (Hyperparameter|ConditionLike|ForbiddenLike|list)):
        if self._frozen:
            raise PermissionError("ConfigurationSpace is already frozen")
        if isinstance(arg, list):
            for x in arg:
                self.add(x)
            return
        if isinstance(arg, Condition):
            if arg.child.name not in self._conditions:
                self._conditions[arg.child.name] = []
            self._conditions[arg.child.name].append(
                (type(arg), arg.parent.name, arg.value))
        elif isinstance(arg, Conjunction):
            for component in arg.components:
                self.add(component)
        else:
            self._inner.add(arg)

    def add_configuration_space(self, prefix: str,
                                configuration_space: 'ConfigurationSpace',
                                delimiter: str = '.',
                                parent_hyperparameter: dict|None = None):
        if self._frozen:
            raise PermissionError("ConfigurationSpace is already frozen")
        parent_equals_conditions = {}
        if parent_hyperparameter is not None:
            parent_name = parent_hyperparameter['parent'].name
            parent_value = parent_hyperparameter['value']
            for child_name in configuration_space:
                parent_equals_conditions[child_name] = (EqualsCondition, parent_name, parent_value)

        self._inner.add_configuration_space(
            prefix=prefix, configuration_space=configuration_space._inner,
            delimiter=delimiter)

        for child_name, cond_list in configuration_space._conditions.items():
            child_name = prefix + delimiter + child_name
            for cond_tuple in cond_list:
                cond_type, parent_name, value = cond_tuple
                parent_name = prefix + delimiter + parent_name
                if child_name not in self._conditions:
                    self._conditions[child_name] = []
                self._conditions[child_name].append(
                    (cond_type, parent_name, value))

        for child_name, cond in parent_equals_conditions.items():
            child_name = prefix + delimiter + child_name
            if child_name not in self._conditions:
                self._conditions[child_name] = []
            self._conditions[child_name].append(cond)

    def __getitem__(self, name):
        return self._inner[name]

    def __contains__(self, name):
        return name in self._inner

    def __iter__(self):
        return self._inner.__iter__()

    def items(self):
        return self._inner.items()

    def freeze(self):

        def convert_condition_tuples(child_name, conditions: list[tuple[type,str,Any]]):
            result = []
            child = self._inner[child_name]
            for cond_tuple in conditions:
                cond_type, parent_name, value = cond_tuple
                parent = self._inner[parent_name]
                result.append(cond_type(child, parent, value))
            return result

        for child_name, conditions in self._conditions.items():
            conditions = convert_condition_tuples(child_name, conditions)
            if len(conditions) == 1:
                self._inner.add(conditions[0])
            else:
                self._inner.add(AndConjunction(*conditions))
        self._frozen = True
        return self._inner


class PrefixedConfigSpaceWrapper:

    def __init__(self, configuration_space: ConfigurationSpace, prefix: str):
        self.prefix = prefix
        self.cs = configuration_space

    def add(self, arg):
        if isinstance(arg, Hyperparameter):
            arg.name = self.prefix + arg.name
        if isinstance(arg, list):
            for a in args:
                self.add(a)
        else:
            self.cs.add(arg)

    def __getattr__(self, key):
        return getattr(self.cs, key)


def CategoricalOrConst(name: str, items: Sequence[Any]|Any, *,
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


def IntegerOrConst(name: str, bounds: int|tuple[int, int], *,
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

def FloatOrConst(name: str, bounds: float|tuple[float, float], *,
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


def CategoricalChoice(name: str,
                      num_options: IntegerHyperparameter|int,
                      weights: Sequence[float] | None = None,
                      meta: Mapping[Hashable, Any] | None = None) -> list[Any]:
    """
    This function is a helper to build a Categorical hyperparameter with the specified
    name, whose value must be taken between 0 and num_options-1. num_options can be either
    and integer or an IntegerHyperparameter. In the later case, this function will also
    construct the required Forbidden clauses to constrain the value of the choice.

    The result of this function is a list of Hyperparameter and Forbidden clauses, which
    should then be passed to a ConfigurationSpace's add function.
    """

    max_options = num_options if isinstance(num_options, int) else int(num_options.upper)
    min_options = num_options if isinstance(num_options, int) else int(num_options.lower)
    choice = CategoricalOrConst(name, list(range(max_options)),
                                default=0, meta=meta, weights=weights)
    conditions = []
    if max_options != min_options:
        for j in range(min_options, max_options):
            conditions.append(
                ForbiddenAndConjunction(
                    ForbiddenEqualsClause(num_options, j),
                    ForbiddenInClause(choice, list(range(j, max_options)))))
    return [choice, *conditions]

