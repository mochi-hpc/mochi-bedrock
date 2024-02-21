import typer
from typing_extensions import Annotated
from enum import Enum
from typing import Optional, List
from ..spec import PoolSpec
from ..client import ClientException


app = typer.Typer()


@app.command()
def create(
        name: Annotated[
            str, typer.Argument(help="Name of the SSG group to create")],
        file: Annotated[
            str, typer.Option(
                "-f", "--file", help="Group file")],
        pool: Annotated[
            str, typer.Option(
                "-p", "--pool", help="Pool for the SSG group to use")] = "__primary__",
        disable_swim: Annotated[
            bool, typer.Option("--disable-swim/--enable-swim",
                               help="Disable SWIM protocol")] = False,
        swim_period_length_ms: Annotated[
            int, typer.Option(help="SWIM period length in milliseconds")] = 0,
        swim_suspect_timeout_periods: Annotated[
            int, typer.Option(help="SWIM number of suspect timeout periods")] = -1,
        swim_subgroup_member_count: Annotated[
            int, typer.Option(help="SWIM subgroup member count")] = -1,
        credential: Annotated[int, typer.Option(help="Credential")] = -1,
        target: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Target addresses or group file")] = None,
        ranks: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Comma-separated list of ranks")] = None
        ):
    """
    Create a new SSG group in the target Bedrock process(es).
    """
    from ._util import parse_target_ranks, rank_is_in
    ranks = parse_target_ranks(ranks)
    ssg_group = {
        "name": name,
        "pool": pool,
        "group_file": file,
        "credential": credential,
        "bootstrap": "init",
        "swim": {
            "disabled": disable_swim,
            "period_length_ms": swim_period_length_ms,
            "suspect_timeout_periods": swim_suspect_timeout_periods,
            "subgroup_member_count": swim_subgroup_member_count
        }
    }
    from ._util import ServiceContext
    with ServiceContext(target) as service:
        # TODO: for now we need the first rank to "init" the group
        # then the next processes need to "join" it. It would be
        # better to be able to add a list of addresses as agument.
        for i in range(0, len(service)):
            if not rank_is_in(i, ranks):
                continue
            try:
                service[i].add_ssg_group(ssg_group)
                ssg_group["bootstrap"] = "join"
            except ClientException as e:
                print(f"Error adding SSG group in {service[i].address}: {str(e)}")
                if i == 0:
                    raise typer.Exit(code=-1)
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
    Lists the SSG groups in each of the target Bedrock process(es).
    """
    from ._util import parse_target_ranks, rank_is_in
    ranks = parse_target_ranks(ranks)
    from ._util import ServiceContext
    from rich import print
    with ServiceContext(target) as service:
        config = { a: c["ssg"] for a, c in service.config.items() }
        for i in range(len(service)):
            if not rank_is_in(i, ranks):
                del config[service[i].address]
        print(config)
        del service


@app.command()
def remove(
        name: Annotated[
            str, typer.Argument(help="Name of the SSG group to remove")],
        target: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Target addresses or group file")] = None,
        ranks: Annotated[
            Optional[str], typer.Option(hidden=True,
                help="Comma-separated list of ranks")] = None
        ):
    """
    Remove an SSG group from the target Bedrock process(es).
    """
    print("This command is not implemented yet")
    raise typer.Exit(code=-1)


if __name__ == "__main__":
    app()
