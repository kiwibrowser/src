# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import re

from file_system import FileNotFoundError


# This provides utilities for extracting information from the local git
# repository to which it belongs.


# Regex to match ls-tree output.
_LS_TREE_REGEX = re.compile(
    '\d+\s(?P<type>\w+)\s(?P<id>[0-9a-f]{40})\s(?P<name>.*)')


def _GetRoot():
  return os.path.join(os.path.dirname(os.path.realpath(__file__)),
                      os.pardir, os.pardir, os.pardir, os.pardir, os.pardir)


def _RelativePath(path):
  return os.path.join(_GetRoot(), path)


def RunGit(command, args=[]):
  # We import subprocess symbols lazily because they aren't available on
  # AppEngine, and the frontend may load (but should never execute) this module.
  from subprocess import check_output
  with open(os.devnull, 'w') as dev_null:
    return check_output(['git', command] + args, stderr=dev_null,
        cwd=_GetRoot())


def ParseRevision(name):
  return RunGit('rev-parse', [name]).rstrip()


def GetParentRevision(revision):
  return RunGit('show', ['-s', '--format=%P', revision]).rstrip()


def GetRootTree(revision):
  return RunGit('show', ['-s', '--format=%T', revision]).rstrip()


def ListDir(path, revision='HEAD'):
  '''Retrieves a directory listing for the specified path and optional revision
  (default is HEAD.) Returns a list of objects with the following properties:

    |type|: the type of entry; 'blob' (file) or 'tree' (directory)
    |id|: the hash of the directory entry. This is either tree hash or blob hash
    |name|: the name of the directory entry.
  '''
  from subprocess import CalledProcessError
  try:
    ref = '%s:%s' % (ParseRevision(revision), path.lstrip('/'))
    listing = RunGit('ls-tree', [ref]).rstrip().splitlines()
  except CalledProcessError:
    raise FileNotFoundError('%s not found in revision %s' % (path, revision))

  def parse_line(line):
    return re.match(_LS_TREE_REGEX, line).groupdict()

  return map(parse_line, listing)


def ReadFile(path, revision='HEAD'):
  '''Retrieves the contents of a file at the specified path and optional
  revision (default is HEAD.) Returns its contents as a string.
  '''
  from subprocess import CalledProcessError
  try:
    return RunGit('show', ['%s:%s' % (ParseRevision(revision), path)])
  except CalledProcessError:
    raise FileNotFoundError('%s not found in revision %s' % (path, revision))
