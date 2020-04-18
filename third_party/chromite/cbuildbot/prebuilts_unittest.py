# -*- coding: utf-8 -*-
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for prebuilts."""

from __future__ import print_function

import mock
import os

from chromite.cbuildbot import cbuildbot_unittest
from chromite.lib import constants
from chromite.cbuildbot import prebuilts
from chromite.cbuildbot.stages import generic_stages_unittest
from chromite.lib import cros_test_lib
from chromite.lib import osutils

DEFAULT_CHROME_BRANCH = '27'

# pylint: disable=W0212
class PrebuiltTest(cros_test_lib.RunCommandTempDirTestCase):
  """Test general cbuildbot command methods."""

  def setUp(self):
    self._board = 'test-board'
    self._buildroot = self.tempdir
    self._overlays = ['%s/src/third_party/chromiumos-overlay' % self._buildroot]
    self._chroot = os.path.join(self._buildroot, 'chroot')
    os.makedirs(os.path.join(self._buildroot, '.repo'))

  def testUploadPrebuilts(self, builder_type=constants.PFQ_TYPE, private=False,
                          chrome_rev=None, version=None):
    """Test UploadPrebuilts with a public location."""
    prebuilts.UploadPrebuilts(builder_type, chrome_rev, private,
                              buildroot=self._buildroot, board=self._board,
                              version=version)
    self.assertCommandContains([builder_type, 'gs://chromeos-prebuilt'])

  def testUploadPrivatePrebuilts(self):
    """Test UploadPrebuilts with a private location."""
    self.testUploadPrebuilts(private=True)

  def testChromePrebuilts(self):
    """Test UploadPrebuilts for Chrome prebuilts."""
    self.testUploadPrebuilts(builder_type=constants.CHROME_PFQ_TYPE,
                             chrome_rev='tot')

  def testSdkPrebuilts(self):
    """Test UploadPrebuilts for SDK builds."""
    # A magical date for a magical time.
    version = '1994.04.02.000000'

    # Fake out toolchains overlay tarballs.
    tarball_dir = os.path.join(self._buildroot, constants.DEFAULT_CHROOT_DIR,
                               constants.SDK_OVERLAYS_OUTPUT)
    osutils.SafeMakedirs(tarball_dir)

    toolchain_overlay_tarball_args = []
    # Sample toolchain combos, corresponding to x86-alex and daisy.
    toolchain_combos = (
        ('i686-pc-linux-gnu',),
        ('armv7a-cros-linux-gnueabi', 'arm-none-eabi'),
    )
    for toolchains in ['-'.join(sorted(combo)) for combo in toolchain_combos]:
      tarball = 'built-sdk-overlay-toolchains-%s.tar.xz' % toolchains
      tarball_path = os.path.join(tarball_dir, tarball)
      osutils.Touch(tarball_path)
      tarball_arg = '%s:%s' % (toolchains, tarball_path)
      toolchain_overlay_tarball_args.append(['--toolchains-overlay-tarball',
                                             tarball_arg])

    # Fake out toolchain tarballs.
    tarball_dir = os.path.join(self._buildroot, constants.DEFAULT_CHROOT_DIR,
                               constants.SDK_TOOLCHAINS_OUTPUT)
    osutils.SafeMakedirs(tarball_dir)

    toolchain_tarball_args = []
    for tarball_base in ('i686', 'arm-none-eabi'):
      tarball = '%s.tar.xz' % tarball_base
      tarball_path = os.path.join(tarball_dir, tarball)
      osutils.Touch(tarball_path)
      tarball_arg = '%s:%s' % (tarball_base, tarball_path)
      toolchain_tarball_args.append(['--toolchain-tarball', tarball_arg])

    self.testUploadPrebuilts(builder_type=constants.CHROOT_BUILDER_TYPE,
                             version=version)
    self.assertCommandContains([
        '--toolchains-overlay-upload-path',
        '1994/04/cros-sdk-overlay-toolchains-%%(toolchains)s-'
        '%(version)s.tar.xz'])
    self.assertCommandContains(['--toolchain-upload-path',
                                '1994/04/%%(target)s-%(version)s.tar.xz'])
    for args in toolchain_overlay_tarball_args + toolchain_tarball_args:
      self.assertCommandContains(args)
    self.assertCommandContains(['--set-version', version])
    self.assertCommandContains(['--prepackaged-tarball',
                                os.path.join(self._buildroot,
                                             'built-sdk.tar.xz')])

  def testDevInstallerPrebuilts(self, packages=('package1', 'package2')):
    """Test UploadDevInstallerPrebuilts."""
    args = ['gs://dontcare', 'some_path_to_key', 'https://my_test/location']
    with mock.patch.object(prebuilts, '_AddPackagesForPrebuilt',
                           return_value=packages):
      prebuilts.UploadDevInstallerPrebuilts(*args, buildroot=self._buildroot,
                                            board=self._board)
    self.assertCommandContains([constants.CANARY_TYPE] + args[2:] + args[0:2])

  def testAddPackagesForPrebuilt(self):
    """Test AddPackagesForPrebuilt."""
    self.assertEqual(prebuilts._AddPackagesForPrebuilt('/'), None)

    data = """# comment!
cat/pkg-0
ca-t2/pkg2-123
ca-t3/pk-g4-4.0.1-r333
"""
    pkgs = [
        'cat/pkg',
        'ca-t2/pkg2',
        'ca-t3/pk-g4',
    ]
    cmds = ['--packages=' + x for x in pkgs]
    f = os.path.join(self.tempdir, 'package.provided')
    osutils.WriteFile(f, data)
    self.assertEqual(prebuilts._AddPackagesForPrebuilt(f), cmds)

  def testMissingDevInstallerFile(self):
    """Test that we raise an exception when the installer file is missing."""
    self.assertRaises(prebuilts.PackageFileMissing,
                      self.testDevInstallerPrebuilts, packages=())


# pylint: disable=too-many-ancestors
class BinhostConfWriterTest(
    generic_stages_unittest.RunCommandAbstractStageTestCase,
    cbuildbot_unittest.SimpleBuilderTestCase):
  """Tests for the BinhostConfWriter class."""

  cmd = 'upload_prebuilts'
  RELEASE_TAG = '1234.5.6'
  VERSION = 'R%s-%s' % (DEFAULT_CHROME_BRANCH, RELEASE_TAG)

  def _Prepare(self, bot_id=None, **kwargs):
    super(BinhostConfWriterTest, self)._Prepare(bot_id, **kwargs)
    self.cmd = os.path.join(self.build_root, constants.CHROMITE_BIN_SUBDIR,
                            'upload_prebuilts')
    self._run.options.prebuilts = True

  def _Run(self, build_config):
    """Prepare and run a BinhostConfWriter.

    Args:
      build_config: Name of build config to run for.
    """
    self._Prepare(build_config)
    confwriter = prebuilts.BinhostConfWriter(self._run)
    confwriter.Perform()

  def ConstructStage(self):
    pass

  def _VerifyResults(self, public_slave_boards=(), private_slave_boards=()):
    """Verify that the expected prebuilt commands were run.

    Do various assertions on the two RunCommands that were run by stage.
    There should be one private (--private) and one public (default) run.

    Args:
      public_slave_boards: List of public slave boards.
      private_slave_boards: List of private slave boards.
    """
    # TODO(mtennant): Add functionality in partial_mock to support more flexible
    # asserting.  For example here, asserting that '--sync-host' appears in
    # the command that did not include '--public'.

    # Some args are expected for any public run.
    if public_slave_boards:
      # It would be nice to confirm that --private is not in command, but note
      # that --sync-host should not appear in the --private command.
      cmd = [self.cmd, '--sync-binhost-conf', '--sync-host']
      self.assertCommandContains(cmd, expected=True)

    # Some args are expected for any private run.
    if private_slave_boards:
      cmd = [self.cmd, '--sync-binhost-conf', '--private']
      self.assertCommandContains(cmd, expected=True)

    # Assert public slave boards are mentioned in public run.
    for board in public_slave_boards:
      # This check does not actually confirm that this board was in the public
      # run rather than the private run, unfortunately.
      cmd = [self.cmd, '--slave-board', board]
      self.assertCommandContains(cmd, expected=True)

    # Assert private slave boards are mentioned in private run.
    for board in private_slave_boards:
      cmd = [self.cmd, '--slave-board', board, '--private']
      self.assertCommandContains(cmd, expected=True)

    # We expect --set-version so long as build config has manifest_version=True.
    self.assertCommandContains([self.cmd, '--set-version', self.VERSION],
                               expected=self._run.config.manifest_version)

  def testMasterPaladinUpload(self):
    self._Run('master-paladin')

    # Provide a sample of private/public slave boards that are expected.
    public_slave_boards = ('amd64-generic', 'daisy')
    private_slave_boards = ('cyan', 'samus', 'daisy_spring')

    self._VerifyResults(public_slave_boards=public_slave_boards,
                        private_slave_boards=private_slave_boards)

  def testMasterPaladinExperimentalBuilders(self):
    """Tests that commands are not run for experimental builders."""
    self._Prepare('master-paladin')
    confwriter = prebuilts.BinhostConfWriter(self._run)
    self._run.attrs.metadata.UpdateWithDict({
        constants.METADATA_EXPERIMENTAL_BUILDERS: ['samus', 'daisy']
    })
    confwriter.Perform()

    # Provide a sample of private/public slave boards that are expected.
    public_slave_boards = ('amd64-generic',)
    private_slave_boards = ('cyan', 'daisy_spring')

    self._VerifyResults(public_slave_boards=public_slave_boards,
                        private_slave_boards=private_slave_boards)

  def testMasterChromiumPFQUpload(self):
    self._Run('master-chromium-pfq')

    # Provide a sample of private/public slave boards that are expected.
    public_slave_boards = ('amd64-generic', 'daisy')
    private_slave_boards = ('cyan', 'daisy_skate', 'peppy')

    self._VerifyResults(public_slave_boards=public_slave_boards,
                        private_slave_boards=private_slave_boards)
