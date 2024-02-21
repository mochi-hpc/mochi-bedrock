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
            str, typer.Argument(help="Name of the client to create")],
        type: Annotated[
            str, typer.Option(
                "-t", "--type", help="Type of client")],
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
    Create a new client in the target Bedrock process(es).
    """
    from ._util import parse_config_from_args
    config = parse_config_from_args(ctx.args)
    from ._util import parse_target_ranks, rank_is_in
    ranks = parse_target_ranks(ranks)
    from ._util import parse_dependencies
    dependencies = parse_dependencies(dependencies)
    client = {
        "name": name,
        "type": type,
        "config": config,
        "dependencies": dependencies,
        "tags": tags
    }
    from ._util import ServiceContext
    with ServiceContext(target) as service:
        for i in range(len(service)):
            if not rank_is_in(i, ranks):
                continue
            try:
                service[i].add_client(client)
            except ClientException as e:
                print(f"Error adding client in {service[i].address}: {str(e)}")
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
    Lists the clients in each of the target Bedrock process(es).
    """
    from ._util import parse_target_ranks, rank_is_in
    ranks = parse_target_ranks(ranks)
    from ._util import ServiceContext
    from rich import print
    with ServiceContext(target) as service:
        config = { a: c["clients"] for a, c in service.config.items() }
        for i in range(len(service)):
            if not rank_is_in(i, ranks):
                del config[service[i].address]
        print(config)
        del service


@app.command()
def remove(
        name: Annotated[
            str, typer.Argument(help="Name of the client to remove")],
        target: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Target addresses or group file")] = None,
        ranks: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Comma-separated list of ranks")] = None
        ):
    """
    Remove a client from the target Bedrock process(es).
    """
    print("This command is not implemented yet")
    raise typer.Exit(code=-1)


if __name__ == "__main__":
    app()
