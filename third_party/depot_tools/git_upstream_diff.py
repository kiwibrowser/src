#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import sys

import subprocess2

import git_common as git

def main(args):
  default_args = git.get_config_list('depot-tools.upstream-diff.default-args')
  args = default_args + args

  current_branch = git.current_branch()

  parser = argparse.ArgumentParser()
  parser.add_argument('--wordwise', action='store_true', default=False,
                      help=(
                        'Print a colorized wordwise diff '
                        'instead of line-wise diff'))
  parser.add_argument('branch', nargs='?', default=current_branch,
                      help='Show changes from a different branch')
  opts, extra_args = parser.parse_known_args(args)

  if not opts.branch or opts.branch == 'HEAD':
    print 'fatal: Cannot perform git-upstream-diff while not on a branch'
    return 1

  par = git.upstream(opts.branch)
  if not par:
    print 'fatal: No upstream configured for branch \'%s\'' % opts.branch
    return 1

  cmd = [git.GIT_EXE, '-c', 'core.quotePath=false',
         'diff', '--patience', '-C', '-C']
  if opts.wordwise:
    cmd += ['--word-diff=color', r'--word-diff-regex=(\w+|[^[:space:]])']
  cmd += [git.get_or_create_merge_base(opts.branch, par)]
  # Only specify the end commit if it is not the current branch, this lets the
  # diff include uncommitted changes when diffing the current branch.
  if opts.branch != current_branch:
    cmd += [opts.branch]

  cmd += extra_args

  return subprocess2.check_call(cmd)


if __name__ == '__main__':
  try:
    sys.exit(main(sys.argv[1:]))
  except KeyboardInterrupt:
    sys.stderr.write('interrupted\n')
    sys.exit(1)
