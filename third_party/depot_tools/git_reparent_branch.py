#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Change the upstream of the current branch."""

import argparse
import sys

import subprocess2

from git_common import upstream, current_branch, run, tags, set_branch_config
from git_common import get_or_create_merge_base, root, manual_merge_base

import git_rebase_update

def main(args):
  root_ref = root()

  parser = argparse.ArgumentParser()
  g = parser.add_mutually_exclusive_group()
  g.add_argument('new_parent', nargs='?',
                 help='New parent branch (or tag) to reparent to.')
  g.add_argument('--root', action='store_true',
                 help='Reparent to the configured root branch (%s).' % root_ref)
  g.add_argument('--lkgr', action='store_true',
                 help='Reparent to the lkgr tag.')
  opts = parser.parse_args(args)

  # TODO(iannucci): Allow specification of the branch-to-reparent

  branch = current_branch()

  if opts.root:
    new_parent = root_ref
  elif opts.lkgr:
    new_parent = 'lkgr'
  else:
    if not opts.new_parent:
      parser.error('Must specify new parent somehow')
    new_parent = opts.new_parent
  cur_parent = upstream(branch)

  if branch == 'HEAD' or not branch:
    parser.error('Must be on the branch you want to reparent')
  if new_parent == cur_parent:
    parser.error('Cannot reparent a branch to its existing parent')

  if not cur_parent:
    msg = (
      "Unable to determine %s@{upstream}.\n\nThis can happen if you didn't use "
      "`git new-branch` to create the branch and haven't used "
      "`git branch --set-upstream-to` to assign it one.\n\nPlease assign an "
      "upstream branch and then run this command again."
    )
    print >> sys.stderr, msg % branch
    return 1

  mbase = get_or_create_merge_base(branch, cur_parent)

  all_tags = tags()
  if cur_parent in all_tags:
    cur_parent += ' [tag]'

  try:
    run('show-ref', new_parent)
  except subprocess2.CalledProcessError:
    print >> sys.stderr, 'fatal: invalid reference: %s' % new_parent
    return 1

  if new_parent in all_tags:
    print ("Reparenting %s to track %s [tag] (was %s)"
           % (branch, new_parent, cur_parent))
    set_branch_config(branch, 'remote', '.')
    set_branch_config(branch, 'merge', new_parent)
  else:
    print ("Reparenting %s to track %s (was %s)"
           % (branch, new_parent, cur_parent))
    run('branch', '--set-upstream-to', new_parent, branch)

  manual_merge_base(branch, mbase, new_parent)

  # TODO(iannucci): ONLY rebase-update the branch which moved (and dependants)
  return git_rebase_update.main(['--no-fetch'])


if __name__ == '__main__':  # pragma: no cover
  try:
    sys.exit(main(sys.argv[1:]))
  except KeyboardInterrupt:
    sys.stderr.write('interrupted\n')
    sys.exit(1)
