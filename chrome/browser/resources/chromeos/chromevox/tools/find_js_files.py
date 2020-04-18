#!/usr/bin/env python

# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''Scans one or more directory trees for .js files, printing filenames,
relative to the current directory on stdout.
'''

import optparse
import os
import sys

_SCRIPT_DIR = os.path.realpath(os.path.dirname(__file__))
_CHROME_SOURCE = os.path.realpath(
    os.path.join(_SCRIPT_DIR, *[os.path.pardir] * 6))
sys.path.insert(
    0, os.path.join(
        _CHROME_SOURCE, ('chrome/third_party/chromevox/third_party/' +
                         'closure-library/closure/bin/build')))
import treescan


def main():
  parser = optparse.OptionParser(description=__doc__)
  parser.usage = '%prog <tree_root>...'
  _, args = parser.parse_args()
  for root in args:
    print '\n'.join(treescan.ScanTreeForJsFiles(root))


if __name__ == '__main__':
  main()
