# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module tests the cros bisect command."""

from __future__ import print_function

import argparse

from chromite.cros_bisect import autotest_evaluator
from chromite.cros_bisect import git_bisector
from chromite.cros_bisect import simple_chrome_builder
from chromite.cli.cros import cros_bisect
from chromite.lib import commandline
from chromite.lib import cros_test_lib
from chromite.lib import remote_access


class CrosBisectTest(cros_test_lib.MockTestCase):
  """Test BisectCommand."""
  DEFAULT_ARGS = ['--good', '900d900d', '--bad', 'badbad', '--remote',
                  '192.168.0.11', '--base-dir', '/tmp/bisect']

  def setUp(self):
    self.parser = commandline.ArgumentParser()
    cros_bisect.BisectCommand.AddParser(self.parser)

  def testAddParser(self):
    # Use commandline.ArgumentParser to get 'path' type support.
    options = self.parser.parse_args(self.DEFAULT_ARGS)
    self.assertTrue(
        simple_chrome_builder.SimpleChromeBuilder.SanityCheckOptions(options))
    self.assertTrue(
        autotest_evaluator.AutotestEvaluator.SanityCheckOptions(options))
    self.assertTrue(
        git_bisector.GitBisector.SanityCheckOptions(options))

  def testRun(self):
    options = self.parser.parse_args(self.DEFAULT_ARGS + ['--board', 'test'])

    self.PatchObject(simple_chrome_builder.SimpleChromeBuilder, '__init__',
                     return_value=None)
    self.PatchObject(autotest_evaluator.AutotestEvaluator, '__init__',
                     return_value=None)
    self.PatchObject(git_bisector.GitBisector, '__init__',
                     return_value=None)
    self.PatchObject(git_bisector.GitBisector, 'SetUp')
    self.PatchObject(git_bisector.GitBisector, 'Run')

    bisector = cros_bisect.BisectCommand(options)
    bisector.Run()

  def testProcessOptionsReuseFlag(self):
    args_with_board = self.DEFAULT_ARGS + ['--board', 'test']
    options = self.parser.parse_args(self.DEFAULT_ARGS)
    options = self.parser.parse_args(args_with_board)
    bisector = cros_bisect.BisectCommand(options)
    bisector.ProcessOptions()
    self.assertFalse(bisector.options.reuse_repo)
    self.assertFalse(bisector.options.reuse_build)
    self.assertFalse(bisector.options.reuse_eval)

    # Flip --reuse.
    options = self.parser.parse_args(args_with_board + ['--reuse'])
    bisector = cros_bisect.BisectCommand(options)
    bisector.ProcessOptions()
    self.assertTrue(bisector.options.reuse_repo)
    self.assertTrue(bisector.options.reuse_build)
    self.assertTrue(bisector.options.reuse_eval)

  def testProcessOptionsResolveBoard(self):
    # No --board specified.
    options = self.parser.parse_args(self.DEFAULT_ARGS)

    self.PatchObject(remote_access, 'ChromiumOSDevice',
                     return_value=cros_test_lib.EasyAttr(board='test_board'))
    bisector = cros_bisect.BisectCommand(options)
    bisector.ProcessOptions()
    self.assertEqual('test_board', bisector.options.board)

  def testProcessOptionsResolveBoardFailed(self):
    # No --board specified.
    options = self.parser.parse_args(self.DEFAULT_ARGS)

    self.PatchObject(remote_access, 'ChromiumOSDevice',
                     return_value=cros_test_lib.EasyAttr(board=''))
    bisector = cros_bisect.BisectCommand(options)
    self.assertRaisesRegexp(Exception, 'Unable to obtain board name from DUT',
                            bisector.ProcessOptions)

  def testGoodBadCommitType(self):
    """Tests GoodBadCommitType."""
    sha1 = '900d900d'
    self.assertEqual(sha1, cros_bisect.GoodBadCommitType(sha1))

    not_sha1 = 'bad_sha1'
    self.assertRaises(argparse.ArgumentTypeError,
                      cros_bisect.GoodBadCommitType,
                      not_sha1)

    cros_version = 'R60-9531.0.0'
    self.assertEqual(cros_version, cros_bisect.GoodBadCommitType(cros_version))

    cros_version_variant = '60.9531.0.0'
    self.assertEqual(cros_version,
                     cros_bisect.GoodBadCommitType(cros_version_variant))

    not_cros_version = '91.3.3'
    self.assertRaises(argparse.ArgumentTypeError,
                      cros_bisect.GoodBadCommitType,
                      not_cros_version)
