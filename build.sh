#!/bin/bash
SCRIPT_DIR=$(dirname "$0")
cmake $SCRIPT_DIR -DENABLE_TESTS=ON -DENABLE_EXAMPLES=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_PYTHON=ON -DENABLE_MPI=ON -DENABLE_MONA=ON -DENABLE_SSG=ON -DENABLE_ABT_IO=ON -DENABLE_FLOCK=ON
