import typer
from typing_extensions import Annotated
from enum import Enum
from typing import Optional, List
from ..spec import PoolSpec
from ..client import ClientException


app = typer.Typer()


@app.command()
def load(
        name: Annotated[
            str, typer.Argument(help="Name of the module to load")],
        path: Annotated[
            str, typer.Option(
                "-p", "--path", help="Path to the dynamic library")],
        target: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Target addresses or group file")] = None,
        ranks: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Comma-separated list of ranks")] = None
        ):
    """
    Load a new module in the target Bedrock process(es).
    """
    from ._util import ServiceContext
    with ServiceContext(target) as service:
        for i in range(len(service)):
            try:
                sh = service[i].load_module(name, path)
            except ClientException as e:
                print(f"Error loading module \"{name}\" in {service[i].address}: {str(e)}")
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
    Lists the modules loaded in each of the target Bedrock process(es).
    """
    from ._util import ServiceContext
    from rich import print
    with ServiceContext(target) as service:
        config = { a: c["libraries"] for a, c in service.config.items() }
        print(config)
        del service


if __name__ == "__main__":
    app()
