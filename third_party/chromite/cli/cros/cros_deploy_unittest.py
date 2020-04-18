# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module tests the cros deploy command."""

from __future__ import print_function

from chromite.cli import command_unittest
from chromite.cli import deploy
from chromite.cli.cros import cros_deploy
from chromite.lib import commandline
from chromite.lib import cros_test_lib


# pylint: disable=protected-access


class MockDeployCommand(command_unittest.MockCommand):
  """Mock out the deploy command."""
  TARGET = 'chromite.cli.cros.cros_deploy.DeployCommand'
  TARGET_CLASS = cros_deploy.DeployCommand
  COMMAND = 'deploy'

  def __init__(self, *args, **kwargs):
    command_unittest.MockCommand.__init__(self, *args, **kwargs)

  def Run(self, inst):
    command_unittest.MockCommand.Run(self, inst)


class CrosDeployTest(cros_test_lib.MockTempDirTestCase,
                     cros_test_lib.OutputTestCase):
  """Test calling `cros deploy` with various arguments.

  These tests just check that arguments as specified on the command
  line are properly passed through to deploy. Testing the
  actual update flow should be done in the deploy unit tests.
  """

  DEVICE = '1.1.1.1'
  PACKAGES = ['foo', 'bar']

  def SetupCommandMock(self, cmd_args):
    """Setup comand mock."""
    self.cmd_mock = MockDeployCommand(
        cmd_args, base_args=['--cache-dir', self.tempdir])
    self.StartPatcher(self.cmd_mock)

  def setUp(self):
    """Patches objects."""
    self.cmd_mock = None
    self.deploy_mock = self.PatchObject(deploy, 'Deploy', autospec=True)
    self.run_inside_chroot_mock = self.PatchObject(
        commandline, 'RunInsideChroot', autospec=True)

  def VerifyDeployParameters(self, device, packages, **kwargs):
    """Verifies the arguments passed to Deployer.Run().

    This function helps verify that command line specifications are
    parsed properly.

    Args:
      device: expected device hostname.
      packages: expected packages list.
      kwargs: keyword arguments expected in the call to Deployer.Run().
          Arguments unspecified here are checked against their default
          value for `cros deploy`.
    """
    deploy_args, deploy_kwargs = self.deploy_mock.call_args
    self.assertEqual(device, deploy_args[0].hostname)
    self.assertListEqual(packages, deploy_args[1])
    # `cros deploy` default options. Must match AddParser().
    expected_kwargs = {
        'board': None,
        'strip': True,
        'emerge': True,
        'root': '/',
        'clean_binpkg': True,
        'emerge_args': None,
        'ssh_private_key': None,
        'ping': True,
        'dry_run': False,
        'force': False,
        'update': False,
        'deep': False,
        'deep_rev': False}
    # Overwrite defaults with any variations in this test.
    expected_kwargs.update(kwargs)
    self.assertDictEqual(expected_kwargs, deploy_kwargs)

  def testDefaults(self):
    """Tests `cros deploy` default values."""
    self.SetupCommandMock([self.DEVICE] + self.PACKAGES)
    self.cmd_mock.inst.Run()
    self.assertTrue(self.run_inside_chroot_mock.called)
    self.VerifyDeployParameters(self.DEVICE, self.PACKAGES)

  def testDeployError(self):
    """Tests that DeployErrors are passed through."""
    with self.OutputCapturer():
      self.SetupCommandMock([self.DEVICE] + self.PACKAGES)
      self.deploy_mock.side_effect = deploy.DeployError
      with self.assertRaises(deploy.DeployError):
        self.cmd_mock.inst.Run()
