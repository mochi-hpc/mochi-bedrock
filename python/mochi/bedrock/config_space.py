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
from typing import Sequence, Any


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
        self._conditions = {} # associates the name of the hp the condition applies to the condition

    def add(self, arg: (Hyperparameter|ConditionLike|ForbiddenLike)):
        if self._frozen:
            raise PermissionError("ConfigurationSpace is already frozen")
        if isinstance(arg, Condition):
            if arg.child.name not in self._conditions:
                self._conditions[arg.child.name] = []
            self._conditions[arg.child.name].append(arg)
        elif isinstance(arg, Conjunction):
            if arg.child.name not in self._conditions:
                self._conditions[arg.child.name] = []
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
        conditions_from_sub = []
        for param, conditions in configuration_space._conditions.items():
            for cond in conditions:
                conditions_from_sub.append(
                    (type(cond), cond.child.name, cond.parent.name, cond.value))
        self._inner.add_configuration_space(
            prefix=prefix, configuration_space=configuration_space._inner,
            delimiter=delimiter, parent_hyperparameter=parent_hyperparameter)
        for cond_from_sub in conditions_from_sub:
            cond_type, child_name, parent_name, value = cond_from_sub
            child_name = prefix + delimiter + child_name
            parent_name = prefix + delimiter + parent_name
            child = self._inner[child_name]
            parent = self._inner[parent_name]
            self.add(cond_type(child, parent, value))

    def __getitem__(self, name):
        return self._inner[name]

    def __contains__(self, name):
        return name in self._inner

    def __iter__(self):
        return self._inner.__iter__()

    def items(self):
        return self._inner.items()

    def freeze(self):
        for name, conditions in self._conditions.items():
            if len(conditions) == 1:
                self._inner.add(conditions[0])
            else:
                self._inner.add(AndConjunction(*conditions))
        self._frozen = True
        return self._inner


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
