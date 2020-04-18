#!/usr/bin/python
#
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

"""
Table minimization algorithm.
"""

import dgen_core

def optimize_rows(rows):
    """Breaks rows up into batches, and attempts to minimize each batch,
    using _optimize_rows_for_single_action.
    """
    rows_by_action = dict()
    for row in rows:
        if row.action in rows_by_action:
            rows_by_action[row.action].append(row)
        else:
            rows_by_action[row.action] = [row]

    optimized_rows = []
    for row_group in rows_by_action.itervalues():
        optimized_rows.extend(_optimize_rows_for_single_action(row_group))

    _remove_unused_columns(optimized_rows)
    return optimized_rows

def _optimize_rows_for_single_action(rows):
    """Performs basic automatic minimization on the given rows.

    Repeatedly selects a pair of rows to merge.  Recurses until no suitable pair
    can be found.  It's not real smart, and is O(n^2).

    A pair of rows is compatible if all columns are equal, or if exactly one
    row differs but is_strictly_compatible.
    """
    for (i, j) in each_index_pair(rows):
        row_i, row_j = rows[i], rows[j]

        if row_i.can_merge(row_j):
            new_rows = list(rows)
            del new_rows[j]
            del new_rows[i]
            new_rows.append(row_i + row_j)
            return _optimize_rows_for_single_action(new_rows)

    # No changes made:
    return rows

def _remove_unused_columns(rows):
    for r in rows:
      # Remove true entries form row.
      if not r.patterns:
        continue
      simp_patterns = []
      for p in r.patterns:
        if p.mask != 0:  # i.e. test if not true.
          simp_patterns.append(p)
      if not simp_patterns:
        # Stick in true, so row is always non-empty
        simp_patterns.append(dgen_core.BitPattern(0, 0, '=='))
      r.patterns = simp_patterns

def each_index_pair(sequence):
    """Utility function: Generates each unique index pair in sequence."""
    for i in range(0, len(sequence)):
        for j in range(i + 1, len(sequence)):
            yield (i, j)
