# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module tests the cros build command."""

from __future__ import print_function

from chromite.cli import command
from chromite.cli import command_unittest
from chromite.cli.cros import cros_build
from chromite.lib import chroot_util
from chromite.lib import cros_logging as logging
from chromite.lib import cros_test_lib
from chromite.lib import parallel_unittest
from chromite.lib import partial_mock
from chromite.lib import workon_helper


class MockBuildCommand(command_unittest.MockCommand):
  """Mock out the build command."""
  TARGET = 'chromite.cli.cros.cros_build.BuildCommand'
  TARGET_CLASS = cros_build.BuildCommand

  def __init__(self, *args, **kwargs):
    super(MockBuildCommand, self).__init__(*args, **kwargs)
    self.chroot_update_called = 0

  def OnChrootUpdate(self, *_args, **_kwargs):
    self.chroot_update_called += 1

  def Run(self, inst):
    self.PatchObject(chroot_util, 'UpdateChroot',
                     side_effect=self.OnChrootUpdate)
    self.PatchObject(chroot_util, 'Emerge')
    with parallel_unittest.ParallelMock():
      command_unittest.MockCommand.Run(self, inst)


class FakeWorkonHelper(object):
  """Fake workon_helper.WorkonHelper."""

  def __init__(self, *_args, **_kwargs):
    self.start_called = 0
    self.use_workon_only = None

  def ListAtoms(self, *_args, **_kwargs):
    pass

  def StartWorkingOnPackages(self, *_args, **kwargs):
    self.start_called += 1
    self.use_workon_only = kwargs.get('use_workon_only')


class BuildCommandTest(cros_test_lib.MockTempDirTestCase,
                       cros_test_lib.OutputTestCase):
  """Test class for our BuildCommand class."""

  def testBrilloBuildOperationCalled(self):
    """Test that BrilloBuildOperation is used when appropriate."""
    cmd = ['--board=randonname', 'power_manager']
    self.PatchObject(workon_helper, 'WorkonHelper')
    self.PatchObject(command, 'UseProgressBar', return_value=True)
    with MockBuildCommand(cmd) as build:
      operation_run = self.PatchObject(cros_build.BrilloBuildOperation, 'Run')
      build.inst.Run()
      self.assertTrue(operation_run.called)

  def testBrilloBuildOperationNotCalled(self):
    """Test that BrilloBuildOperation is not used when it shouldn't be."""
    cmd = ['--board=randonname', 'power_manager']
    self.PatchObject(workon_helper, 'WorkonHelper')
    self.PatchObject(command, 'UseProgressBar', return_value=False)
    with MockBuildCommand(cmd) as build:
      operation_run = self.PatchObject(cros_build.BrilloBuildOperation, 'Run')
      build.inst.Run()
      self.assertFalse(operation_run.called)

  def testSuccess(self):
    """Test that successful commands work."""
    cmds = [['--host', 'power_manager'],
            ['--board=randomname', 'power_manager'],
            ['--board=randomname', '--debug', 'power_manager'],
            ['--board=randomname', '--no-deps', 'power_manager'],
            ['--board=randomname', '--no-chroot-update', 'power_manager'],
            ['--board=randomname', '--no-enable-only-latest', 'power_manager']]
    for cmd in cmds:
      update_chroot = not ('--no-deps' in cmd or '--no-chroot-update' in cmd)
      enable_only_latest = '--no-enable-only-latest' not in cmd
      fake_workon_helper = FakeWorkonHelper()
      self.PatchObject(workon_helper, 'WorkonHelper',
                       return_value=fake_workon_helper)
      with MockBuildCommand(cmd) as build:
        build.inst.Run()
        self.assertEquals(1 if update_chroot else 0, build.chroot_update_called)
        self.assertEquals(1 if enable_only_latest else 0,
                          fake_workon_helper.start_called)
        self.assertEquals(True if enable_only_latest else None,
                          fake_workon_helper.use_workon_only)

  def testFailedDeps(self):
    """Test that failures are detected correctly."""
    # pylint: disable=protected-access
    args = ['--board=randomname', 'power_manager']
    self.PatchObject(workon_helper, 'WorkonHelper',
                     return_value=FakeWorkonHelper())
    with MockBuildCommand(args) as build:
      cmd = partial_mock.In('--backtrack=0')
      build.rc_mock.AddCmdResult(cmd=cmd, returncode=1, error='error\n')
      with self.OutputCapturer():
        try:
          build.inst.Run()
        except Exception as e:
          logging.error(e)
      self.AssertOutputContainsError(cros_build.BuildCommand._BAD_DEPEND_MSG,
                                     check_stderr=True)
