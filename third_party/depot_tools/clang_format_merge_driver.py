#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""clang-format 3-way merge driver.

This is a custom merge driver for git that helps automatically resolves
conflicts caused by clang-format changes. The conflict resolution
strategy is extremely simple: it simply clang-formats the current,
ancestor branch's, and other branch's version of the file and delegates
the remaining work to git merge-file.

See https://git-scm.com/docs/gitattributes ("Defining a custom merge
driver") for more details.
"""

import subprocess
import sys

import clang_format


def main():
  if len(sys.argv) < 5:
    print('usage: %s <base> <current> <others> <path in the tree>' %
          sys.argv[0])
    sys.exit(1)

  base, current, others, file_name_in_tree = sys.argv[1:5]

  if file_name_in_tree == '%P':
    print >>sys.stderr
    print >>sys.stderr, 'ERROR: clang-format merge driver needs git 2.5+'
    if sys.platform == 'darwin':
      print >>sys.stderr, 'Upgrade to Xcode 7.2+'
    print >>sys.stderr
    return 1

  print 'Running clang-format 3-way merge driver on ' + file_name_in_tree

  try:
    tool = clang_format.FindClangFormatToolInChromiumTree()
    for fpath in base, current, others:
      # Typically, clang-format is used with the -i option to rewrite files
      # in-place. However, merge files live in the repo root, so --style=file
      # will always pick up the root .clang-format.
      #
      # Instead, this tool uses --assume-filename so clang-format will pick up
      # the appropriate .clang-format. Unfortunately, --assume-filename only
      # works when the input is from stdin, so the file I/O portions are lifted
      # up into the script as well.
      with open(fpath, 'rb') as input_file:
        output = subprocess.check_output(
            [tool, '--assume-filename=%s' % file_name_in_tree, '--style=file'],
            stdin=input_file)
      with open(fpath, 'wb') as output_file:
        output_file.write(output)
  except clang_format.NotFoundError, e:
    print e
    print 'Failed to find clang-format. Falling-back on standard 3-way merge'

  return subprocess.call(['git', 'merge-file', '-Lcurrent', '-Lbase', '-Lother',
                          current, base, others])


if __name__ == '__main__':
  sys.exit(main())
