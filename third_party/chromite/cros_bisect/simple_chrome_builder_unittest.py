# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test simple_chrome_builder  module."""

from __future__ import print_function

import mock
import os

from chromite.cros_bisect import simple_chrome_builder
from chromite.cbuildbot import commands
from chromite.lib import commandline
from chromite.lib import cros_test_lib
from chromite.lib import gclient
from chromite.lib import git
from chromite.lib import osutils


class TestSimpleChromeBuilder(cros_test_lib.MockTempDirTestCase):
  """Tests AutotestEvaluator class."""
  BOARD = 'samus'
  DUT_IP = '192.168.1.1'
  DUT = commandline.DeviceParser(commandline.DEVICE_SCHEME_SSH)(DUT_IP)

  def setUp(self):
    self.default_chromium_dir = os.path.join(self.tempdir, 'chromium')
    self.default_repo_dir = os.path.join(self.tempdir, 'chromium', 'src')
    self.default_archive_base = os.path.join(self.tempdir, 'build')
    self.gclient_path = os.path.join(self.tempdir, 'gclient')
    self.log_output_args = {'log_output': True}

  def GetBuilder(self, base_dir=None, board=None, reuse_repo=True,
                 chromium_dir=None, build_dir=None, archive_build=True,
                 reuse_build=True):
    """Obtains a SimpleChromeBuilder instance.

    Args:
      base_dir: Base directory. Default self.tempdir.
      board: Board name. Default self.BOARD.
      reuse_repo: True to reuse repo.
      chromium_dir: Optional. If specified, use the chromium repo the path
          points to.
      build_dir: Optional. Store build result to it if specified.
      archive_build: True to archive build.
      reuse_build: True to reuse previous build.

    Returns:
      A SimpleChromeBuilder instance.
    """
    if base_dir is None:
      base_dir = self.tempdir
    if board is None:
      board = self.BOARD
    options = cros_test_lib.EasyAttr(
        base_dir=base_dir, board=board, reuse_repo=reuse_repo,
        chromium_dir=chromium_dir, build_dir=build_dir,
        archive_build=archive_build, reuse_build=reuse_build)
    builder = simple_chrome_builder.SimpleChromeBuilder(options)

    # Override gclient path.
    builder.gclient = self.gclient_path

    return builder

  def testInit(self):
    builder = self.GetBuilder()
    base_dir = self.tempdir
    self.assertEqual(base_dir, builder.base_dir)
    self.assertEqual(self.default_chromium_dir, builder.chromium_dir)
    self.assertEqual(self.default_repo_dir, builder.repo_dir)
    self.assertTrue(builder.reuse_repo)
    self.assertTrue(builder.reuse_build)
    self.assertTrue(builder.archive_build)
    self.assertEqual(self.default_archive_base, builder.archive_base)
    self.assertTrue(os.path.isdir(builder.archive_base))
    self.assertDictEqual(self.log_output_args, builder.log_output_args)

  def testInitMissingRequiredArgs(self):
    options = cros_test_lib.EasyAttr()
    with self.assertRaises(Exception) as cm:
      simple_chrome_builder.SimpleChromeBuilder(options)
    exception_message = cm.exception.message
    self.assertTrue('Missing command line' in exception_message)
    self.assertTrue('SimpleChromeBuilder' in exception_message)
    for arg in simple_chrome_builder.SimpleChromeBuilder.REQUIRED_ARGS:
      self.assertTrue(arg in exception_message)

  def testInitCustomizedDir(self):
    base_dir = self.tempdir
    chromium_dir = os.path.join(base_dir, 'another_chromium')
    build_dir = os.path.join(base_dir, 'another_build')
    builder = self.GetBuilder(chromium_dir=chromium_dir, build_dir=build_dir)

    self.assertEqual(base_dir, builder.base_dir)
    self.assertEqual(chromium_dir, builder.chromium_dir)
    self.assertEqual(os.path.join(chromium_dir, 'src'), builder.repo_dir)
    self.assertTrue(builder.reuse_repo)
    self.assertTrue(builder.reuse_build)
    self.assertTrue(builder.archive_build)
    self.assertEqual(build_dir, builder.archive_base)
    self.assertTrue(os.path.isdir(builder.archive_base))
    self.assertDictEqual(self.log_output_args, builder.log_output_args)

  def testInitFlipFlags(self):
    builder = self.GetBuilder(reuse_repo=False, archive_build=False,
                              reuse_build=False)
    base_dir = self.tempdir
    self.assertEqual(base_dir, builder.base_dir)
    self.assertEqual(self.default_chromium_dir, builder.chromium_dir)
    self.assertEqual(self.default_repo_dir, builder.repo_dir)
    self.assertFalse(builder.reuse_repo)
    self.assertFalse(builder.reuse_build)
    self.assertFalse(builder.archive_build)
    self.assertEqual(self.default_archive_base, builder.archive_base)
    self.assertFalse(os.path.isdir(builder.archive_base))
    self.assertDictEqual(self.log_output_args, builder.log_output_args)

  def testSetUp(self):
    command_mock = self.StartPatcher(cros_test_lib.RunCommandMock())
    command_mock.AddCmdResult(['fetch', '--nohooks', 'chromium'])
    write_config_mock = self.PatchObject(gclient, 'WriteConfigFile')
    git_mock = self.PatchObject(git, 'RunGit')
    gsync_mock = self.PatchObject(gclient, 'Sync')

    builder = self.GetBuilder()
    builder.SetUp()

    write_config_mock.assert_called_with(
        self.gclient_path, self.default_chromium_dir, True, None, managed=False)
    git_mock.assert_called_with(self.default_repo_dir,
                                ['pull', 'origin', 'master'])
    gsync_mock.assert_called_with(
        self.gclient_path, self.default_chromium_dir, reset=True, nohooks=True,
        verbose=False, run_args=self.log_output_args)

  def testSetUpSkip(self):
    write_config_mock = self.PatchObject(gclient, 'WriteConfigFile')
    git_mock = self.PatchObject(git, 'RunGit')
    gsync_mock = self.PatchObject(gclient, 'Sync')

    # Make it looks like a git repo.
    osutils.SafeMakedirs(os.path.join(self.default_repo_dir, '.git'))

    builder = self.GetBuilder()
    builder.SetUp()

    write_config_mock.assert_not_called()
    git_mock.assert_not_called()
    gsync_mock.assert_not_called()

  def testSetUpExistingRepoException(self):
    write_config_mock = self.PatchObject(gclient, 'WriteConfigFile')
    git_mock = self.PatchObject(git, 'RunGit')
    gsync_mock = self.PatchObject(gclient, 'Sync')

    # Make it looks like a git repo.
    osutils.SafeMakedirs(os.path.join(self.default_repo_dir, '.git'))

    builder = self.GetBuilder(reuse_repo=False)
    self.assertRaisesRegexp(Exception, 'Chromium repo exists.*',
                            builder.SetUp)

    write_config_mock.assert_not_called()
    git_mock.assert_not_called()
    gsync_mock.assert_not_called()

  def testSyncToHead(self):
    git_mock = self.PatchObject(git, 'CleanAndCheckoutUpstream')
    builder = self.GetBuilder()
    builder.SyncToHead()
    git_mock.assert_called_with(self.default_repo_dir)

  def testGclientSync(self):
    gsync_mock = self.PatchObject(gclient, 'Sync')

    builder = self.GetBuilder()
    builder.GclientSync()
    gsync_mock.assert_called_with(
        self.gclient_path, self.default_chromium_dir, reset=False,
        nohooks=False, verbose=False, run_args=self.log_output_args)

    builder.GclientSync(reset=True, nohooks=True)
    gsync_mock.assert_called_with(
        self.gclient_path, self.default_chromium_dir, reset=True,
        nohooks=True, verbose=False, run_args=self.log_output_args)

  def testBuildReuse(self):
    commit_label = 'test'

    # Let the build already be in archive.
    archive_path = os.path.join(
        self.default_archive_base, 'out_%s_%s' % (self.BOARD, commit_label),
        'Release')
    osutils.SafeMakedirs(archive_path)

    builder = self.GetBuilder()
    build_to_deploy = builder.Build(commit_label)
    self.assertEqual(archive_path, build_to_deploy)

  def _ChromeSdkRunSideEffect(self, *args, **unused_kwargs):
    if len(args) > 0 and len(args[0]) == 3:
      bash_command = args[0][2]
      if 'gn gen' in bash_command:
        build_dir = bash_command.split()[2]
        osutils.SafeMakedirs(os.path.join(self.default_repo_dir, build_dir))
      return mock.DEFAULT

  def testBuild(self):
    gsync_mock = self.PatchObject(simple_chrome_builder.SimpleChromeBuilder,
                                  'GclientSync')
    success_result = cros_test_lib.EasyAttr(returncode=0)
    chrome_sdk_run_mock = self.PatchObject(
        commands.ChromeSDK, 'Run', side_effect=self._ChromeSdkRunSideEffect,
        return_value=success_result)
    chrome_sdk_ninja_mock = self.PatchObject(
        commands.ChromeSDK, 'Ninja', return_value=success_result)

    commit_label = 'test'
    archive_path = os.path.join(
        self.default_archive_base, 'out_%s_%s' % (self.BOARD, commit_label),
        'Release')

    self.assertFalse(os.path.isdir(archive_path))
    builder = self.GetBuilder()
    build_to_deploy = builder.Build(commit_label)
    self.assertEqual(archive_path, build_to_deploy)
    # Check that build_to_deploy exists after builder.Build()
    self.assertTrue(os.path.isdir(archive_path))
    gsync_mock.assert_called()
    chrome_sdk_run_mock.assert_called_with(
        ['bash', '-c', 'gn gen out_%s/Release --args="$GN_ARGS"' % self.BOARD],
        run_args=self.log_output_args)
    chrome_sdk_ninja_mock.assert_called_with(
        targets=['chrome', 'chrome_sandbox', 'nacl_helper'],
        run_args=self.log_output_args)

  def testBuildNoArchive(self):
    gsync_mock = self.PatchObject(simple_chrome_builder.SimpleChromeBuilder,
                                  'GclientSync')
    success_result = cros_test_lib.EasyAttr(returncode=0)
    chrome_sdk_run_mock = self.PatchObject(
        commands.ChromeSDK, 'Run', side_effect=self._ChromeSdkRunSideEffect,
        return_value=success_result)
    chrome_sdk_ninja_mock = self.PatchObject(
        commands.ChromeSDK, 'Ninja', return_value=success_result)

    commit_label = 'test'
    archive_path = os.path.join(
        self.default_archive_base, 'out_%s_%s' % (self.BOARD, commit_label),
        'Release')

    self.assertFalse(os.path.isdir(archive_path))
    builder = self.GetBuilder(archive_build=False)
    build_to_deploy = builder.Build(commit_label)
    # No archive. Check that archive_path is not created.
    self.assertNotEqual(archive_path, build_to_deploy)
    self.assertFalse(os.path.isdir(archive_path))

    self.assertEqual(os.path.join('out_%s' % self.BOARD, 'Release'),
                     build_to_deploy)
    self.assertTrue(os.path.isdir(
        os.path.join(self.default_repo_dir, build_to_deploy)))
    gsync_mock.assert_called()
    chrome_sdk_run_mock.assert_called_with(
        ['bash', '-c', 'gn gen out_%s/Release --args="$GN_ARGS"' % self.BOARD],
        run_args=self.log_output_args)
    chrome_sdk_ninja_mock.assert_called_with(
        targets=['chrome', 'chrome_sandbox', 'nacl_helper'],
        run_args=self.log_output_args)

  def testDeploy(self):
    chrome_sdk_run_mock = self.PatchObject(commands.ChromeSDK, 'Run')
    build_to_deploy = os.path.join('out_%s' % self.BOARD, 'Release')
    commit_label = 'test'

    builder = self.GetBuilder()
    builder.Deploy(self.DUT, build_to_deploy, commit_label)
    chrome_sdk_run_mock.assert_called_with(
        ['deploy_chrome', '--build-dir', build_to_deploy, '--to', self.DUT_IP,
         '--force'],
        run_args=self.log_output_args)

  def testDeployWithPort(self):
    port = '9999'
    dut = commandline.DeviceParser(commandline.DEVICE_SCHEME_SSH)(
        self.DUT_IP + ':' + port)
    chrome_sdk_run_mock = self.PatchObject(commands.ChromeSDK, 'Run')
    build_to_deploy = os.path.join('out_%s' % self.BOARD, 'Release')
    commit_label = 'test'

    builder = self.GetBuilder()
    builder.Deploy(dut, build_to_deploy, commit_label)
    chrome_sdk_run_mock.assert_called_with(
        ['deploy_chrome', '--build-dir', build_to_deploy, '--to', self.DUT_IP,
         '--force', '--port', port],
        run_args=self.log_output_args)
