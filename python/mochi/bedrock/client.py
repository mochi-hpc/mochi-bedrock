# (C) 2018 The University of Chicago
# See COPYRIGHT in top-level directory.


"""
.. module:: client
   :synopsis: This package provides access to the Bedrock C++ wrapper

.. moduleauthor:: Matthieu Dorier <mdorier@anl.gov>


"""


try:
    import _pybedrock
except ModuleNotFoundError as error:
    raise ModuleNotFoundError("Could not find bedrock client extension. Did you disable it?")

import pymargo.core
import pymargo


class Client(_pybedrock.Client):

    def __init__(self, arg):
        if isinstance(arg, pymargo.core.Engine):
            super().__init__(arg.get_internal_mid())
            self._engine = None
        elif isinstance(arg, str):
            self._engine = pymargo.core.Engine(
                arg, pymargo.client)
            super().__init__(self._engine.get_internal_mid())
        else:
            raise TypeError(f'Invalid argument type {type(arg)}')

    def __del__(self):
        if self._engine is not None:
            self._engine.finalize()
            del self._engine
