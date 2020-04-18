#!/usr/bin/env python
#-------------------------------------------------------------------------------
# test/all_tests.py
#
# Run all pyelftools tests.
#
# Eli Bendersky (eliben@gmail.com)
# This code is in the public domain
#-------------------------------------------------------------------------------
from __future__ import print_function
import subprocess, sys
from utils import is_in_rootdir

def run_test_script(path):
    cmd = [sys.executable, path]
    print("Running '%s'" % ' '.join(cmd))
    subprocess.check_call(cmd)

def main():
    if not is_in_rootdir():
        testlog.error('Error: Please run me from the root dir of pyelftools!')
        return 1
    run_test_script('test/run_all_unittests.py')
    run_test_script('test/run_examples_test.py')
    run_test_script('test/run_readelf_tests.py')

if __name__ == '__main__':
    sys.exit(main())
