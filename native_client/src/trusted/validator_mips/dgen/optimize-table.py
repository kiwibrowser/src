#!/usr/bin/python
#
# Copyright 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# Copyright 2012, Google Inc.
#

"""Table optimizer script.

Usage: optimize_table.py <input-table> <output-table>
"""

import sys
import dgen_input
import dgen_dump

def main(argv):
    in_filename, out_filename = argv[1], argv[2]

    print "Optimizer reading ", in_filename
    f = open(in_filename, 'r')
    tables = dgen_input.parse_tables(f)
    f.close()

    print "Successful - got %d tables." % len(tables)

    print "Generating output to %s..." % out_filename
    f = open(out_filename, 'w')
    dgen_dump.dump_tables(tables, f)
    f.close()
    print "Completed."

    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv))
