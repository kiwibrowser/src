# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest
import os

from gpu_tests import path_util

path_util.AddDirToPathIfNeeded(
    path_util.GetChromiumSrcDir(), 'third_party', 'logilab')

path_util.AddDirToPathIfNeeded(
    path_util.GetChromiumSrcDir(), 'third_party', 'logilab', 'logilab')

path_util.AddDirToPathIfNeeded(
    path_util.GetChromiumSrcDir(), 'third_party', 'pylint')

try:
  from pylint import lint
except ImportError:
  lint = None


_RC_FILE = os.path.join(path_util.GetGpuTestDir(), 'pylintrc')

def LintCheckPassed(directory):
  args = [directory, '--rcfile=%s' % _RC_FILE]
  try:
    assert lint, 'pylint module cannot be found'
    lint.Run(args)
    assert False, (
        'This should not be reached as lint.Run always raise SystemExit')
  except SystemExit as err:
    if err.code == 0:
      return True
    return False


class LintTest(unittest.TestCase):
  def testPassingPylintCheckForGpuTestsDir(self):
    self.assertTrue(LintCheckPassed(os.path.abspath(os.path.dirname(__file__))))
