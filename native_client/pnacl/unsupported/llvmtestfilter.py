#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Clean-up LLVM's "make check" results for easier diffing."""

import re
import sys


def FilterLLVMTestErrors(file):
  start = False
  for line in file:
    if re.search('^make[\[\]\d]*:', line):
      continue
    if not start:
      if ('Unexpected Passing Tests' in line or
          'Failing Tests' in line):
        start = True
    if start:
      sys.stdout.write(line)


if __name__ == '__main__':
  if len(sys.argv) < 2:
    fileobj = sys.stdin
  else:
    fileobj = open(sys.argv[1])

  FilterLLVMTestErrors(fileobj)

