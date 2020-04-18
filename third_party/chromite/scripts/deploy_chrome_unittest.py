# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for the deploy_chrome script."""

from __future__ import print_function

import mock
import os
import time

from chromite.cli.cros import cros_chrome_sdk_unittest
from chromite.lib import chrome_util
from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import osutils
from chromite.lib import partial_mock
from chromite.lib import remote_access
from chromite.lib import remote_access_unittest
from chromite.scripts import deploy_chrome


# pylint: disable=W0212

_REGULAR_TO = ('--to', 'monkey')
_GS_PATH = 'gs://foon'


def _ParseCommandLine(argv):
  return deploy_chrome._ParseCommandLine(['--log-level', 'debug'] + argv)


class InterfaceTest(cros_test_lib.OutputTestCase):
  """Tests the commandline interface of the script."""

  BOARD = 'eve'

  def testGsLocalPathUnSpecified(self):
    """Test no chrome path specified."""
    with self.OutputCapturer():
      self.assertRaises2(SystemExit, _ParseCommandLine, list(_REGULAR_TO),
                         check_attrs={'code': 2})

  def testGsPathSpecified(self):
    """Test case of GS path specified."""
    argv = list(_REGULAR_TO) + ['--gs-path', _GS_PATH]
    _ParseCommandLine(argv)

  def testLocalPathSpecified(self):
    """Test case of local path specified."""
    argv = list(_REGULAR_TO) + ['--local-pkg-path', '/path/to/chrome']
    _ParseCommandLine(argv)

  def testNoTarget(self):
    """Test no target specified."""
    argv = ['--gs-path', _GS_PATH]
    self.assertParseError(argv)

  def assertParseError(self, argv):
    with self.OutputCapturer():
      self.assertRaises2(SystemExit, _ParseCommandLine, argv,
                         check_attrs={'code': 2})

  def testNoBoard(self):
    """Test cases where --board is not specified."""
    argv = ['--staging-only', '--build-dir=/path/to/nowhere']
    self.assertParseError(argv)

    # Don't need --board if no stripping is necessary.
    argv_nostrip = argv + ['--nostrip']
    _ParseCommandLine(argv_nostrip)

    # Don't need --board if strip binary is provided.
    argv_strip_bin = argv + ['--strip-bin', 'strip.bin']
    _ParseCommandLine(argv_strip_bin)

  def testMountOptionSetsTargetDir(self):
    argv = list(_REGULAR_TO) + ['--gs-path', _GS_PATH, '--mount']
    options = _ParseCommandLine(argv)
    self.assertIsNot(options.target_dir, None)

  def testMountOptionSetsMountDir(self):
    argv = list(_REGULAR_TO) + ['--gs-path', _GS_PATH, '--mount']
    options = _ParseCommandLine(argv)
    self.assertIsNot(options.mount_dir, None)

  def testMountOptionDoesNotOverrideTargetDir(self):
    argv = list(_REGULAR_TO) + ['--gs-path', _GS_PATH, '--mount',
                                '--target-dir', '/foo/bar/cow']
    options = _ParseCommandLine(argv)
    self.assertEqual(options.target_dir, '/foo/bar/cow')

  def testMountOptionDoesNotOverrideMountDir(self):
    argv = list(_REGULAR_TO) + ['--gs-path', _GS_PATH, '--mount',
                                '--mount-dir', '/foo/bar/cow']
    options = _ParseCommandLine(argv)
    self.assertEqual(options.mount_dir, '/foo/bar/cow')

  def testSshIdentityOptionSetsOption(self):
    argv = list(_REGULAR_TO) + ['--private-key', '/foo/bar/key',
                                '--board', 'cedar',
                                '--build-dir', '/path/to/nowhere']
    options = _ParseCommandLine(argv)
    self.assertEqual(options.private_key, '/foo/bar/key')

class DeployChromeMock(partial_mock.PartialMock):
  """Deploy Chrome Mock Class."""

  TARGET = 'chromite.scripts.deploy_chrome.DeployChrome'
  ATTRS = ('_KillProcsIfNeeded', '_DisableRootfsVerification')

  def __init__(self):
    partial_mock.PartialMock.__init__(self)
    self.remote_device_mock = remote_access_unittest.RemoteDeviceMock()
    # Target starts off as having rootfs verification enabled.
    self.rsh_mock = remote_access_unittest.RemoteShMock()
    self.rsh_mock.SetDefaultCmdResult(0)
    self.MockMountCmd(1)
    self.rsh_mock.AddCmdResult(
        deploy_chrome.LSOF_COMMAND % (deploy_chrome._CHROME_DIR,), 1)

  def MockMountCmd(self, returnvalue):
    self.rsh_mock.AddCmdResult(deploy_chrome.MOUNT_RW_COMMAND,
                               returnvalue)

  def _DisableRootfsVerification(self, inst):
    with mock.patch.object(time, 'sleep'):
      self.backup['_DisableRootfsVerification'](inst)

  def PreStart(self):
    self.remote_device_mock.start()
    self.rsh_mock.start()

  def PreStop(self):
    self.rsh_mock.stop()
    self.remote_device_mock.stop()

  def _KillProcsIfNeeded(self, _inst):
    # Fully stub out for now.
    pass


class DeployTest(cros_test_lib.MockTempDirTestCase):
  """Setup a deploy object with a GS-path for use in tests."""

  def _GetDeployChrome(self, args):
    options = _ParseCommandLine(args)
    return deploy_chrome.DeployChrome(
        options, self.tempdir, os.path.join(self.tempdir, 'staging'))

  def setUp(self):
    self.deploy_mock = self.StartPatcher(DeployChromeMock())
    self.deploy = self._GetDeployChrome(
        list(_REGULAR_TO) + ['--gs-path', _GS_PATH, '--force'])
    self.remote_reboot_mock = \
      self.PatchObject(remote_access.RemoteAccess, 'RemoteReboot',
                       return_value=True)

class TestDisableRootfsVerification(DeployTest):
  """Testing disabling of rootfs verification and RO mode."""

  def testDisableRootfsVerificationSuccess(self):
    """Test the working case, disabling rootfs verification."""
    self.deploy_mock.MockMountCmd(0)
    self.deploy._DisableRootfsVerification()
    self.assertFalse(self.deploy._target_dir_is_still_readonly.is_set())

  def testDisableRootfsVerificationFailure(self):
    """Test failure to disable rootfs verification."""
    #pylint: disable=unused-argument
    def RaiseRunCommandError(timeout_sec=None):
      raise cros_build_lib.RunCommandError('Mock RunCommandError', 0)
    self.remote_reboot_mock.side_effect = RaiseRunCommandError
    self.assertRaises(cros_build_lib.RunCommandError,
                      self.deploy._DisableRootfsVerification)
    self.remote_reboot_mock.side_effect = None
    self.assertFalse(self.deploy._target_dir_is_still_readonly.is_set())


class TestMount(DeployTest):
  """Testing mount success and failure."""

  def testSuccess(self):
    """Test case where we are able to mount as writable."""
    self.assertFalse(self.deploy._target_dir_is_still_readonly.is_set())
    self.deploy_mock.MockMountCmd(0)
    self.deploy._MountRootfsAsWritable()
    self.assertFalse(self.deploy._target_dir_is_still_readonly.is_set())

  def testMountError(self):
    """Test that mount failure doesn't raise an exception by default."""
    self.assertFalse(self.deploy._target_dir_is_still_readonly.is_set())
    self.PatchObject(remote_access.RemoteDevice, 'IsDirWritable',
                     return_value=False, autospec=True)
    self.deploy._MountRootfsAsWritable()
    self.assertTrue(self.deploy._target_dir_is_still_readonly.is_set())

  def testMountRwFailure(self):
    """Test that mount failure raises an exception if error_code_ok=False."""
    self.assertRaises(cros_build_lib.RunCommandError,
                      self.deploy._MountRootfsAsWritable, error_code_ok=False)
    self.assertFalse(self.deploy._target_dir_is_still_readonly.is_set())

  def testMountTempDir(self):
    """Test that mount succeeds if target dir is writable."""
    self.assertFalse(self.deploy._target_dir_is_still_readonly.is_set())
    self.PatchObject(remote_access.RemoteDevice, 'IsDirWritable',
                     return_value=True, autospec=True)
    self.deploy._MountRootfsAsWritable()
    self.assertFalse(self.deploy._target_dir_is_still_readonly.is_set())


class TestUiJobStarted(DeployTest):
  """Test detection of a running 'ui' job."""

  def MockStatusUiCmd(self, **kwargs):
    self.deploy_mock.rsh_mock.AddCmdResult('status ui', **kwargs)

  def testUiJobStartedFalse(self):
    """Correct results with a stopped job."""
    self.MockStatusUiCmd(output='ui stop/waiting')
    self.assertFalse(self.deploy._CheckUiJobStarted())

  def testNoUiJob(self):
    """Correct results when the job doesn't exist."""
    self.MockStatusUiCmd(error='start: Unknown job: ui', returncode=1)
    self.assertFalse(self.deploy._CheckUiJobStarted())

  def testCheckRootfsWriteableTrue(self):
    """Correct results with a running job."""
    self.MockStatusUiCmd(output='ui start/running, process 297')
    self.assertTrue(self.deploy._CheckUiJobStarted())


class StagingTest(cros_test_lib.MockTempDirTestCase):
  """Test user-mode and ebuild-mode staging functionality."""

  def setUp(self):
    self.staging_dir = os.path.join(self.tempdir, 'staging')
    self.build_dir = os.path.join(self.tempdir, 'build_dir')
    self.common_flags = ['--build-dir', self.build_dir,
                         '--board=eve', '--staging-only', '--cache-dir',
                         self.tempdir]
    self.sdk_mock = self.StartPatcher(cros_chrome_sdk_unittest.SDKFetcherMock())
    self.PatchObject(
        osutils, 'SourceEnvironment', autospec=True,
        return_value={'STRIP': 'x86_64-cros-linux-gnu-strip'})

  def testSingleFileDeployFailure(self):
    """Default staging enforces that mandatory files are copied"""
    options = _ParseCommandLine(self.common_flags)
    osutils.Touch(os.path.join(self.build_dir, 'chrome'), makedirs=True)
    self.assertRaises(
        chrome_util.MissingPathError, deploy_chrome._PrepareStagingDir,
        options, self.tempdir, self.staging_dir, chrome_util._COPY_PATHS_CHROME)

  def testSloppyDeployFailure(self):
    """Sloppy staging enforces that at least one file is copied."""
    options = _ParseCommandLine(self.common_flags + ['--sloppy'])
    self.assertRaises(
        chrome_util.MissingPathError, deploy_chrome._PrepareStagingDir,
        options, self.tempdir, self.staging_dir, chrome_util._COPY_PATHS_CHROME)

  def testSloppyDeploySuccess(self):
    """Sloppy staging - stage one file."""
    options = _ParseCommandLine(self.common_flags + ['--sloppy'])
    osutils.Touch(os.path.join(self.build_dir, 'chrome'), makedirs=True)
    deploy_chrome._PrepareStagingDir(options, self.tempdir, self.staging_dir,
                                     chrome_util._COPY_PATHS_CHROME)


class DeployTestBuildDir(cros_test_lib.MockTempDirTestCase):
  """Set up a deploy object with a build-dir for use in deployment type tests"""

  def _GetDeployChrome(self, args):
    options = _ParseCommandLine(args)
    return deploy_chrome.DeployChrome(
        options, self.tempdir, os.path.join(self.tempdir, 'staging'))

  def setUp(self):
    self.staging_dir = os.path.join(self.tempdir, 'staging')
    self.build_dir = os.path.join(self.tempdir, 'build_dir')
    self.deploy_mock = self.StartPatcher(DeployChromeMock())
    self.deploy = self._GetDeployChrome(
        list(_REGULAR_TO) + ['--build-dir', self.build_dir,
                             '--board=eve', '--staging-only', '--cache-dir',
                             self.tempdir, '--sloppy'])

  def getCopyPath(self, source_path):
    """Return a chrome_util.Path or None if not present."""
    paths = [p for p in self.deploy.copy_paths if p.src == source_path]
    return paths[0] if paths else None

class TestDeploymentType(DeployTestBuildDir):
  """Test detection of deployment type using build dir."""

  def testAppShellDetection(self):
    """Check for an app_shell deployment"""
    osutils.Touch(os.path.join(self.deploy.options.build_dir, 'app_shell'),
                  makedirs=True)
    self.deploy._CheckDeployType()
    self.assertTrue(self.getCopyPath('app_shell'))
    self.assertFalse(self.getCopyPath('chrome'))

  def testChromeAndAppShellDetection(self):
    """Check for a chrome deployment when app_shell also exists."""
    osutils.Touch(os.path.join(self.deploy.options.build_dir, 'chrome'),
                  makedirs=True)
    osutils.Touch(os.path.join(self.deploy.options.build_dir, 'app_shell'),
                  makedirs=True)
    self.deploy._CheckDeployType()
    self.assertTrue(self.getCopyPath('chrome'))
    self.assertFalse(self.getCopyPath('app_shell'))

  def testChromeDetection(self):
    """Check for a regular chrome deployment"""
    osutils.Touch(os.path.join(self.deploy.options.build_dir, 'chrome'),
                  makedirs=True)
    self.deploy._CheckDeployType()
    self.assertTrue(self.getCopyPath('chrome'))
    self.assertFalse(self.getCopyPath('app_shell'))
