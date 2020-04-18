# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test the remote_access module."""

from __future__ import print_function

import os

from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import osutils
from chromite.lib import partial_mock
from chromite.lib import remote_access


# pylint: disable=protected-access


class TestNormalizePort(cros_test_lib.TestCase):
  """Verifies we normalize port."""

  def testNormalizePortStrOK(self):
    """Tests that string will be converted to integer."""
    self.assertEqual(remote_access.NormalizePort('123'), 123)

  def testNormalizePortStrNotOK(self):
    """Tests that error is raised if port is string and str_ok=False."""
    self.assertRaises(
        ValueError, remote_access.NormalizePort, '123', str_ok=False)

  def testNormalizePortOutOfRange(self):
    """Tests that error is rasied when port is out of range."""
    self.assertRaises(ValueError, remote_access.NormalizePort, '-1')
    self.assertRaises(ValueError, remote_access.NormalizePort, 99999)


class TestRemoveKnownHost(cros_test_lib.MockTempDirTestCase):
  """Verifies RemoveKnownHost() functionality."""

  # ssh-keygen doesn't check for a valid hostname so use something that won't
  # be in the user's known_hosts to avoid changing their file contents.
  _HOST = '0.0.0.0.0.0'

  _HOST_KEY = (
      _HOST + ' ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQCjysPTaDAtRaxRaW1JjqzCHp2'
      '88gvlUgtJxd2Jt/v63fkqZ5zzLLoeoAMwv0oYSRU82qhLimXpHxXRkrMC5nrpz5zJch+ktql'
      '0rSRgo+dqc1GzmyOOAq5NkQsgBb3hefxMxCZRV8Dv0n7qaindZRxE8MnRJmVUoj8Wq8wryab'
      'p+fUBkesBwaJhPXa4WBJeI5d+rO5tEBSNkvIp0USU6Ku3Ct0q2sZbOkY5g1VFAUYm4wyshCf'
      'oWvU8ivMFp0pCezMISGstKpkIQApq2dLUb6EmeIgnhHzZXOn7doxIGD33JUfFmwNi0qfk3vV'
      '6vKRVDEZD68+ix6gjKpicY5upA/9P\n')

  def testRemoveKnownHostDefaultFile(self):
    """Tests RemoveKnownHost() on the default known_hosts file.

    `ssh-keygen -R` on its own fails when run from within the chroot
    since the default known_hosts is bind mounted.
    """
    # It doesn't matter if known_hosts actually has this host in it or not,
    # this test just makes sure the command doesn't fail. The default
    # known_hosts file always exists in the chroot due to the bind mount.
    remote_access.RemoveKnownHost(self._HOST)

  def testRemoveKnownHostCustomFile(self):
    """Tests RemoveKnownHost() on a custom known_hosts file."""
    path = os.path.join(self.tempdir, 'known_hosts')
    osutils.WriteFile(path, self._HOST_KEY)
    remote_access.RemoveKnownHost(self._HOST, known_hosts_path=path)
    self.assertEqual(osutils.ReadFile(path), '')

  def testRemoveKnownHostNonexistentFile(self):
    """Tests RemoveKnownHost() on a nonexistent known_hosts file."""
    path = os.path.join(self.tempdir, 'known_hosts')
    remote_access.RemoveKnownHost(self._HOST, known_hosts_path=path)


class TestCompileSSHConnectSettings(cros_test_lib.TestCase):
  """Verifies CompileSSHConnectSettings()."""

  def testCustomSettingIncluded(self):
    """Tests that a custom setting will be included in the output."""
    self.assertIn(
        '-oNumberOfPasswordPrompts=100',
        remote_access.CompileSSHConnectSettings(NumberOfPasswordPrompts=100))

  def testNoneSettingOmitted(self):
    """Tests that a None value will omit a default setting from the output."""
    self.assertIn('-oProtocol=2', remote_access.CompileSSHConnectSettings())
    self.assertNotIn(
        '-oProtocol=2',
        remote_access.CompileSSHConnectSettings(Protocol=None))


class RemoteShMock(partial_mock.PartialCmdMock):
  """Mocks the RemoteSh function."""
  TARGET = 'chromite.lib.remote_access.RemoteAccess'
  ATTRS = ('RemoteSh',)
  DEFAULT_ATTR = 'RemoteSh'

  def RemoteSh(self, inst, cmd, *args, **kwargs):
    """Simulates a RemoteSh invocation.

    Returns:
      A CommandResult object with an additional member |rc_mock| to
      enable examination of the underlying RunCommand() function call.
    """
    result = self._results['RemoteSh'].LookupResult(
        (cmd,), hook_args=(inst, cmd,) + args, hook_kwargs=kwargs)

    # Run the real RemoteSh with RunCommand mocked out.
    rc_mock = cros_test_lib.RunCommandMock()
    rc_mock.AddCmdResult(
        partial_mock.Ignore(), result.returncode, result.output, result.error)

    with rc_mock:
      result = self.backup['RemoteSh'](inst, cmd, *args, **kwargs)
    result.rc_mock = rc_mock
    return result


class RemoteDeviceMock(partial_mock.PartialMock):
  """Mocks the RemoteDevice function."""

  TARGET = 'chromite.lib.remote_access.RemoteDevice'
  ATTRS = ('Pingable',)

  def Pingable(self, _):
    return True


class RemoteAccessTest(cros_test_lib.MockTempDirTestCase):
  """Base class with RemoteSh mocked out for testing RemoteAccess."""
  def setUp(self):
    self.rsh_mock = self.StartPatcher(RemoteShMock())
    self.host = remote_access.RemoteAccess('foon', self.tempdir)


class RemoteShTest(RemoteAccessTest):
  """Tests of basic RemoteSh functions"""
  TEST_CMD = 'ls'
  RETURN_CODE = 0
  OUTPUT = 'witty'
  ERROR = 'error'

  def assertRemoteShRaises(self, **kwargs):
    """Asserts that RunCommandError is raised when running TEST_CMD."""
    self.assertRaises(cros_build_lib.RunCommandError, self.host.RemoteSh,
                      self.TEST_CMD, **kwargs)

  def assertRemoteShRaisesSSHConnectionError(self, **kwargs):
    """Asserts that SSHConnectionError is raised when running TEST_CMD."""
    self.assertRaises(remote_access.SSHConnectionError, self.host.RemoteSh,
                      self.TEST_CMD, **kwargs)

  def SetRemoteShResult(self, returncode=RETURN_CODE, output=OUTPUT,
                        error=ERROR):
    """Sets the RemoteSh command results."""
    self.rsh_mock.AddCmdResult(self.TEST_CMD, returncode=returncode,
                               output=output, error=error)

  def testNormal(self):
    """Test normal functionality."""
    self.SetRemoteShResult()
    result = self.host.RemoteSh(self.TEST_CMD)
    self.assertEquals(result.returncode, self.RETURN_CODE)
    self.assertEquals(result.output.strip(), self.OUTPUT)
    self.assertEquals(result.error.strip(), self.ERROR)

  def testRemoteCmdFailure(self):
    """Test failure in remote cmd."""
    self.SetRemoteShResult(returncode=1)
    self.assertRemoteShRaises()
    self.assertRemoteShRaises(ssh_error_ok=True)
    self.host.RemoteSh(self.TEST_CMD, error_code_ok=True)
    self.host.RemoteSh(self.TEST_CMD, ssh_error_ok=True, error_code_ok=True)

  def testSshFailure(self):
    """Test failure in ssh command."""
    self.SetRemoteShResult(returncode=remote_access.SSH_ERROR_CODE)
    self.assertRemoteShRaisesSSHConnectionError()
    self.assertRemoteShRaisesSSHConnectionError(error_code_ok=True)
    self.host.RemoteSh(self.TEST_CMD, ssh_error_ok=True)
    self.host.RemoteSh(self.TEST_CMD, ssh_error_ok=True, error_code_ok=True)

  def testEnvLcMessagesSet(self):
    """Test that LC_MESSAGES is set to 'C' for an SSH command."""
    self.SetRemoteShResult()
    result = self.host.RemoteSh(self.TEST_CMD)
    rc_kwargs = result.rc_mock.call_args_list[-1][1]
    self.assertEqual(rc_kwargs['extra_env']['LC_MESSAGES'], 'C')

  def testEnvLcMessagesOverride(self):
    """Test that LC_MESSAGES is overridden to 'C' for an SSH command."""
    self.SetRemoteShResult()
    result = self.host.RemoteSh(self.TEST_CMD, extra_env={'LC_MESSAGES': 'fr'})
    rc_kwargs = result.rc_mock.call_args_list[-1][1]
    self.assertEqual(rc_kwargs['extra_env']['LC_MESSAGES'], 'C')


class CheckIfRebootedTest(RemoteAccessTest):
  """Tests of the CheckIfRebooted function."""

  _OLD_BOOT_ID = '1234'
  _NEW_BOOT_ID = '5678'

  def _SetCheckRebootResult(self, returncode=0, output='', error=''):
    """Sets the result object fields to mock a specific ssh command.

    The command is the one used to fetch the boot ID (cat /proc/sys/...)
    """
    self.rsh_mock.AddCmdResult(partial_mock.ListRegex('/proc/sys/.*'),
                               returncode=returncode,
                               output=output, error=error)
  def testSuccess(self):
    """Test the case of successful reboot."""
    self._SetCheckRebootResult(returncode=0, output=self._NEW_BOOT_ID)
    self.assertTrue(self.host.CheckIfRebooted(self._OLD_BOOT_ID))

  def testFailure(self):
    """Test case of failed reboot (boot ID did not change)."""
    self._SetCheckRebootResult(0, output=self._OLD_BOOT_ID)
    self.assertFalse(self.host.CheckIfRebooted(self._OLD_BOOT_ID))

  def testSshFailure(self):
    """Test case of reboot pending (ssh failed)."""
    self._SetCheckRebootResult(returncode=remote_access.SSH_ERROR_CODE)
    self.assertFalse(self.host.CheckIfRebooted(self._OLD_BOOT_ID))

  def testInvalidErrorCode(self):
    """Test case of bad error code returned."""
    self._SetCheckRebootResult(returncode=2)
    self.assertRaises(Exception,
                      lambda: self.host.CheckIfRebooted(self._OLD_BOOT_ID))


class RemoteDeviceTest(cros_test_lib.MockTestCase):
  """Tests for RemoteDevice class."""

  def setUp(self):
    self.rsh_mock = self.StartPatcher(RemoteShMock())
    self.pingable_mock = self.PatchObject(
        remote_access.RemoteDevice, 'Pingable', return_value=True)

  def _SetupRemoteTempDir(self):
    """Mock out the calls needed for a remote tempdir."""
    self.rsh_mock.AddCmdResult(partial_mock.In('mktemp'))
    self.rsh_mock.AddCmdResult(partial_mock.In('rm'))

  def testCommands(self):
    """Tests simple RunCommand() and BaseRunCommand() usage."""
    command = ['echo', 'foo']
    expected_output = 'foo'
    self.rsh_mock.AddCmdResult(command, output=expected_output)
    self._SetupRemoteTempDir()

    with remote_access.RemoteDeviceHandler('1.1.1.1') as device:
      self.assertEqual(expected_output,
                       device.RunCommand(['echo', 'foo']).output)
      self.assertEqual(expected_output,
                       device.BaseRunCommand(['echo', 'foo']).output)

  def testRunCommandShortCmdline(self):
    """Verify short command lines execute env settings directly."""
    with remote_access.RemoteDeviceHandler('1.1.1.1') as device:
      self.PatchObject(remote_access.RemoteDevice, 'CopyToWorkDir',
                       side_effect=Exception('should not be copying files'))
      self.rsh_mock.AddCmdResult(partial_mock.In('runit'))
      device.RunCommand(['runit'], extra_env={'VAR': 'val'})

  def testRunCommandLongCmdline(self):
    """Verify long command lines execute env settings via script."""
    with remote_access.RemoteDeviceHandler('1.1.1.1') as device:
      self._SetupRemoteTempDir()
      m = self.PatchObject(remote_access.RemoteDevice, 'CopyToWorkDir')
      self.rsh_mock.AddCmdResult(partial_mock.In('runit'))
      device.RunCommand(['runit'], extra_env={'VAR': 'v' * 1024 * 1024})
      # We'll assume that the test passed when it tries to copy a file to the
      # remote side (the shell script to run indirectly).
      self.assertEqual(m.call_count, 1)

  def testNoDeviceBaseDir(self):
    """Tests base_dir=None."""
    command = ['echo', 'foo']
    expected_output = 'foo'
    self.rsh_mock.AddCmdResult(command, output=expected_output)

    with remote_access.RemoteDeviceHandler('1.1.1.1', base_dir=None) as device:
      self.assertEqual(expected_output,
                       device.BaseRunCommand(['echo', 'foo']).output)

  def testDelayedRemoteDirs(self):
    """Tests the delayed creation of base_dir/work_dir."""
    with remote_access.RemoteDeviceHandler('1.1.1.1', base_dir='/f') as device:
      # Make sure we didn't talk to the remote yet.
      self.assertEqual(self.rsh_mock.call_count, 0)

      # The work dir will get automatically created when we use it.
      self.rsh_mock.AddCmdResult(partial_mock.In('mktemp'))
      _ = device.work_dir
      self.assertEqual(self.rsh_mock.call_count, 1)

      # Add a mock for the clean up logic.
      self.rsh_mock.AddCmdResult(partial_mock.In('rm'))

    self.assertEqual(self.rsh_mock.call_count, 2)
