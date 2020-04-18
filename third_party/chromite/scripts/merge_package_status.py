# -*- coding: utf-8 -*-
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Merge multiple package status CSV files into one csv file.

This simplifies uploading to a Google Docs spreadsheet.
"""

from __future__ import print_function

import os
import re

from chromite.lib import commandline
from chromite.lib import operation
from chromite.lib import table
from chromite.lib import upgrade_table as utable

COL_PACKAGE = utable.UpgradeTable.COL_PACKAGE
COL_SLOT = utable.UpgradeTable.COL_SLOT
COL_TARGET = utable.UpgradeTable.COL_TARGET
COL_OVERLAY = utable.UpgradeTable.COL_OVERLAY
ID_COLS = [COL_PACKAGE, COL_SLOT]

oper = operation.Operation('merge_package_status')

# A bit of hard-coding with knowledge of how cros targets work.
CHROMEOS_TARGET_ORDER = [
    'virtual/target-os',
    'virtual/target-os-dev',
    'virtual/target-os-test',
]


def _GetCrosTargetRank(target):
  """Hard-coded ranking of known/expected OS root targets for sorting.

  The lower the ranking, the earlier in the target list it falls by
  convention.  In other words, in the typical target combination
  "virtual/target-os virtual/target-os-dev", "virtual/target-os" has
  a lower ranking than "virtual/target-os-dev".

  All valid rankings are greater than zero.

  Returns:
    Valid ranking for target or a false value if target is unrecognized.
  """
  for ix, targ in enumerate(CHROMEOS_TARGET_ORDER):
    if target == targ:
      return ix + 1  # Avoid a 0 (non-true) result
  return None


def ProcessTargets(targets, reverse_cros=False):
  """Process a list of |targets| to smaller, sorted list.

  For example:
  virtual/target-os virtual/target-os-dev -> virtual/target-os-dev
  virtual/target-os virtual/target-os-dev world -> virtual/target-os-dev world
  world virtual/target-sdk -> virtual/target-sdk world

  The one virtual/target-os target always comes back first, with targets
  otherwise sorted alphabetically.  The virtual/target-os target that is
  kept will be the one with the highest 'ranking', as decided
  by _GetCrosTargetRank.  To reverse the ranking sense, specify
  |reverse_cros| as True.

  These rules are specific to how we want the information to appear
  in the final spreadsheet.
  """
  if targets:
    # Sort cros targets according to "rank".
    cros_targets = [t for t in targets if _GetCrosTargetRank(t)]
    cros_targets.sort(key=_GetCrosTargetRank, reverse=reverse_cros)

    # Don't condense non-cros targets.
    other_targets = [t for t in targets if not _GetCrosTargetRank(t)]
    other_targets.sort()

    # Assemble final target list, with single cros target first.
    final_targets = []
    if cros_targets:
      final_targets.append(cros_targets[-1])
    if other_targets:
      final_targets.extend(other_targets)

    return final_targets


def LoadTable(filepath):
  """Load the csv file at |filepath| into a table.Table object."""
  table_name = os.path.basename(filepath)
  if table_name.endswith('.csv'):
    table_name = table_name[:-4]
  return table.Table.LoadFromCSV(filepath, name=table_name)


def MergeTables(tables):
  """Merge all |tables| into one merged table.  Return table."""
  def TargetMerger(_col, val, other_val):
    """Function to merge two values in Root Target column from two tables."""
    targets = []
    if val:
      targets.extend(val.split())
    if other_val:
      targets.extend(other_val.split())

    processed_targets = ProcessTargets(targets, reverse_cros=True)
    return ' '.join(processed_targets)

  def DefaultMerger(col, val, other_val):
    """Merge |val| and |other_val| in column |col| for some row."""
    # This function is registered as the default merge function,
    # so verify that the column is a supported one.
    prfx = utable.UpgradeTable.COL_DEPENDS_ON.replace('ARCH', '')
    if col.startswith(prfx):
      # Merge dependencies by taking the superset.
      return MergeToSuperset(col, val, other_val)

    prfx = utable.UpgradeTable.COL_USED_BY.replace('ARCH', '')
    if col.startswith(prfx):
      # Merge users by taking the superset.
      return MergeToSuperset(col, val, other_val)

    regexp = utable.UpgradeTable.COL_UPGRADED.replace('ARCH', r'\S+')
    if re.search(regexp, col):
      return MergeWithAND(col, val, other_val)

    # For any column, if one value is missing just accept the other value.
    # For example, when one table has an entry for 'arm version' but
    # the other table does not.
    if val == table.Table.EMPTY_CELL and other_val != table.Table.EMPTY_CELL:
      return other_val
    if other_val == table.Table.EMPTY_CELL and val != table.Table.EMPTY_CELL:
      return val

    # Raise a generic ValueError, which MergeTable function will clarify.
    # The effect should be the same as having no merge_rule for this column.
    raise ValueError

  def MergeToSuperset(_col, val, other_val):
    """Merge |col| values as superset of tokens in |val| and |other_val|."""
    tokens = set(val.split())
    other_tokens = set(other_val.split())
    all_tokens = tokens.union(other_tokens)
    return ' '.join(sorted(tok for tok in all_tokens))

  # This is only needed because the automake-wrapper package is coming from
  # different overlays for different boards right now!
  def MergeWithAND(_col, val, other_val):
    """For merging columns that might have differences but should not!."""
    if not val:
      return '"" AND ' + other_val
    if not other_val + ' AND ""':
      return val
    return val + ' AND ' + other_val

  # Prepare merge_rules with the defined functions.
  merge_rules = {COL_TARGET: TargetMerger,
                 COL_OVERLAY: MergeWithAND,
                 '__DEFAULT__': DefaultMerger}

  # Merge each table one by one.
  csv_table = tables[0]
  if len(tables) > 1:
    oper.Notice('Merging tables into one.')
    for tmp_table in tables[1:]:
      oper.Notice('Merging "%s" and "%s".' %
                  (csv_table.GetName(), tmp_table.GetName()))
      csv_table.MergeTable(tmp_table, ID_COLS,
                           merge_rules=merge_rules, allow_new_columns=True)

  # Sort the table by package name, then slot.
  def IdSort(row):
    return tuple(row[col] for col in ID_COLS)
  csv_table.Sort(IdSort)

  return csv_table


def LoadAndMergeTables(args):
  """Load all csv files in |args| into one merged table.  Return table."""
  tables = []
  for arg in args:
    oper.Notice('Loading csv table from "%s".' % arg)
    tables.append(LoadTable(arg))

  return MergeTables(tables)


# Used by upload_package_status.
def FinalizeTable(csv_table):
  """Process the table to prepare it for upload to online spreadsheet."""
  oper.Notice('Processing final table to prepare it for upload.')

  col_ver = utable.UpgradeTable.COL_CURRENT_VER
  col_arm_ver = utable.UpgradeTable.GetColumnName(col_ver, 'arm')
  col_x86_ver = utable.UpgradeTable.GetColumnName(col_ver, 'x86')

  # Insert new columns
  col_cros_target = 'ChromeOS Root Target'
  col_host_target = 'Host Root Target'
  col_cmp_arch = 'Comparing arm vs x86 Versions'
  csv_table.AppendColumn(col_cros_target)
  csv_table.AppendColumn(col_host_target)
  csv_table.AppendColumn(col_cmp_arch)

  # Row by row processing
  for row in csv_table:
    # If the row is not unique when just the package
    # name is considered, then add a ':<slot>' suffix to the package name.
    id_values = {COL_PACKAGE: row[COL_PACKAGE]}
    matching_rows = csv_table.GetRowsByValue(id_values)
    if len(matching_rows) > 1:
      for mr in matching_rows:
        mr[COL_PACKAGE] += ':' + mr[COL_SLOT]

    # Split target column into cros_target and host_target columns
    target_str = row.get(COL_TARGET, None)
    if target_str:
      targets = target_str.split()
      cros_targets = []
      host_targets = []
      for target in targets:
        if _GetCrosTargetRank(target):
          cros_targets.append(target)
        else:
          host_targets.append(target)

      row[col_cros_target] = ' '.join(cros_targets)
      row[col_host_target] = ' '.join(host_targets)

    # Compare x86 vs. arm version, add result to col_cmp_arch.
    x86_ver = row.get(col_x86_ver)
    arm_ver = row.get(col_arm_ver)
    if x86_ver and arm_ver:
      if x86_ver != arm_ver:
        row[col_cmp_arch] = 'different'
      else:
        row[col_cmp_arch] = 'same'


def WriteTable(csv_table, outpath):
  """Write |csv_table| out to |outpath| as csv."""
  try:
    fh = open(outpath, 'w')
    csv_table.WriteCSV(fh)
    oper.Notice('Wrote merged table to "%s"' % outpath)
  except IOError as ex:
    oper.Error('Unable to open %s for write: %s' % (outpath, ex))
    raise


def GetParser():
  """Return a command line parser."""
  parser = commandline.ArgumentParser(description=__doc__)
  parser.add_argument('--out', dest='outpath', type='path',
                      required=True,
                      help='File to write merged results to')
  parser.add_argument('input_csv_files', nargs='+')
  return parser


def main(argv):
  """Main function."""
  parser = GetParser()
  options = parser.parse_args(argv)

  csv_table = LoadAndMergeTables(options.input_csv_files)

  WriteTable(csv_table, options.outpath)
