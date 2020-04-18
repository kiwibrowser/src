#!/usr/bin/python
# Copyright 2009 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""Simple test comparing two files.
"""

# python imports
import filecmp
import os.path
import sys

# local imports
import command_tester

def main(argv):
  if (len(argv) != 2):
    return -1
  command_tester.GlobalSettings['name'] = os.path.split(argv[0])[1]
  if filecmp.cmp(argv[0], argv[1]):
    command_tester.Print(command_tester.SuccessMessage(0))  # bogus time
    return 0
  else:
    def dump_file_contents(fn):
      print 'File %s' % fn
      print 70 * '-'
      h = None
      try:
        h = open(fn)
        print h.read()
      finally:
        if h is not None:
          h.close()
      print 70 * '-'
    dump_file_contents(argv[0])
    dump_file_contents(argv[1])
    command_tester.Print(command_tester.FailureMessage(0))  # bogus time
    return -1

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
