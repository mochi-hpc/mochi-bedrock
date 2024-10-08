import typer
from typing_extensions import Annotated
from pymargo.core import Engine


app = typer.Typer(help="Bedrock CLI.")


from .pool import app as pool_app
app.add_typer(pool_app, name="pool", help="Access and modify pools")

from .xstream import app as xstream_app
app.add_typer(xstream_app, name="xstream", help="Access and modify xstreams")

from .module import app as module_app
app.add_typer(module_app, name="module", help="Access and modify modules")

from .provider import app as provider_app
app.add_typer(provider_app, name="provider", help="Access and modify providers")


@app.command()
def status():
    """
    Print the list of addresses bedrockctl is currently set to interact with.
    """
    from ._util import ServiceContext
    with ServiceContext() as service:
        for i in range(len(service)):
            print(service[i].address)
        del service


@app.command()
def show():
    """
    Show the current configuration of the service.
    """
    from ._util import ServiceContext
    from rich import print
    with ServiceContext() as service:
        print(service.config)
        del service


if __name__ == "__main__":
    app()
