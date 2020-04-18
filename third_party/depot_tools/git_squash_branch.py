#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import sys

import git_common

def main(args):
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '-m', '--message', metavar='<msg>', default=None,
      help='Use the given <msg> as the first line of the commit message.')
  opts = parser.parse_args(args)
  if git_common.is_dirty_git_tree('squash-branch'):
    return 1
  git_common.squash_current_branch(opts.message)
  return 0


if __name__ == '__main__':
  try:
    sys.exit(main(sys.argv[1:]))
  except KeyboardInterrupt:
    sys.stderr.write('interrupted\n')
    sys.exit(1)
