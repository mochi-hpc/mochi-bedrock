import os
import typer
from typing import Optional
from ..client import Client
from pymargo.core import Engine


class ServiceContext:

    def __init__(self, target=None):
        if target is None:
            self.connection = os.environ.get("BEDROCKCTL_CONNECTION", None)
        else:
            self.connection = target
        if self.connection is None:
            print(f"Error: bedrockctl not connected")
            raise typer.Exit(code=-1)
        if "://" in self.connection:
            self.protocol = connection.split(":")[0]
        elif os.path.exists(self.connection) and os.path.isfile(self.connection):
            import pyssg
            self.protocol = pyssg.get_group_transport_from_file(self.connection)
        else:
            print(f"Error: could not find file or address {self.connection}")
            raise typer.Exit(code=-1)

    def __enter__(self):
        self.engine = Engine(self.protocol)
        client = Client(self.engine)
        import pyssg
        pyssg.init()
        if "://" in self.connection:
            self.service = client.make_service_group_handle([self.connection])
        else:
            self.service = client.make_service_group_handle(self.connection)
        return self.service

    def __exit__(self, type, value, traceback):
        self.service = None
        del self.service
        import pyssg
        pyssg.finalize()
        self.engine.finalize()
