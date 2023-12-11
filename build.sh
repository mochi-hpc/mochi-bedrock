#!/bin/bash
SCRIPT_DIR=$(dirname "$0")
cmake $SCRIPT_DIR -DENABLE_TESTS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_MPI=ON -DENABLE_MONA=ON -DENABLE_SSG=ON -DENABLE_ABT_IO=ON