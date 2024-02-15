import typer
from typing_extensions import Annotated
from pymargo.core import Engine


app = typer.Typer(help="Bedrock CLI.")

from .pool import app as pool_app
app.add_typer(pool_app, name="pool")


@app.command()
def status():
    """
    Print the list of addresses bedrockctl is currently set to interact with.
    """
    from ._util import Service
    with Service() as service:
        for i in range(len(service)):
            print(service[i].address)
        del service


@app.command()
def show():
    """
    Show the current configuration of the service.
    """
    from ._util import Service
    from rich import print
    with Service() as service:
        print(service.config)
        del service


if __name__ == "__main__":
    app()
