#!/bin/bash
#
# Run all the regression tests
cd $(dirname "$0")

# Halt on error
set -e

# Verify that the loader is working
./run_loader_tests.sh
./run_extra_loader_tests.sh

