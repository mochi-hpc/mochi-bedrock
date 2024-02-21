import os
import typer
from typing import Optional, List
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


def parse_config_from_args(args: List[str]):
    config = {}
    while len(args) != 0:
        arg = args[0]
        args.pop(0)
        if len(args) == 0:
            print(f"Error: configuration argument {arg} has no value")
            raise typer.Exit(code=-1)
        if not arg.startswith("--config."):
            continue
        value = args[0]
        args.pop(0)
        if value in ["True", "true"]:
            value = True
        elif value in ["False", "false"]:
            value = False
        elif value.isdecimal():
            value = int(value)
        elif value.replace(".","", 1).isnumeric():
            value = float(value)
        field_names = arg.split(".")
        field_names.pop(0)
        section = config
        for i in range(len(field_names)):
            if i == len(field_names) - 1:
                section[field_names[i]] = value
            else:
                if field_names[i] not in section:
                    section[field_names[i]] = {}
                section = section[field_names[i]]
    return config


def parse_dependencies(deps: List[str]):
    dependencies = {}
    for dep in deps:
        if dep.count(":") != 1:
            print(f"Error: ill-formatted dependency {dep}")
            raise typer.Exit(code=-1)
        key, value = tuple(dep.split(":"))
        if key in dependencies:
            print(f"Error: dependency {key} specified multiple times")
            raise typer.Exit(code=-1)
        dependencies[key] = value
    return dependencies


def parse_target_ranks(input_string, sort=False):
    if input_string is None:
        return None
    import re
    match = re.fullmatch(r'(\d+(-\d+)?)(,\d+(-\d+)?)*', input_string)
    if match is None:
        print(f"Invalid rank list: {input_string}")
        raise typer.Exit(code=-1)
    ranks = []
    for part in input_string.split(','):
        if '-' in part:
            start, end = map(int, part.split('-'))
            if start > end:
                print("Invalid range in rank list: {start} > {end}")
                raise typer.Exit(code=-1)
            ranks.extend(range(start, end + 1))
        else:
            ranks.append(int(part))
    if sort:
        return list(sorted(ranks))
    else:
        return ranks


def rank_is_in(rank, ranks: Optional[List[int]]):
    if ranks is None:
        return True
    else:
        return rank in ranks
