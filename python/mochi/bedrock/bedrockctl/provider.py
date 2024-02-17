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
            str, typer.Argument(help="Name of the provider to create")],
        type: Annotated[
            str, typer.Option(
                "-t", "--type", help="Type of provider")],
        provider_id: Annotated[
            Optional[int], typer.Option(
                "-i", "--id", help="Provider id")] = None,
        pool: Annotated[
            str, typer.Option(
                "-p", "--pool", help="Pool for the provider to use")] = "__primary__",
        dependencies: Annotated[
            List[str], typer.Option(
                "-d", "--dependency", help="Dependency in the form \"name:value\"")] = [],
        tags: Annotated[
            List[str], typer.Option(
                "-a", "--tag", help="Tag")] = [],
        target: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Target addresses or group file")] = None,
        ranks: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Comma-separated list of ranks")] = None
        ):
    """
    Create a new provider in the target Bedrock process(es).
    """
    from ._util import _parse_config_from_args, _parse_dependencies
    config = _parse_config_from_args(ctx.args)
    dependencies = _parse_dependencies(dependencies)
    provider = {
        "name": name,
        "type": type,
        "provider_id": provider_id if provider_id is not None else 65535,
        "pool": pool,
        "config": config,
        "dependencies": dependencies,
        "tags": tags
    }
    from ._util import ServiceContext
    with ServiceContext(target) as service:
        for i in range(len(service)):
            try:
                service[i].add_provider(provider)
            except ClientException as e:
                print(f"Error adding provider in {service[i].address}: {str(e)}")
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
    Lists the providers in each of the target Bedrock process(es).
    """
    from ._util import ServiceContext
    from rich import print
    with ServiceContext(target) as service:
        config = { a: c["providers"] for a, c in service.config.items() }
        print(config)
        del service


@app.command()
def remove(
        name: Annotated[
            str, typer.Argument(help="Name of the provider to remove")],
        target: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Target addresses or group file")] = None,
        ranks: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Comma-separated list of ranks")] = None
        ):
    """
    Remove a provider from the target Bedrock process(es).
    """
    print("This command is not implemented yet")
    raise typer.Exit(code=-1)


if __name__ == "__main__":
    app()
