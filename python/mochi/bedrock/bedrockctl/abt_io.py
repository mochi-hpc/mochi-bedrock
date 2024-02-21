import typer
from typing_extensions import Annotated
from enum import Enum
from typing import Optional, List
from ..spec import PoolSpec
from ..client import ClientException


app = typer.Typer()


@app.command(context_settings={"allow_extra_args": True, "ignore_unknown_options": True})
def create(
        ctx: typer.Context,
        name: Annotated[
            str, typer.Argument(help="Name of the ABT-IO instance to create")],
        pool: Annotated[
            str, typer.Option(
                "-p", "--pool", help="Pool for the ABT-IO instance to use")] = "__primary__",
        target: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Target addresses or group file")] = None,
        ranks: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Comma-separated list of ranks")] = None
        ):
    """
    Create a new ABT-IO instance in the target Bedrock process(es).
    """
    from ._util import _parse_config_from_args
    config = _parse_config_from_args(ctx.args)
    abt_io = {
        "name": name,
        "pool": pool,
        "config": config
    }
    from ._util import ServiceContext
    with ServiceContext(target) as service:
        for i in range(len(service)):
            try:
                service[i].add_abtio_instance(abt_io)
            except ClientException as e:
                print(f"Error adding ABT-IO instance in {service[i].address}: {str(e)}")
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
    Lists the ABT-IO instances in each of the target Bedrock process(es).
    """
    from ._util import ServiceContext
    from rich import print
    with ServiceContext(target) as service:
        config = { a: c["abt_io"] for a, c in service.config.items() }
        print(config)
        del service


@app.command()
def remove(
        name: Annotated[
            str, typer.Argument(help="Name of the ABT-IO instance to remove")],
        target: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Target addresses or group file")] = None,
        ranks: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Comma-separated list of ranks")] = None
        ):
    """
    Remove an ABT-IO instance from the target Bedrock process(es).
    """
    print("This command is not implemented yet")
    raise typer.Exit(code=-1)


if __name__ == "__main__":
    app()
