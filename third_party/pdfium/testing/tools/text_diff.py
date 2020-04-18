#!/usr/bin/env python
# Copyright 2015 The PDFium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import difflib
import sys

def main(argv):
  if len(argv) != 3:
     print '%s: invalid arguments' % argv[0]
     return 2
  filename1 = argv[1]
  filename2 = argv[2]
  try:
    with open(filename1, "r") as f1:
      str1 = f1.readlines();
    with open(filename2, "r") as f2:
      str2 = f2.readlines();
    diffs = difflib.unified_diff(
        str1, str2, fromfile=filename1, tofile=filename2)
  except Exception as e:
    print "something went astray: %s" % e
    return 1
  status_code = 0
  for diff in diffs:
    sys.stdout.write(diff)
    status_code = 1
  return status_code

if __name__ == '__main__':
  sys.exit(main(sys.argv))
