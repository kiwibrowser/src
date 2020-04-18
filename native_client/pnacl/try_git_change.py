#!/usr/bin/env python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This tool will send a PNaCl Git change to the trybots for testing.

This tool is intended for testing changes to the PNaCl Git
repositories that are checked out under subdirectories of toolchain_build/src.
It should be run from one of these subdirectories.  The tool sends
changes to the PNaCl toolchain trybots.

Example usage:

$ cd toolchain_build/src/llvm
$ git checkout -b my-change origin/master
$ ... make changes to LLVM
$ git commit -a
$ git cl upload
$ ../../try_git_change.py

The trybot results will appear on the Rietveld code review created by
"git cl upload".
"""

import argparse
import base64
import os
import re
import shutil
import subprocess
import sys
import tempfile


# Find the top-level directory of the Git checkout that the current
# directory is inside.
def FindGitDir():
  path = os.getcwd()
  while not os.path.exists(os.path.join(path, '.git')):
    parent = os.path.dirname(path)
    if parent == path:
      raise Exception('Current directory is not inside a Git repo directory')
    path = parent
  return path


def WriteGitBundle(dest_dir, component_name):
  patch_dir = os.path.join(dest_dir, 'pnacl', 'not_for_commit')
  os.makedirs(patch_dir)

  # Record the commit ID.  Include the commit's subject line to give
  # readable context, to make it easy to tell from the try job which
  # Git change was tested.
  subprocess.check_call(
      ['git', 'log', '--no-walk',
       '--pretty=format:%H%n%ad%n%s', 'HEAD'],
      stdout=open(os.path.join(patch_dir,
                               '%s_commit_id' % component_name), 'w'))

  # Save the Git commits as a Git bundle.
  proc = subprocess.Popen(['git', 'bundle', 'create', '-',
                           'origin/master..HEAD'],
                          stdout=subprocess.PIPE)
  # Encode the binary bundle using base64 so that it can be included
  # in a textual patch file.
  with open(os.path.join(patch_dir, '%s_bundle.b64' % component_name),
            'w') as bundle_file:
    base64.encode(proc.stdout, bundle_file)
  rc = proc.wait()
  assert rc == 0, 'git bundle failed: %i' % rc


def Main(args):
  parser = argparse.ArgumentParser(
      epilog=__doc__,
      formatter_class=argparse.RawDescriptionHelpFormatter)
  parser.add_argument('-n', '--dry-run', action='store_true', default=False,
                      dest='dry_run',
                      help='Output the patch that would be sent to the '
                      'try server without sending it')
  options = parser.parse_args(args)

  # Find the name of the Git repo we're testing a change for (e.g. 'llvm').
  repo_dir = FindGitDir()
  parent_path1, component_name = os.path.split(repo_dir)
  parent_path2, parent2 = os.path.split(parent_path1)
  parent_path3, parent3 = os.path.split(parent_path2)
  if ((parent3, parent2) != ('toolchain_build', 'src')):
    raise Exception(
        'Expected the Git repo (%r) to be under '
        'toolchain_build/src/' % repo_dir)
  print 'Trying change to %r component' % component_name

  # Check that there are no uncommitted changes.
  rc = subprocess.call(['git', 'diff', '--quiet', 'HEAD'])
  if rc != 0:
    raise Exception('There are local uncommitted changes to %r'
                    % component_name)

  temp_dir = tempfile.mkdtemp(prefix='try_pnacl_git_change_')
  try:
    before_dir = os.path.join(temp_dir, 'a')
    after_dir = os.path.join(temp_dir, 'b')
    os.mkdir(before_dir)
    os.mkdir(after_dir)
    WriteGitBundle(after_dir, component_name)

    patch_file = os.path.join(temp_dir, 'trybot_patch')
    with open(patch_file, 'w') as f:
      rc = subprocess.call(['diff', '-urN', 'a', 'b'], cwd=temp_dir, stdout=f)

    # massage patch file to resemble one produced by 'git diff --no-prefix'
    # which is how 'git try' normally generates patches.  Unless we do this
    # the code in bot_update.py fails to parse or apply the patch correctly.
    with open(patch_file) as f:
      patch_data = f.read()
    patch_data = re.sub("^diff -urN", "diff --git", patch_data, flags=re.M)
    patch_data = re.sub("^--- a/", "--- ", patch_data, flags=re.M)
    patch_data = re.sub("^\+\+\+ b/", "+++ ", patch_data, flags=re.M)
    with open(patch_file, 'w') as f:
      f.write(patch_data)

    # diff returns 1 when the patch is non-empty.
    if rc != 1:
      raise Exception('diff failed with exit status %i' % rc)

    trybots = [
        'nacl-toolchain-linux-pnacl-x86_64',
        'nacl-toolchain-linux-pnacl-x86_32',
        'nacl-toolchain-mac-pnacl-x86_32',
        'nacl-toolchain-win7-pnacl-x86_64',
        ]

    if options.dry_run:
      subprocess.check_call(['cat', patch_file])
      return

    # Send the patch to the trybots.  Keeping the cwd set to
    # toolchain_build/src/FOO has the effect of sending the trybot results to
    # the Rietveld code review for the change to FOO.
    subprocess.check_call([
        'git', 'try',
        # Specify the patch.
        '-p1', '--diff=%s' % patch_file,
        # Directory that the trybot should apply the patch to.
        '--root=native_client',
        # SVN URL for trybot patch queue.
        '--svn_repo=svn://svn.chromium.org/chrome-try/try-nacl',
        '-b', ','.join(trybots)])
  finally:
    shutil.rmtree(temp_dir)


if __name__ == '__main__':
  Main(sys.argv[1:])
