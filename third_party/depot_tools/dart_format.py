#!/usr/bin/python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Redirects to the version of dartfmt checked into a gclient repo.

dartfmt binaries are pulled down during gclient sync in the mojo repo.

This tool is named dart_format.py instead of dartfmt to parallel
clang_format.py, which is in this same repository."""

import os
import subprocess
import sys

import gclient_utils

class NotFoundError(Exception):
  """A file could not be found."""
  def __init__(self, e):
    Exception.__init__(self,
        'Problem while looking for dartfmt in Chromium source tree:\n'
        '  %s' % e)


def FindDartFmtToolInChromiumTree():
  """Return a path to the dartfmt executable, or die trying."""
  primary_solution_path = gclient_utils.GetPrimarySolutionPath()
  if not primary_solution_path:
    raise NotFoundError(
        'Could not find checkout in any parent of the current path.')

  dartfmt_path = os.path.join(primary_solution_path, 'third_party', 'dart-sdk',
                              'dart-sdk', 'bin', 'dartfmt')
  if not os.path.exists(dartfmt_path):
    raise NotFoundError('File does not exist: %s' % dartfmt_path)
  return dartfmt_path


def main(args):
  try:
    tool = FindDartFmtToolInChromiumTree()
  except NotFoundError, e:
    print >> sys.stderr, e
    sys.exit(1)

  # Add some visibility to --help showing where the tool lives, since this
  # redirection can be a little opaque.
  help_syntax = ('-h', '--help', '-help', '-help-list', '--help-list')
  if any(match in args for match in help_syntax):
    print '\nDepot tools redirects you to the dartfmt at:\n    %s\n' % tool

  return subprocess.call([tool] + sys.argv[1:])


if __name__ == '__main__':
  sys.exit(main(sys.argv))
