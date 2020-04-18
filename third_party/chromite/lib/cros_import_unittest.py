# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test the cros_import module."""

from __future__ import print_function

from chromite.lib import cros_import
from chromite.lib import cros_test_lib


class ImportTest(cros_test_lib.TempDirTestCase):
  """Tests for the ImportModule function."""

  def testMissingModule(self):
    """Check behavior on unknown modules."""
    self.assertRaises(ImportError, cros_import.ImportModule, 'asdf.aja.ew.q.a')

  def _testImportModule(self, target):
    """Verify we can import |target| successfully."""
    module = cros_import.ImportModule(target)
    self.assertTrue(hasattr(module, 'ImportModule'))

  def testImportString(self):
    """Verify we can import using a string."""
    self._testImportModule('chromite.lib.cros_import')

  def testImportTupleList(self):
    """Verify we can import using a tuple & list."""
    parts = ('chromite', 'lib', 'cros_import')
    self._testImportModule(parts)
    self._testImportModule(list(parts))

  def testImportGenerator(self):
    """Verify we can import using a generator."""
    def target():
      for p in ('chromite', 'lib', 'cros_import'):
        yield p
    self._testImportModule(target())
