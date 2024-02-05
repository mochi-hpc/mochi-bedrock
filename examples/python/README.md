To use this example notebook, create a spack environment and
install Bedrock with Python support as follows.

```
spack env create my-bedrock-env
spack env activate my-bedrock-env
spack add mochi-bedrock+python
spack install
```

Then, with the `my-bedrock-env` spack environment activated,
create a Python virtual environment as follows.

```
python -m venv my-bedrock-env
source my-bedrock-env/bin/activate
python -m pip install -r requirements.txt
```

Finally, start Jupyterlab as follows.

```
jupyter notebook --ip 0.0.0.0 --port 8888 --no-browser
```

You will be able to connect to the notebook at the URL
displayed by jupyter on your terminal.
