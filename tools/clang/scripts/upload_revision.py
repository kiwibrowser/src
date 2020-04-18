#!/usr/bin/env python
# Copyright (c) 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This script takes a Clang revision as an argument, it then
creates a feature branch, puts this revision into update.py, uploads
a CL, triggers Clang Upload try bots, and tells what to do next"""

import argparse
import fnmatch
import itertools
import os
import re
import shutil
import subprocess
import sys

# Path constants.
THIS_DIR = os.path.dirname(__file__)
UPDATE_PY_PATH = os.path.join(THIS_DIR, "update.py")
CHROMIUM_DIR = os.path.abspath(os.path.join(THIS_DIR, '..', '..', '..'))

is_win = sys.platform.startswith('win32')

def PatchRevision(clang_revision, clang_sub_revision):
  with open(UPDATE_PY_PATH, 'rb') as f:
    content = f.read()
  m = re.search("CLANG_REVISION = '([0-9]+)'", content)
  clang_old_revision = m.group(1)
  content = re.sub("CLANG_REVISION = '[0-9]+'",
                   "CLANG_REVISION = '{}'".format(clang_revision),
                   content, count=1)
  content = re.sub("CLANG_SUB_REVISION=[0-9]+",
                   "CLANG_SUB_REVISION={}".format(clang_sub_revision),
                   content, count=1)
  with open(UPDATE_PY_PATH, 'wb') as f:
    f.write(content)
  return clang_old_revision


def Git(args):
  # Needs shell=True on Windows due to git.bat in depot_tools.
  subprocess.check_call(["git"] + args, shell=is_win)

def main():
  parser = argparse.ArgumentParser(description='upload new clang revision')
  parser.add_argument('clang_revision', metavar='CLANG_REVISION',
                      type=int, nargs=1,
                      help='Clang revision to build the toolchain for.')
  parser.add_argument('clang_sub_revision', metavar='CLANG_SUB_REVISION',
                      type=int, nargs='?', default=1,
                      help='Clang sub-revision to build the toolchain for.')

  args = parser.parse_args()

  clang_revision = args.clang_revision[0]
  clang_sub_revision = args.clang_sub_revision
  # Needs shell=True on Windows due to git.bat in depot_tools.
  git_revision = subprocess.check_output(
      ["git", "rev-parse", "origin/master"], shell=is_win).strip()
  print "Making a patch for Clang revision r{}-{}".format(
      clang_revision, clang_sub_revision)
  print "Chrome revision: {}".format(git_revision)
  clang_old_revision = PatchRevision(clang_revision, clang_sub_revision)

  Git(["checkout", "-b", "clang-{}-{}".format(
      clang_revision, clang_sub_revision)])
  Git(["add", UPDATE_PY_PATH])

  commit_message = 'Ran `{}`.'.format(' '.join(sys.argv))
  Git(["commit", "-m", "Roll clang {}:{}.\n\n{}".format(
      clang_old_revision, clang_revision, commit_message)])

  Git(["cl", "upload", "-f"])
  Git(["cl", "try", "-b", "linux_upload_clang", "-r", git_revision])
  Git(["cl", "try", "-b", "mac_upload_clang", "-r", git_revision])
  Git(["cl", "try", "-b", "win_upload_clang", "-r", git_revision])

  print ("Please, wait until the try bots succeeded "
         "and then push the binaries to goma.")

if __name__ == '__main__':
  sys.exit(main())
