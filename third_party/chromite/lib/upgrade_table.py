# -*- coding: utf-8 -*-
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""UpgradeTable class is used in Portage package upgrade process."""

from __future__ import print_function

from chromite.lib import table


class UpgradeTable(table.Table):
  """Class to represent upgrade data in memory, can be written to csv."""

  # Column names.  Note that 'ARCH' is replaced with a real arch name when
  # these are accessed as attributes off an UpgradeTable object.
  COL_PACKAGE = 'Package'
  COL_SLOT = 'Slot'
  COL_OVERLAY = 'Overlay'
  COL_CURRENT_VER = 'Current ARCH Version'
  COL_STABLE_UPSTREAM_VER = 'Stable Upstream ARCH Version'
  COL_LATEST_UPSTREAM_VER = 'Latest Upstream ARCH Version'
  COL_STATE = 'State On ARCH'
  COL_DEPENDS_ON = 'Dependencies On ARCH'
  COL_USED_BY = 'Required By On ARCH'
  COL_TARGET = 'Root Target'
  COL_UPGRADED = 'Upgraded ARCH Version'

  # COL_STATE values should be one of the following:
  STATE_UNKNOWN = 'unknown'
  STATE_LOCAL_ONLY = 'local only'
  STATE_UPSTREAM_ONLY = 'upstream only'
  STATE_NEEDS_UPGRADE = 'needs upgrade'
  STATE_PATCHED = 'patched locally'
  STATE_DUPLICATED = 'duplicated locally'
  STATE_NEEDS_UPGRADE_AND_PATCHED = 'needs upgrade and patched locally'
  STATE_NEEDS_UPGRADE_AND_DUPLICATED = 'needs upgrade and duplicated locally'
  STATE_CURRENT = 'current'

  @staticmethod
  def GetColumnName(col, arch=None):
    """Translate from generic column name to specific given |arch|."""
    if arch:
      return col.replace('ARCH', arch)
    return col

  def __init__(self, arch, upgrade=False, name=None):
    self._arch = arch

    # These constants serve two roles, for csv output:
    # 1) Restrict which column names are valid.
    # 2) Specify the order of those columns.
    columns = [
        self.COL_PACKAGE,
        self.COL_SLOT,
        self.COL_OVERLAY,
        self.COL_CURRENT_VER,
        self.COL_STABLE_UPSTREAM_VER,
        self.COL_LATEST_UPSTREAM_VER,
        self.COL_STATE,
        self.COL_DEPENDS_ON,
        self.COL_USED_BY,
        self.COL_TARGET,
    ]

    if upgrade:
      columns.append(self.COL_UPGRADED)

    table.Table.__init__(self, columns, name=name)

  def __getattribute__(self, name):
    """When accessing self.COL_*, substitute ARCH name."""
    if name.startswith('COL_'):
      text = getattr(UpgradeTable, name)
      return UpgradeTable.GetColumnName(text, arch=self._arch)
    else:
      return object.__getattribute__(self, name)

  def GetArch(self):
    """Get the architecture associated with this UpgradeTable."""
    return self._arch
