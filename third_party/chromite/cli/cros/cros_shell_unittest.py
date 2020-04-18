# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests the `cros shell` command."""

from __future__ import print_function

from chromite.cli import command_unittest
from chromite.cli.cros import cros_shell
from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import remote_access


class _KeyMismatchError(remote_access.SSHConnectionError):
  """Test exception to fake a key mismatch."""
  def IsKnownHostsMismatch(self):
    return True


class MockShellCommand(command_unittest.MockCommand):
  """Mock out the `cros shell` command."""
  TARGET = 'chromite.cli.cros.cros_shell.ShellCommand'
  TARGET_CLASS = cros_shell.ShellCommand
  COMMAND = 'shell'


class ShellTest(cros_test_lib.MockTempDirTestCase,
                cros_test_lib.OutputTestCase):
  """Test the flow of ShellCommand.run with the SSH methods mocked out."""

  DEVICE_IP = '1.1.1.1'

  def SetupCommandMock(self, cmd_args):
    """Sets up the `cros shell` command mock."""
    self.cmd_mock = MockShellCommand(
        cmd_args, base_args=['--cache-dir', self.tempdir])
    self.StartPatcher(self.cmd_mock)

  def setUp(self):
    """Patches objects."""
    self.cmd_mock = None

    # Patch any functions we want to control that may get called by a test.
    self.mock_remove_known_host = self.PatchObject(
        remote_access, 'RemoveKnownHost', autospec=True)
    self.mock_prompt = self.PatchObject(
        cros_build_lib, 'BooleanPrompt', autospec=True)

    self.mock_device = self.PatchObject(
        remote_access, 'ChromiumOSDevice', autospec=True).return_value
    self.mock_device.hostname = self.DEVICE_IP
    self.mock_device.connection_type = None
    self.mock_base_run_command = self.mock_device.BaseRunCommand
    self.mock_base_run_command.return_value = cros_build_lib.CommandResult()

  def testSshInteractive(self):
    """Tests flow for an interactive session.

    User should not be prompted for input, and SSH should be attempted
    once.
    """
    self.SetupCommandMock([self.DEVICE_IP])
    self.cmd_mock.inst.Run()

    self.assertEqual(self.mock_base_run_command.call_count, 1)
    # Make sure that BaseRunCommand() started an interactive session (no cmd).
    self.assertEqual(self.mock_base_run_command.call_args[0][0], [])
    self.assertFalse(self.mock_prompt.called)

  def testSshNonInteractiveSingleArg(self):
    """Tests a non-interactive command as a single argument.

    Example: cros shell 1.1.1.1 "ls -l /etc"
    """
    self.SetupCommandMock([self.DEVICE_IP, 'ls -l /etc'])
    self.cmd_mock.inst.Run()

    self.assertEqual(self.mock_base_run_command.call_args[0][0],
                     ['ls -l /etc'])

  def testSshNonInteractiveMultipleArgs(self):
    """Tests a non-interactive command as multiple arguments with "--".

    Example: cros shell 1.1.1.1 -- ls -l /etc
    """
    self.SetupCommandMock([self.DEVICE_IP, '--', 'ls', '-l', '/etc'])
    self.cmd_mock.inst.Run()

    self.assertEqual(self.mock_base_run_command.call_args[0][0],
                     ['ls', '-l', '/etc'])

  def testSshReturnValue(self):
    """Tests that `cros shell` returns the exit code of BaseRunCommand()."""
    self.SetupCommandMock([self.DEVICE_IP])
    self.mock_base_run_command.return_value.returncode = 42

    self.assertEqual(self.cmd_mock.inst.Run(), 42)

  def testSshKeyChangeOK(self):
    """Tests a host SSH key changing but the user giving it the OK.

    User should be prompted, SSH should be attempted twice, and host
    keys should be removed.
    """
    self.SetupCommandMock([self.DEVICE_IP])
    error_message = 'Test error message'
    # BaseRunCommand() gives a key mismatch error the first time only.
    self.mock_base_run_command.side_effect = [_KeyMismatchError(error_message),
                                              cros_build_lib.CommandResult()]
    # User chooses to continue.
    self.mock_prompt.return_value = True

    with self.OutputCapturer():
      self.cmd_mock.inst.Run()

    self.AssertOutputContainsWarning(error_message, check_stderr=True)
    self.assertTrue(self.mock_prompt.called)
    self.assertEqual(self.mock_base_run_command.call_count, 2)
    self.assertTrue(self.mock_remove_known_host.called)

  def testSshKeyChangeAbort(self):
    """Tests a host SSH key changing and the user canceling.

    User should be prompted, but SSH should only be attempted once, and
    no host keys should be removed.
    """
    self.SetupCommandMock([self.DEVICE_IP])
    self.mock_base_run_command.side_effect = _KeyMismatchError()
    # User chooses to abort.
    self.mock_prompt.return_value = False

    self.cmd_mock.inst.Run()

    self.assertTrue(self.mock_prompt.called)
    self.assertEqual(self.mock_base_run_command.call_count, 1)
    self.assertFalse(self.mock_remove_known_host.called)

  def testSshConnectError(self):
    """Tests an SSH error other than a host key mismatch.

    User should not be prompted, SSH should only be attempted once, and
    no host keys should be removed.
    """
    self.SetupCommandMock([self.DEVICE_IP])
    self.mock_base_run_command.side_effect = remote_access.SSHConnectionError()

    self.assertRaises(remote_access.SSHConnectionError, self.cmd_mock.inst.Run)

    self.assertFalse(self.mock_prompt.called)
    self.assertEqual(self.mock_base_run_command.call_count, 1)
    self.assertFalse(self.mock_remove_known_host.called)
