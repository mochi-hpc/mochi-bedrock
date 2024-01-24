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


class Client(pybedrock_client.Client):

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
