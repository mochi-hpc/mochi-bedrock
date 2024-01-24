# (C) 2018 The University of Chicago
# See COPYRIGHT in top-level directory.


"""
.. module:: local
   :synopsis: This package helps deploy a Bedrock service on a local machine

.. moduleauthor:: Matthieu Dorier <mdorier@anl.gov>


"""
from .spec import ProcSpec


def _in_notebook():
    try:
        from IPython import get_ipython
        if 'IPKernelApp' not in get_ipython().config:
            return False
    except ImportError:
        return False
    return True


def _run_in_background(fn, *args, **kwargs):
    from IPython.lib import backgroundjobs as bg
    jobs = bg.BackgroundJobManager()
    jobs.new(fn, *args, kw=kwargs)


def _run_as_daemon(cmd):
    import subprocess as sp
    sp.Popen(cmd, shell=True, executable='/bin/bash')


def _bash(cmd, print_stdout=True, print_stderr=True):
    import subprocess as sp
    import sys
    proc = sp.Popen(cmd, stderr=sp.PIPE, stdout=sp.PIPE,
                    shell=True, universal_newlines=True,
                    executable='/bin/bash')
    while proc.poll() is None:
        for stdout_line in proc.stdout:
            if stdout_line != '':
                if print_stdout:
                    print(stdout_line, end='')
        for stderr_line in proc.stderr:
            if stderr_line != '':
                if print_stderr:
                    print(stderr_line, end='', file=sys.stderr)
    return proc.wait()


def deploy(
        spec: 'ProcSpec',
        bedrock_binary: str = 'bedrock',
        log_level: str = 'info',
        config_file_name: str = None,
        workdir: str = '.',
        daemon: bool = False,
        output: bool = True):
    from tempfile import NamedTemporaryFile
    config = spec.to_json()
    protocol = spec.margo.mercury.protocol
    input_file = None
    if config_file_name is None:
        input_file = NamedTemporaryFile(
            prefix='bedrock-',
            dir=workdir,
            delete=False,
            suffix='.json')
        config_file_name = input_file.name
    with open(config_file_name, 'w+') as f:
        f.write(config)
    cmd = [bedrock_binary, protocol,
           '-v', log_level,
           '-c', config_file_name]
    command = ' '.join(cmd)
    if not daemon:
        if _in_notebook():
            print('WARNING: daemon=False used in notebook will block' +
                  ' the notebook until the Bedrock service shuts down')
        _bash(command, print_stdout=output, print_stderr=output)
    else:
        if _in_notebook():
            _run_in_background(
                _bash, command,
                print_stdout=output, print_stderr=output)
        else:
            if output:
                print('WARNING: output=True and daemon=True both specified,' +
                      ' output will be redirected to /dev/null anyway.')
            _run_as_daemon(command)
