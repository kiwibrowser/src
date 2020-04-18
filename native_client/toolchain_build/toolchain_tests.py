#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Run all python tests in this directory."""

import sys
import unittest

MODULES = [
    'once_test',
    'toolchain_main_test',
]

suite = unittest.TestLoader().loadTestsFromNames(MODULES)
result = unittest.TextTestRunner(verbosity=2).run(suite)
if result.wasSuccessful():
  sys.exit(0)
else:
  sys.exit(1)
