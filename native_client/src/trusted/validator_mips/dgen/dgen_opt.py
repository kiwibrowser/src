#!/usr/bin/python
#
# Copyright 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# Copyright 2012, Google Inc.
#

"""
Table minimization algorithm.
"""

def optimize_rows(rows):
    """Breaks rows up into batches, and attempts to minimize each batch,
    using _optimize_rows_for_single_action.
    """
    rows_by_action = dict()
    for row in rows:
        if (row.action, row.arch) in rows_by_action:
            rows_by_action[(row.action, row.arch)].append(row)
        else:
            rows_by_action[(row.action, row.arch)] = [row]

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
    num_cols = len(rows[0].patterns)
    used = [False] * num_cols

    for r in rows:
        for i in range(0, num_cols):
            if r.patterns[i].mask != 0:
                used[i] = True

    if not True in used:
        # Always preserve at least one column
        used[0] = True

    for col in range(num_cols - 1, 0 - 1, -1):
        for r in rows:
            if not used[col]:
                del r.patterns[col]


def each_index_pair(sequence):
    """Utility function: Generates each unique index pair in sequence."""
    for i in range(0, len(sequence)):
        for j in range(i + 1, len(sequence)):
            yield (i, j)
