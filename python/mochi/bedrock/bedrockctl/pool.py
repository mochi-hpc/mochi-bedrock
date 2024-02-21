import typer
from typing_extensions import Annotated
from enum import Enum
from typing import Optional
from ..spec import PoolSpec
from ..client import ClientException


app = typer.Typer()


class PoolKind(str, Enum):
    fifo = "fifo"
    fifo_wait = "fifo_wait"
    prio = "prio"
    prio_wait = "prio_wait"
    earliest_first = "earliest_first"


class PoolAccess(str, Enum):
    private = "private"
    spsc = "spsc"
    mpsc = "mpsc"
    spmc = "spmc"
    mpmc = "mpmc"


@app.command()
def create(
        name: Annotated[
            str, typer.Argument(help="Name of the pool to create")],
        kind: Annotated[
            PoolKind, typer.Option(
                "-k", "--kind", help="Kind of pool")] = PoolKind.fifo_wait,
        access: Annotated[
            PoolAccess, typer.Option(
                "-a", "--access", help="Access policy of the pool")] = PoolAccess.mpmc,
        target: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Target addresses or group file")] = None,
        ranks: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Comma-separated list of ranks")] = None
        ):
    """
    Create a new pool in the target Bedrock process(es).
    """
    from ._util import parse_target_ranks, rank_is_in
    ranks = parse_target_ranks(ranks)
    from ._util import ServiceContext
    with ServiceContext(target) as service:
        for i in range(len(service)):
            if not rank_is_in(i, ranks):
                continue
            try:
                service[i].add_pool(PoolSpec(
                    name=name, kind=kind.value, access=access.value))
            except ClientException as e:
                print(f"Error adding pool in {service[i].address}: {str(e)}")
        del service


@app.command()
def list(target: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Target addresses or group file")] = None,
         ranks: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Comma-separated list of ranks")] = None
         ):
    """
    Lists the pools in each of the target Bedrock process(es).
    """
    from ._util import parse_target_ranks, rank_is_in
    ranks = parse_target_ranks(ranks)
    from ._util import ServiceContext
    from rich import print
    with ServiceContext(target) as service:
        config = { a: c["margo"]["argobots"]["pools"] for a, c in service.config.items() }
        for i in range(len(service)):
            if not rank_is_in(i, ranks):
                del config[service[i].address]
        print(config)
        del service


@app.command()
def remove(
        name: Annotated[
            str, typer.Argument(help="Name of the pool to remove")],
        target: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Target addresses or group file")] = None,
        ranks: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Comma-separated list of ranks")] = None
        ):
    """
    Remove a pool from the target Bedrock process(es).
    """
    from ._util import parse_target_ranks, rank_is_in
    ranks = parse_target_ranks(ranks)
    from ._util import ServiceContext
    with ServiceContext(target) as service:
        for i in range(len(service)):
            if not rank_is_in(i, ranks):
                continue
            try:
                service[i].remove_pool(name)
            except ClientException as e:
                print(f"Error removing pool in {service[i].address}: {str(e)}")
        del service


if __name__ == "__main__":
    app()
