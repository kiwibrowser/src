#!/usr/bin/env python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This tool helps with updating Git commit IDs in the
pnacl/COMPONENT_REVISIONS file to the latest commits.  It creates a
Rietveld code review for the update, listing the new Git commits.

This tool should be run from a Git checkout of native_client.
"""

import argparse
import os
import re
import subprocess
import sys


def MatchKey(data, key):
  # Search for "key=value" line in the COMPONENT_REVISIONS file.
  # Also, the keys have underscores instead of dashes.
  key = key.replace('-', '_')
  match = re.search('^%s=(\S+)?$' % key, data, re.M)
  if match is None:
    raise Exception('Key %r not found' % key)
  return match


def GetDepsField(data, key):
  match = MatchKey(data, key)
  return match.group(1)


def SetDepsField(data, key, value):
  match = MatchKey(data, key)
  return ''.join([data[:match.start(1)],
                  value,
                  data[match.end(1):]])


# Returns the Git commit ID for the latest revision on the given
# branch in the given Git repository.
def GetNewRev(git_dir, branch):
  subprocess.check_call(['git', 'fetch'], cwd=git_dir)
  output = subprocess.check_output(['git', 'rev-parse', branch], cwd=git_dir)
  return output.strip()


# Extracts some information about new Git commits.  Returns a tuple
# containing:
#  * log: list of strings summarising the Git commits
#  * authors: list of author e-mail addresses (each a string)
#  * bugs: list of "BUG=..." strings to put in a commit message
def GetLog(git_dir, new_rev, old_revs):
  log_args = [new_rev] + ['^' + rev for rev in old_revs]
  log_data = subprocess.check_output(
      ['git', 'log', '--pretty=format:%h: (%ae) %s'] + log_args, cwd=git_dir)
  authors_data = subprocess.check_output(
      ['git', 'log', '--pretty=%ae'] + log_args, cwd=git_dir)
  full_log = subprocess.check_output(
      ['git', 'log', '--pretty=%B'] + log_args, cwd=git_dir)
  log = [line + '\n' for line in reversed(log_data.strip().split('\n'))]
  authors = authors_data.strip().split('\n')
  bugs = []
  for line in reversed(full_log.split('\n')):
    if line.startswith('BUG='):
      bug = line[4:].strip()
      bug_line = 'BUG= %s\n' % bug
      if bug.lower() != 'none' and len(bug):
        bugs.append(bug_line)
  return log, authors, bugs


def AssertNoUncommittedChanges():
  # Check for uncommitted changes.  Note that this can still lose
  # changes that have been committed to a detached-HEAD branch, but
  # those should be retrievable via the reflog.  This can also lose
  # changes that have been staged to the index but then undone in the
  # working files.
  changes = subprocess.check_output(['git', 'diff', '--name-only', 'HEAD'])
  if len(changes) != 0:
    raise AssertionError('You have uncommitted changes:\n%s' % changes)


def UpdateComponent(src_base, deps_data, component, revision):
  component_name_map = {'llvm': 'LLVM',
                        'clang': 'Clang',
                        'pnacl-gcc': 'GCC',
                        'binutils': 'Binutils',
                        'libcxx': 'libc++',
                        'libcxxabi': 'libc++abi',
                        'llvm-test-suite': 'LLVM test suite',
                        'pnacl-newlib': 'Newlib',
                        'compiler-rt': 'compiler-rt',
                        'subzero': 'Subzero'}

  git_dir = os.path.join(src_base, component)
  component_name = component_name_map.get(component, component)
  if component == 'pnacl-gcc':
    pnacl_branch = 'origin/pnacl'
    upstream_branches = []
  elif component == 'binutils':
    pnacl_branch = 'origin/pnacl/2.25/master'
    upstream_branches = ['origin/ng/2.25/master']
  elif component == 'subzero':
    pnacl_branch = 'origin/master'
    upstream_branches = []
  else:
    pnacl_branch = 'origin/master'
    # Skip changes merged (but not cherry-picked) from upstream git.
    upstream_branches = ['origin/upstream/master']

  new_rev = revision
  if new_rev is None:
    new_rev = GetNewRev(git_dir, pnacl_branch)

  old_rev = GetDepsField(deps_data, component)
  if new_rev == old_rev:
    raise AssertionError('No new changes!')
  deps_data = SetDepsField(deps_data, component, new_rev)

  msg_logs, authors, bugs = GetLog(git_dir, new_rev,
                                   [old_rev] + upstream_branches)

  msg = '\n\nThis pulls in the following %s %s:\n\n' % (
      component_name,
      {True: 'change', False: 'changes'}[len(msg_logs) == 1])
  msg += ''.join(msg_logs)
  msg += '\n'

  return msg, bugs, authors, deps_data


def Main(args):
  src_base = 'toolchain_build/src'
  parser = argparse.ArgumentParser(description=__doc__.strip())
  parser.add_argument('--list', action='store_true', default=False,
                      dest='list_only',
                      help='Only list the new Git revisions to be pulled in')
  parser.add_argument('-c', '--component', action='append',
                      help='Subdirectory of {src_dir} to update '
                      'COMPONENT_REVISIONS from (defaults to "llvm"). '
                      'Can be specified multiple times.'.format(
                        src_dir=src_base))
  parser.add_argument('-r', '--revision', action='append',
                      help='Git revision to use. Defaults to head of origin. '
                      'If given, must be given exactly once per component.')
  parser.add_argument('-u', '--no-upload', action='store_true', default=False,
                      help='Do not run "git cl upload"')
  options = parser.parse_args(args)
  components = options.component if options.component is not None else ['llvm']
  revisions = (options.revision if options.revision is not None
               else [None] * len(components))
  if len(components) != len(revisions):
    parser.error('Number of revision arguments must match number of '
                 'component arguments')

  if not options.list_only:
    AssertNoUncommittedChanges()
  subprocess.check_call(['git', 'fetch'])

  deps_file = 'pnacl/COMPONENT_REVISIONS'
  deps_data = subprocess.check_output(['git', 'cat-file', 'blob',
                                       'origin/master:%s' % deps_file])

  component_names = ' and '.join(components)
  msg = 'PNaCl: Update %s revision in %s' % (component_names, deps_file)
  bugs = []
  authors = []

  for component, revision in zip(components, revisions):
    m, b, a, d = UpdateComponent(src_base, deps_data, component, revision)
    msg += m
    bugs.extend(b)
    authors.extend(a)
    deps_data = d

  if len(bugs) == 0:
    bugs.append('BUG=none\n')
  msg += ''.join(set(bugs))
  msg += 'TEST= PNaCl toolchain trybots\n'
  print msg
  cc_list = ', '.join(sorted(set(authors)))
  print 'CC:', cc_list

  if options.list_only:
    return
  subprocess.check_call(['git', 'checkout', 'origin/master'])
  branch_name = '%s-deps-%s' % (components[0],
                                GetDepsField(deps_data, components[0])[:8])
  subprocess.check_call(['git', 'checkout', '-b', branch_name, 'origin/master'])
  with open(deps_file, 'w') as fh:
    fh.write(deps_data)
  subprocess.check_call(['git', 'commit', '-a', '-m', msg])

  if options.no_upload:
    return
  environ = os.environ.copy()
  environ['EDITOR'] = 'true'
  # TODO(mseaborn): This can ask for credentials when the cached
  # credentials expire, so could fail when automated.  Can we fix
  # that?
  subprocess.check_call(['git', 'cl', 'upload', '-m', msg, '--cc', cc_list],
                        env=environ)


if __name__ == '__main__':
  Main(sys.argv[1:])
