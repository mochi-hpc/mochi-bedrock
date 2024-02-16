import typer
from typing_extensions import Annotated
from enum import Enum
from typing import Optional
from ..spec import PoolSpec
from ..client import ClientException


app = typer.Typer()


class SchedType(str, Enum):
    default = "default"
    basic = "basic"
    prio = "prio"
    randws = "randws"
    basic_wait = "basic_wait"


@app.command()
def create(
        name: Annotated[
            str, typer.Argument(help="Name of xstream to create")],
        pools: Annotated[
            str, typer.Option(
                "-p", "--pools", help="Comma-separated list of pool names")],
        scheduler: Annotated[
            SchedType, typer.Option(
                "-s", "--scheduler", help="Type of scheduler")] = SchedType.basic_wait,
        target: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Target addresses or group file")] = None,
        ranks: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Comma-separated list of ranks")] = None
        ):
    """
    Create a new xstream in the target Bedrock process(es).
    """
    from ._util import ServiceContext
    if len(pools) == 0:
        print(f"Error: invalid list of pools")
        raise typer.Exit(-1)
    pools = pools.split(",")
    xstream = {
        "name": name,
        "scheduler": {
            "pools": pools,
            "type": scheduler.value
        }
    }
    with ServiceContext(target) as service:
        for i in range(len(service)):
            try:
                sh = service[i].add_xstream(xstream)
            except ClientException as e:
                print(f"Error adding xstream in {service[i].address}: {str(e)}")
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
    Lists the xstreams in each of the target Bedrock process(es).
    """
    from ._util import ServiceContext
    from rich import print
    with ServiceContext(target) as service:
        config = { a: c["margo"]["argobots"]["xstreams"] for a, c in service.config.items() }
        print(config)
        del service


@app.command()
def remove(
        name: Annotated[
            str, typer.Argument(help="Name of the xstream to remove")],
        target: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Target addresses or group file")] = None,
        ranks: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Comma-separated list of ranks")] = None
        ):
    """
    Remove an xstream from the target Bedrock process(es).
    """
    from ._util import ServiceContext
    with ServiceContext(target) as service:
        for i in range(len(service)):
            try:
                sh = service[i].remove_xstream(name)
            except ClientException as e:
                print(f"Error removing xstream in {service[i].address}: {str(e)}")
        del service


if __name__ == "__main__":
    app()
