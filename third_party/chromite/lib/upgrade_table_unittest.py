# -*- coding: utf-8 -*-
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for the upgrade_table module."""

from __future__ import print_function

from chromite.lib import cros_test_lib
from chromite.lib import upgrade_table as utable


class UpgradeTableTest(cros_test_lib.TestCase):
  """Unittests for UpgradeTable."""
  ARCH = 'some-arch'
  NAME = 'some-name'

  def _CreateTable(self, upgrade_mode, arch=ARCH, name=NAME):
    return utable.UpgradeTable(arch, upgrade=upgrade_mode, name=name)

  def testGetArch(self):
    t1 = self._CreateTable(True, arch='arch1')
    self.assertEquals(t1.GetArch(), 'arch1')
    t2 = self._CreateTable(False, arch='arch2')
    self.assertEquals(t2.GetArch(), 'arch2')

  def _AssertEqualsAfterArchSub(self, arch, table_col_name,
                                static_table_col_name):
    self.assertEquals(table_col_name,
                      static_table_col_name.replace('ARCH', arch))

  def testColumnNameArchSubstitute(self):
    arch = 'foobar'
    t1 = self._CreateTable(True, arch=arch)

    # Some column names are independent of ARCH.
    self.assertEquals(t1.COL_PACKAGE, utable.UpgradeTable.COL_PACKAGE)
    self.assertEquals(t1.COL_SLOT, utable.UpgradeTable.COL_SLOT)
    self.assertEquals(t1.COL_OVERLAY, utable.UpgradeTable.COL_OVERLAY)
    self.assertEquals(t1.COL_TARGET, utable.UpgradeTable.COL_TARGET)

    # Other column names require ARCH substitution.
    self._AssertEqualsAfterArchSub(arch, t1.COL_CURRENT_VER,
                                   utable.UpgradeTable.COL_CURRENT_VER)
    self._AssertEqualsAfterArchSub(arch, t1.COL_STABLE_UPSTREAM_VER,
                                   utable.UpgradeTable.COL_STABLE_UPSTREAM_VER)
    self._AssertEqualsAfterArchSub(arch, t1.COL_LATEST_UPSTREAM_VER,
                                   utable.UpgradeTable.COL_LATEST_UPSTREAM_VER)
    self._AssertEqualsAfterArchSub(arch, t1.COL_STATE,
                                   utable.UpgradeTable.COL_STATE)
    self._AssertEqualsAfterArchSub(arch, t1.COL_DEPENDS_ON,
                                   utable.UpgradeTable.COL_DEPENDS_ON)
    self._AssertEqualsAfterArchSub(arch, t1.COL_USED_BY,
                                   utable.UpgradeTable.COL_USED_BY)
    self._AssertEqualsAfterArchSub(arch, t1.COL_UPGRADED,
                                   utable.UpgradeTable.COL_UPGRADED)

  def testColumnExistence(self):
    t1 = self._CreateTable(False)
    t2 = self._CreateTable(True)

    # All these columns should be in both tables, with same name.
    cols = [
        t1.COL_PACKAGE,
        t1.COL_SLOT,
        t1.COL_OVERLAY,
        t1.COL_CURRENT_VER,
        t1.COL_STABLE_UPSTREAM_VER,
        t1.COL_LATEST_UPSTREAM_VER,
        t1.COL_STATE,
        t1.COL_DEPENDS_ON,
        t1.COL_USED_BY,
        t1.COL_TARGET,
    ]

    for col in cols:
      self.assertTrue(t1.HasColumn(col))
      self.assertTrue(t2.HasColumn(col))

    # The UPGRADED column should only be in the table with upgrade_mode=True.
    col = t1.COL_UPGRADED
    self.assertFalse(t1.HasColumn(col))
    self.assertTrue(t2.HasColumn(col))
