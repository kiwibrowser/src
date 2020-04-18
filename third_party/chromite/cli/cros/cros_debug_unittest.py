# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module tests the cros debug command."""

from __future__ import print_function

from chromite.cli import command_unittest
from chromite.cli.cros import cros_debug
from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import remote_access


class MockDebugCommand(command_unittest.MockCommand):
  """Mock out the debug command."""
  TARGET = 'chromite.cli.cros.cros_debug.DebugCommand'
  TARGET_CLASS = cros_debug.DebugCommand
  COMMAND = 'debug'
  ATTRS = ('_ListProcesses', '_DebugNewProcess', '_DebugRunningProcess')

  def __init__(self, *args, **kwargs):
    command_unittest.MockCommand.__init__(self, *args, **kwargs)

  def _ListProcesses(self, _inst, *_args, **_kwargs):
    """Mock out _ListProcesses."""

  def _DebugNewProcess(self, _inst, *_args, **_kwargs):
    """Mock out _DebugNewProcess."""

  def _DebugRunningProcess(self, _inst, *_args, **_kwargs):
    """Mock out _DebugRunningProcess."""

  def Run(self, inst):
    command_unittest.MockCommand.Run(self, inst)


class DebugRunThroughTest(cros_test_lib.MockTempDirTestCase):
  """Test the flow of DebugCommand.run with the debug methods mocked out."""

  DEVICE = '1.1.1.1'
  EXE = '/path/to/exe'
  PID = '1'

  def SetupCommandMock(self, cmd_args):
    """Set up command mock."""
    self.cmd_mock = MockDebugCommand(
        cmd_args, base_args=['--cache-dir', self.tempdir])
    self.StartPatcher(self.cmd_mock)

  def setUp(self):
    """Patches objects."""
    self.cmd_mock = None
    self.device_mock = self.PatchObject(remote_access,
                                        'ChromiumOSDevice').return_value

  def testMissingExeAndPid(self):
    """Test that command fails when --exe and --pid are not provided."""
    self.SetupCommandMock([self.DEVICE])
    self.assertRaises(cros_build_lib.DieSystemExit, self.cmd_mock.inst.Run)

  def testListDisallowedWithPid(self):
    """Test that --list is disallowed when --pid is used."""
    self.SetupCommandMock([self.DEVICE, '--list', '--pid', self.PID])
    self.assertRaises(cros_build_lib.DieSystemExit, self.cmd_mock.inst.Run)

  def testExeDisallowedWithPid(self):
    """Test that --exe is disallowed when --pid is used."""
    self.SetupCommandMock([self.DEVICE, '--exe', self.EXE, '--pid', self.PID])
    self.assertRaises(cros_build_lib.DieSystemExit, self.cmd_mock.inst.Run)

  def testExeMustBeFullPath(self):
    """Test that --exe only takes full path as a valid argument."""
    self.SetupCommandMock([self.DEVICE, '--exe', 'bash'])
    self.assertRaises(cros_build_lib.DieSystemExit, self.cmd_mock.inst.Run)

  def testDebugProcessWithPid(self):
    """Test that methods are called correctly when pid is provided."""
    self.SetupCommandMock([self.DEVICE, '--pid', self.PID])
    self.cmd_mock.inst.Run()
    self.assertFalse(self.cmd_mock.patched['_ListProcesses'].called)
    self.assertFalse(self.cmd_mock.patched['_DebugNewProcess'].called)
    self.assertTrue(self.cmd_mock.patched['_DebugRunningProcess'].called)

  def testListProcesses(self):
    """Test that methods are called correctly for listing processes."""
    self.SetupCommandMock([self.DEVICE, '--exe', self.EXE, '--list'])
    self.cmd_mock.inst.Run()
    self.assertTrue(self.cmd_mock.patched['_ListProcesses'].called)
    self.assertFalse(self.cmd_mock.patched['_DebugNewProcess'].called)
    self.assertFalse(self.cmd_mock.patched['_DebugRunningProcess'].called)

  def testNoRunningProcess(self):
    """Test that command starts a new process to debug if no process running."""
    self.SetupCommandMock([self.DEVICE, '--exe', self.EXE])
    self.PatchObject(self.device_mock, 'GetRunningPids', return_value=[])
    self.cmd_mock.inst.Run()
    self.assertTrue(self.cmd_mock.patched['_ListProcesses'].called)
    self.assertTrue(self.cmd_mock.patched['_DebugNewProcess'].called)
    self.assertFalse(self.cmd_mock.patched['_DebugRunningProcess'].called)

  def testDebugNewProcess(self):
    """Test that user can select zero to start a new process to debug."""
    self.SetupCommandMock([self.DEVICE, '--exe', self.EXE])
    self.PatchObject(self.device_mock, 'GetRunningPids', return_value=['1'])
    mock_prompt = self.PatchObject(cros_build_lib, 'GetChoice', return_value=0)
    self.cmd_mock.inst.Run()
    self.assertTrue(mock_prompt.called)
    self.assertTrue(self.cmd_mock.patched['_ListProcesses'].called)
    self.assertTrue(self.cmd_mock.patched['_DebugNewProcess'].called)
    self.assertFalse(self.cmd_mock.patched['_DebugRunningProcess'].called)

  def testDebugRunningProcess(self):
    """Test that user can select none-zero to debug a running process."""
    self.SetupCommandMock([self.DEVICE, '--exe', self.EXE])
    self.PatchObject(self.device_mock, 'GetRunningPids', return_value=['1'])
    mock_prompt = self.PatchObject(cros_build_lib, 'GetChoice', return_value=1)
    self.cmd_mock.inst.Run()
    self.assertTrue(mock_prompt.called)
    self.assertTrue(self.cmd_mock.patched['_ListProcesses'].called)
    self.assertFalse(self.cmd_mock.patched['_DebugNewProcess'].called)
    self.assertTrue(self.cmd_mock.patched['_DebugRunningProcess'].called)
