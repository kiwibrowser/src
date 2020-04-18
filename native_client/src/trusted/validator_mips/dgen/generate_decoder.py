#!/usr/bin/python
#
# Copyright 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# Copyright 2012, Google Inc.
#

"""Decoder Generator script.

Usage: generate-decoder.py <table-file> <output-cc-file>
"""

# TODO(petarj): This was copied from the old ARM decoder tablegen.
# Once the changes on that tablegen are stable, this will be rewritten to use
# the new tablegen.

import sys
import dgen_input
import dgen_output

def main(argv):
    table_filename, output_filename = argv[1], argv[2]

    print "Decoder Generator reading ", table_filename
    f = open(table_filename, 'r')
    tables = dgen_input.parse_tables(f)
    f.close()

    print "Successful - got %d tables." % len(tables)

    print "Generating output to %s..." % output_filename
    f = open(output_filename, 'w')
    dgen_output.generate_decoder(tables,
        dgen_output.COutput(f))
    f.close()
    print "Completed."

    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv))
