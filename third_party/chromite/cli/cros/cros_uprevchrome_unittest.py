# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for cros_uprevchrome."""

from __future__ import print_function

import mock
import time
import copy

from chromite.lib import cros_test_lib
from chromite.lib import cidb
from chromite.lib import git
from chromite.cli import command_unittest
from chromite.cli.cros import cros_pinchrome
from chromite.cli.cros import cros_uprevchrome
from chromite.lib import constants

class MockUprevCommand(command_unittest.MockCommand):
  """Mock out the uprevchrome command."""
  TARGET = 'chromite.cli.cros.cros_uprevchrome.UprevChromeCommand'
  TARGET_CLASS = cros_uprevchrome.UprevChromeCommand
  COMMAND = 'uprevchrome'

  def __init__(self, *args, **kwargs):
    command_unittest.MockCommand.__init__(self, *args, **kwargs)

  def Run(self, inst):
    command_unittest.MockCommand.Run(self, inst)

class CrosUprevChromeTest(cros_test_lib.MockTempDirTestCase,
                          cros_test_lib.OutputTestCase):
  """Test for cros uprevchrome."""
  cmd_args = ['--pfq-build=100',
              '--cred-dir=/cred_dir',
              '--bug=100']
  mock_pfq_info = {'id': '100',
                   'status': constants.BUILDER_STATUS_FAILED,
                   'build_number': '100',
                   'start_time': time.time(),
                   'build_config': constants.PFQ_MASTER}

  def SetupCommandMock(self, cmd_args):
    """Setup comand mock."""
    self.cmd_mock = MockUprevCommand(cmd_args)
    self.StartPatcher(self.cmd_mock)

  def setUp(self):
    """Patches objects."""
    self.cmd_mock = None
    cidb.CIDBConnection.__init__ = mock.Mock(return_value=None)
    self.PatchObject(cros_pinchrome, 'CloneWorkingRepo')
    self.PatchObject(git, 'RunGit')
    cidb.CIDBConnection.GetBuildStatus = mock.Mock(
        return_value=self.mock_pfq_info)
    list_history = []
    cidb.CIDBConnection.GetBuildHistory = mock.Mock(
        return_value=list_history)
    self.PatchObject(git, 'Commit', return_value='change_id')

  def testValidatePFQBuild(self):
    """Test ValidatePFQBuild."""
    self.SetupCommandMock(self.cmd_args)
    db = cidb.CIDBConnection('cred_dir')
    self.assertEqual('100', self.cmd_mock.inst.ValidatePFQBuild(100, db))

  def testPassedPFQBuildId(self):
    """Test a passed PFQ build_id"""
    self.SetupCommandMock(self.cmd_args)
    local_mock_pfq_info = copy.deepcopy(self.mock_pfq_info)
    local_mock_pfq_info['status'] = constants.BUILDER_STATUS_PASSED
    cidb.CIDBConnection.GetBuildStatus = mock.Mock(
        return_value=local_mock_pfq_info)
    self.assertRaises(cros_uprevchrome.InvalidPFQBuildIdExcpetion,
                      self.cmd_mock.inst.Run)

  def testPassedPFQBuildHistory(self):
    """Test a failed PFQ build_id with one followed PFQ which is passed."""
    self.SetupCommandMock(self.cmd_args)
    pass_list_history = [{'id': '101',
                          'status': constants.BUILDER_STATUS_PASSED}]
    cidb.CIDBConnection.GetBuildHistory = mock.Mock(
        return_value=pass_list_history)
    self.assertRaises(cros_uprevchrome.InvalidPFQBuildIdExcpetion,
                      self.cmd_mock.inst.Run)

  def testPassedPFQBuildHistory2(self):
    """Test a failed PFQ build_id with two followed PFQs, one is passed."""
    self.SetupCommandMock(self.cmd_args)
    pass_list_history = [{'id': '101',
                          'status': constants.BUILDER_STATUS_FAILED},
                         {'id': '102',
                          'status': constants.BUILDER_STATUS_PASSED}]
    cidb.CIDBConnection.GetBuildHistory = mock.Mock(
        return_value=pass_list_history)
    self.assertRaises(cros_uprevchrome.InvalidPFQBuildIdExcpetion,
                      self.cmd_mock.inst.Run)

  def testNoChangeIdCommitLogs(self):
    """Test commit logs without ChangeIds."""
    self.SetupCommandMock(self.cmd_args)
    self.PatchObject(git, 'Commit', return_value=None)
    self.assertRaises(Exception, self.cmd_mock.inst.Run)

  def testSuccessfulRun(self):
    """Test a successful uprevchrome run."""
    self.SetupCommandMock(self.cmd_args)
    self.cmd_mock.inst.Run()
