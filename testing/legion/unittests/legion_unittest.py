# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Base test class for Legion-specific unittests.

Currently this module is only needed to setup the import paths for the
unittests. This will allow unittests to use the import format:

from legion.foo import bar

Using this base class for all unittests allows for easier extensibility in
the future.
"""

import os
import sys
import unittest

# Setup import paths
THIS_DIR = os.path.dirname(os.path.abspath(__file__))
LEGION_IMPORT_FIX = os.path.join(THIS_DIR, '..', '..')
sys.path.append(LEGION_IMPORT_FIX)


class TestCase(unittest.TestCase):
  pass


def main():
  unittest.main(verbosity=0, argv=sys.argv[:1])
