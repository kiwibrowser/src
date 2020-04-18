#!/usr/bin/env python
#-------------------------------------------------------------------------------
# test/run_all_unittests.py
#
# Run all unit tests (alternative to running 'python -m unittest discover ...')
#
# Eli Bendersky (eliben@gmail.com)
# This code is in the public domain
#-------------------------------------------------------------------------------
from __future__ import print_function

import os, sys

try:
    import unittest2 as unittest
except ImportError:
    import unittest


def main():
    if not os.path.isdir('test'):
        print('!! Please execute from the root directory of pyelftools')
        return 1
    else:
        tests = unittest.TestLoader().discover('test', 'test*.py', 'test')
        result = unittest.TextTestRunner().run(tests)
        if result.wasSuccessful():
            return 0
        else:
            return 1

if __name__ == '__main__':
    sys.exit(main())
