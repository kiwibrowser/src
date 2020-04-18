#!/usr/bin/python
#
# Copyright 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# Copyright 2012, Google Inc.
#

"""
Produces a table from the in-memory representation.  Useful for storing the
optimized table for later use.
"""

import dgen_opt

def dump_tables(tables, out):
    """Dumps the given tables into a text file.

    Args:
        tables: list of Table objects to process.
        out: an output stream.
    """
    if len(tables) == 0: raise Exception('No tables provided.')

    _generate_header(out)
    for t in tables:
        _generate_table(t, out)

def _generate_header(out):
    # TODO do we need a big ridiculous license banner in generated code?
    out.write('# DO NOT EDIT: GENERATED CODE\n')


def _generate_table(t, out):
    rows = dgen_opt.optimize_rows(t.rows)
    print ('Table %s: %d rows minimized to %d.'
        % (t.name, len(t.rows), len(rows)))

    out.write('\n')
    out.write('-- %s (%s)\n' % (t.name, t.citation))
    num_cols = len(rows[0].patterns)
    headers = ['pat%- 31s' % (str(n) + '(31:0)') for n in range(0, num_cols)]
    out.write(''.join(headers))
    out.write('\n')
    for row in rows:
        out.write(''.join(['%- 34s' % p for p in row.patterns]))
        out.write(row.action)
        if row.arch:
            out.write('(%s)' % row.arch)
        out.write('\n')
