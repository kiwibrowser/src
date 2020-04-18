# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for build stages."""

from __future__ import print_function

import contextlib
import mock
import os
import tempfile

from chromite.cbuildbot import cbuildbot_unittest
from chromite.cbuildbot import chromeos_config
from chromite.cbuildbot import commands
from chromite.cbuildbot.stages import build_stages
from chromite.cbuildbot.stages import generic_stages_unittest
from chromite.lib.const import waterfall
from chromite.lib import auth
from chromite.lib import buildbucket_lib
from chromite.lib import build_summary
from chromite.lib import builder_status_lib
from chromite.lib import cidb
from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_sdk_lib
from chromite.lib import cros_test_lib
from chromite.lib import fake_cidb
from chromite.lib import osutils
from chromite.lib import parallel
from chromite.lib import parallel_unittest
from chromite.lib import partial_mock
from chromite.lib import path_util
from chromite.lib import portage_util

from chromite.cbuildbot.stages.generic_stages_unittest import patch
from chromite.cbuildbot.stages.generic_stages_unittest import patches


# pylint: disable=too-many-ancestors
# pylint: disable=protected-access

class InitSDKTest(generic_stages_unittest.RunCommandAbstractStageTestCase):
  """Test building the SDK"""

  def setUp(self):
    self.PatchObject(cros_sdk_lib, 'GetChrootVersion', return_value='12')
    self.cros_sdk = os.path.join(self.tempdir, 'buildroot',
                                 constants.CHROMITE_BIN_SUBDIR, 'cros_sdk')

  def ConstructStage(self):
    return build_stages.InitSDKStage(self._run)

  def testFullBuildWithExistingChroot(self):
    """Tests whether we create chroots for full builds."""
    self._PrepareFull()
    self._Run(dir_exists=True)
    self.assertCommandContains([self.cros_sdk])

  def testBinBuildWithMissingChroot(self):
    """Tests whether we create chroots when needed."""
    self._PrepareBin()
    # Do not force chroot replacement in build config.
    self._run._config.chroot_replace = False
    self._Run(dir_exists=False)
    self.assertCommandContains([self.cros_sdk])

  def testFullBuildWithMissingChroot(self):
    """Tests whether we create chroots when needed."""
    self._PrepareFull()
    self._Run(dir_exists=True)
    self.assertCommandContains([self.cros_sdk])

  def testFullBuildWithNoSDK(self):
    """Tests whether the --nosdk option works."""
    self._PrepareFull(extra_cmd_args=['--nosdk'])
    self._Run(dir_exists=False)
    self.assertCommandContains([self.cros_sdk, '--bootstrap'])

  def testBinBuildWithExistingChroot(self):
    """Tests whether the --nosdk option works."""
    self._PrepareFull(extra_cmd_args=['--nosdk'])
    # Do not force chroot replacement in build config.
    self._run._config.chroot_replace = False
    self._run._config.separate_debug_symbols = False
    self._run.config.useflags = ['foo']
    self._Run(dir_exists=True)
    self.assertCommandContains([self.cros_sdk], expected=False)
    self.assertCommandContains(['./run_chroot_version_hooks'],
                               enter_chroot=True, extra_env={'USE': 'foo'})


class SetupBoardTest(generic_stages_unittest.RunCommandAbstractStageTestCase):
  """Test building the board"""

  def ConstructStage(self):
    return build_stages.SetupBoardStage(self._run, self._current_board)

  def _RunFull(self, dir_exists=False):
    """Helper for testing a full builder."""
    self._Run(dir_exists)
    self.assertCommandContains(['./update_chroot'])
    cmd = ['./setup_board', '--board=%s' % self._current_board, '--nousepkg']
    self.assertCommandContains(cmd)
    cmd = ['./setup_board', '--skip_chroot_upgrade']
    self.assertCommandContains(cmd)

  def testFullBuildWithProfile(self):
    """Tests whether full builds add profile flag when requested."""
    self._PrepareFull(extra_config={'profile': 'foo'})
    self._RunFull(dir_exists=False)
    self.assertCommandContains(['./setup_board', '--profile=foo'])

  def testFullBuildWithOverriddenProfile(self):
    """Tests whether full builds add overridden profile flag when requested."""
    self._PrepareFull(extra_cmd_args=['--profile', 'smock'])
    self._RunFull(dir_exists=False)
    self.assertCommandContains(['./setup_board', '--profile=smock'])

  def _RunBin(self, dir_exists):
    """Helper for testing a binary builder."""
    self._Run(dir_exists)
    update_nousepkg = (not self._run.config.usepkg_toolchain or
                       self._run.options.latest_toolchain)
    self.assertCommandContains(['./update_chroot', '--nousepkg'],
                               expected=update_nousepkg)
    self.assertCommandContains(['./setup_board'])
    cmd = ['./setup_board', '--skip_chroot_upgrade']
    self.assertCommandContains(cmd)
    cmd = ['./setup_board', '--nousepkg']
    self.assertCommandContains(
        cmd, not self._run.config.usepkg_build_packages)

  def testBinBuildWithLatestToolchain(self):
    """Tests whether we use --nousepkg for creating the board."""
    self._PrepareBin()
    self._run.options.latest_toolchain = True
    self._RunBin(dir_exists=False)

  def testBinBuildWithLatestToolchainAndDirExists(self):
    """Tests whether we use --nousepkg for creating the board."""
    self._PrepareBin()
    self._run.options.latest_toolchain = True
    self._RunBin(dir_exists=True)

  def testBinBuildWithNoToolchainPackages(self):
    """Tests whether we use --nousepkg for creating the board."""
    self._PrepareBin()
    self._run.config.usepkg_toolchain = False
    self._RunBin(dir_exists=False)

  def testSDKBuild(self):
    """Tests whether we use --skip_chroot_upgrade for SDK builds."""
    extra_config = {'build_type': constants.CHROOT_BUILDER_TYPE}
    self._PrepareFull(extra_config=extra_config)
    self._Run(dir_exists=False)
    self.assertCommandContains(['./update_chroot'], expected=False)
    self.assertCommandContains(['./setup_board', '--skip_chroot_upgrade'])


class UprevStageTest(generic_stages_unittest.AbstractStageTestCase):
  """Tests for the UprevStage class."""

  def setUp(self):
    self.uprev_mock = self.PatchObject(commands, 'UprevPackages')

    self._Prepare()

  def ConstructStage(self):
    return build_stages.UprevStage(self._run)

  def testBuildRev(self):
    """Uprevving the build without uprevving chrome."""
    self._run.config['uprev'] = True
    self.RunStage()
    self.assertTrue(self.uprev_mock.called)

  def testNoRev(self):
    """No paths are enabled."""
    self._run.config['uprev'] = False
    self.RunStage()
    self.assertFalse(self.uprev_mock.called)


class AllConfigsTestCase(generic_stages_unittest.AbstractStageTestCase,
                         cros_test_lib.OutputTestCase):
  """Test case for testing against all bot configs."""

  def ConstructStage(self):
    """Bypass lint warning"""
    generic_stages_unittest.AbstractStageTestCase.ConstructStage(self)

  @contextlib.contextmanager
  def RunStageWithConfig(self, mock_configurator=None):
    """Run the given config"""
    try:
      with cros_test_lib.RunCommandMock() as rc:
        rc.SetDefaultCmdResult()
        if mock_configurator:
          mock_configurator(rc)
        with self.OutputCapturer():
          with cros_test_lib.LoggingCapturer():
            self.RunStage()

        yield rc

    except AssertionError as ex:
      msg = '%s failed the following test:\n%s' % (self._bot_id, ex)
      raise AssertionError(msg)

  def RunAllConfigs(self, task, skip_missing=False, site_config=None):
    """Run |task| against all major configurations"""
    if site_config is None:
      site_config = chromeos_config.GetConfig()

    boards = ('samus', 'arm-generic')

    with parallel.BackgroundTaskRunner(task) as queue:
      # Test every build config on an waterfall, that builds something.
      for bot_id, cfg in site_config.iteritems():
        if not cfg.boards or cfg.boards[0] not in boards:
          continue

        if skip_missing:
          try:
            for b in cfg.boards:
              portage_util.FindPrimaryOverlay(constants.BOTH_OVERLAYS, b)
          except portage_util.MissingOverlayException:
            continue

        queue.put([bot_id])


class BuildPackagesStageTest(AllConfigsTestCase,
                             cbuildbot_unittest.SimpleBuilderTestCase):
  """Tests BuildPackagesStage."""

  def setUp(self):
    self._release_tag = None
    self._update_metadata = False
    self._mock_configurator = None
    self.PatchObject(commands, 'ExtractDependencies', return_value=dict())

  def ConstructStage(self):
    self._run.attrs.release_tag = self._release_tag
    return build_stages.BuildPackagesStage(
        self._run, self._current_board,
        update_metadata=self._update_metadata)

  def RunTestsWithBotId(self, bot_id, options_tests=True):
    """Test with the config for the specified bot_id."""
    self._Prepare(bot_id)
    self._run.options.tests = options_tests

    with self.RunStageWithConfig(self._mock_configurator) as rc:
      cfg = self._run.config
      rc.assertCommandContains(['./build_packages'])
      rc.assertCommandContains(['./build_packages', '--skip_chroot_upgrade'])
      rc.assertCommandContains(['./build_packages', '--nousepkg'],
                               expected=not cfg['usepkg_build_packages'])
      rc.assertCommandContains(['./build_packages', '--nowithautotest'],
                               expected=not self._run.options.tests)

  def testAllConfigs(self):
    """Test all major configurations"""
    self.RunAllConfigs(self.RunTestsWithBotId)

  def testNoTests(self):
    """Test that self.options.tests = False works."""
    self.RunTestsWithBotId('amd64-generic-paladin', options_tests=False)

  def testIgnoreExtractDependenciesError(self):
    """Ignore errors when failing to extract dependencies."""
    self.PatchObject(commands, 'ExtractDependencies',
                     side_effect=Exception('unmet dependency'))
    self.RunTestsWithBotId('amd64-generic-paladin')

  def testFirmwareVersionsMixedImage(self):
    """Test that firmware versions are extracted correctly."""
    expected_main_firmware_version = 'reef_v1.1.5822-78709a5'
    expected_ec_firmware_version = 'Google_Reef.9042.30.0'

    def _HookRunCommandFirmwareUpdate(rc):
      # A mixed RO+RW image will have separate "(RW) version" fields.
      rc.AddCmdResult(partial_mock.ListRegex('chromeos-firmwareupdate'),
                      output='BIOS (RW) version: %s\nEC (RW) version: %s' %
                      (expected_main_firmware_version,
                       expected_ec_firmware_version))

    self._update_metadata = True
    update = os.path.join(
        self.build_root,
        'chroot/build/amd64-generic/usr/sbin/chromeos-firmwareupdate')
    osutils.Touch(update, makedirs=True)

    self._mock_configurator = _HookRunCommandFirmwareUpdate
    self.RunTestsWithBotId('amd64-generic-paladin', options_tests=False)
    board_metadata = (self._run.attrs.metadata.GetDict()['board-metadata']
                      .get('amd64-generic'))
    if board_metadata:
      self.assertIn('main-firmware-version', board_metadata)
      self.assertEqual(board_metadata['main-firmware-version'],
                       expected_main_firmware_version)
      self.assertIn('ec-firmware-version', board_metadata)
      self.assertEqual(board_metadata['ec-firmware-version'],
                       expected_ec_firmware_version)
      self.assertFalse(self._run.attrs.metadata.GetDict()['unibuild'])

  def testFirmwareVersions(self):
    """Test that firmware versions are extracted correctly."""
    expected_main_firmware_version = 'reef_v1.1.5822-78709a5'
    expected_ec_firmware_version = 'Google_Reef.9042.30.0'

    def _HookRunCommandFirmwareUpdate(rc):
      rc.AddCmdResult(partial_mock.ListRegex('chromeos-firmwareupdate'),
                      output='BIOS version: %s\nEC version: %s' %
                      (expected_main_firmware_version,
                       expected_ec_firmware_version))

    self._update_metadata = True
    update = os.path.join(
        self.build_root,
        'chroot/build/amd64-generic/usr/sbin/chromeos-firmwareupdate')
    osutils.Touch(update, makedirs=True)

    self._mock_configurator = _HookRunCommandFirmwareUpdate
    self.RunTestsWithBotId('amd64-generic-paladin', options_tests=False)
    board_metadata = (self._run.attrs.metadata.GetDict()['board-metadata']
                      .get('amd64-generic'))
    if board_metadata:
      self.assertIn('main-firmware-version', board_metadata)
      self.assertEqual(board_metadata['main-firmware-version'],
                       expected_main_firmware_version)
      self.assertIn('ec-firmware-version', board_metadata)
      self.assertEqual(board_metadata['ec-firmware-version'],
                       expected_ec_firmware_version)
      self.assertFalse(self._run.attrs.metadata.GetDict()['unibuild'])

  def testFirmwareVersionsUnibuild(self):
    """Test that firmware versions are extracted correctly for unibuilds."""

    def _HookRunCommand(rc):
      rc.AddCmdResult(partial_mock.In('list-models'),
                      output='reef\npyro\nelectro')
      rc.AddCmdResult(partial_mock.In('get'), output='key-123')
      rc.AddCmdResult(partial_mock.ListRegex('chromeos-firmwareupdate'),
                      output='''
Model:        reef
BIOS image:
BIOS version: Google_Reef.9042.87.1
BIOS (RW) version: Google_Reef.9042.110.0
EC version:   reef_v1.1.5900-ab1ee51
EC (RW) version: reef_v1.1.5909-bd1f0c9

Model:        pyro
BIOS image:
BIOS version: Google_Pyro.9042.87.1
BIOS (RW) version: Google_Pyro.9042.110.0
EC version:   pyro_v1.1.5900-ab1ee51
EC (RW) version: pyro_v1.1.5909-bd1f0c9

Model:        electro
BIOS image:
BIOS version: Google_Reef.9042.87.1
EC version:   reef_v1.1.5900-ab1ee51
EC (RW) version: reef_v1.1.5909-bd1f0c9
''')

    self._update_metadata = True
    update = os.path.join(
        self.build_root,
        'chroot/build/x86-generic/usr/sbin/chromeos-firmwareupdate')
    osutils.Touch(update, makedirs=True)

    cros_config_host = os.path.join(self.build_root,
                                    'chroot/usr/bin/cros_config_host')
    osutils.Touch(cros_config_host, makedirs=True)

    self._mock_configurator = _HookRunCommand
    self.RunTestsWithBotId('x86-generic-paladin', options_tests=False)
    board_metadata = (self._run.attrs.metadata.GetDict()['board-metadata']
                      .get('x86-generic'))
    self.assertIsNotNone(board_metadata)

    if 'models' in board_metadata:
      reef = board_metadata['models']['reef']
      self.assertEquals('Google_Reef.9042.87.1',
                        reef['main-readonly-firmware-version'])
      self.assertEquals('Google_Reef.9042.110.0',
                        reef['main-readwrite-firmware-version'])
      self.assertEquals('reef_v1.1.5909-bd1f0c9',
                        reef['ec-firmware-version'])
      self.assertEquals('key-123', reef['firmware-key-id'])

      self.assertIn('pyro', board_metadata['models'])
      self.assertIn('electro', board_metadata['models'])
      electro = board_metadata['models']['electro']
      self.assertEquals('Google_Reef.9042.87.1',
                        electro['main-readonly-firmware-version'])
      # Test RW firmware is defaulted to RO version if isn't specified.
      self.assertEquals('Google_Reef.9042.87.1',
                        electro['main-readwrite-firmware-version'])

  def testUnifiedBuilds(self):
    """Test that unified builds are marked as such."""
    def _HookRunCommandCrosConfigHost(rc):
      rc.AddCmdResult(partial_mock.ListRegex('cros_config_host'),
                      output='reef')

    self._update_metadata = True
    cros_config_host = os.path.join(self.build_root,
                                    'chroot/usr/bin/cros_config_host')
    osutils.Touch(cros_config_host, makedirs=True)
    self._mock_configurator = _HookRunCommandCrosConfigHost
    self.RunTestsWithBotId('amd64-generic-paladin', options_tests=False)
    self.assertTrue(self._run.attrs.metadata.GetDict()['unibuild'])

  def testGoma(self):
    self.PatchObject(build_stages.BuildPackagesStage,
                     '_ShouldEnableGoma', return_value=True)
    self._Prepare('amd64-generic-paladin')
    # Set dummy dir name to enable goma.
    with osutils.TempDir() as goma_dir, \
         tempfile.NamedTemporaryFile() as temp_goma_client_json:
      self._run.options.goma_dir = goma_dir
      self._run.options.goma_client_json = temp_goma_client_json.name

      stage = self.ConstructStage()
      chroot_args = stage._SetupGomaIfNecessary()
      self.assertEqual(
          ['--goma_dir', goma_dir,
           '--goma_client_json', temp_goma_client_json.name],
          chroot_args)
      portage_env = stage._portage_extra_env
      self.assertRegexpMatches(
          portage_env.get('GOMA_DIR', ''), '^/home/.*/goma$')
      self.assertIn(portage_env.get('USE', ''), 'goma')
      self.assertEqual(
          '/creds/service_accounts/service-account-goma-client.json',
          portage_env.get('GOMA_SERVICE_ACCOUNT_JSON_FILE', ''))

  def testGomaWithMissingCertFile(self):
    self.PatchObject(build_stages.BuildPackagesStage,
                     '_ShouldEnableGoma', return_value=True)
    self._Prepare('amd64-generic-paladin')
    # Set dummy dir name to enable goma.
    with osutils.TempDir() as goma_dir:
      self._run.options.goma_dir = goma_dir
      self._run.options.goma_client_json = 'dummy-goma-client-json-path'

      stage = self.ConstructStage()
      with self.assertRaisesRegexp(ValueError, 'json file is missing'):
        stage._SetupGomaIfNecessary()

  def testGomaOnBotWithoutCertFile(self):
    self.PatchObject(build_stages.BuildPackagesStage,
                     '_ShouldEnableGoma', return_value=True)
    self.PatchObject(cros_build_lib, 'HostIsCIBuilder', return_value=True)
    self._Prepare('amd64-generic-paladin')
    # Set dummy dir name to enable goma.
    with osutils.TempDir() as goma_dir:
      self._run.options.goma_dir = goma_dir
      stage = self.ConstructStage()

      with self.assertRaisesRegexp(
          ValueError, 'goma_client_json is not provided'):
        stage._SetupGomaIfNecessary()


class BuildImageStageMock(partial_mock.PartialMock):
  """Partial mock for BuildImageStage."""

  TARGET = 'chromite.cbuildbot.stages.build_stages.BuildImageStage'
  ATTRS = ('_BuildImages', '_GenerateAuZip')

  def _BuildImages(self, *args, **kwargs):
    with patches(
        patch(os, 'symlink'),
        patch(os, 'readlink', return_value='foo.txt')):
      self.backup['_BuildImages'](*args, **kwargs)

  def _GenerateAuZip(self, *args, **kwargs):
    with patch(path_util, 'ToChrootPath',
               return_value='/chroot/path'):
      self.backup['_GenerateAuZip'](*args, **kwargs)


class BuildImageStageTest(BuildPackagesStageTest):
  """Tests BuildImageStage."""

  def setUp(self):
    self.StartPatcher(BuildImageStageMock())

  def ConstructStage(self):
    return build_stages.BuildImageStage(self._run, self._current_board)

  def RunTestsWithReleaseConfig(self, release_tag):
    self._release_tag = release_tag

    with parallel_unittest.ParallelMock():
      with self.RunStageWithConfig() as rc:
        cfg = self._run.config
        cmd = ['./build_image', '--version=%s' % (self._release_tag or '')]
        rc.assertCommandContains(cmd, expected=cfg['images'])
        rc.assertCommandContains(['./image_to_vm.sh'],
                                 expected=cfg['vm_tests'])
        cmd = ['./build_library/generate_au_zip.py', '-o', '/chroot/path']
        rc.assertCommandContains(cmd, expected=cfg['images'])

  def RunTestsWithBotId(self, bot_id, options_tests=True):
    """Test with the config for the specified bot_id."""
    release_tag = '0.0.1'
    self._Prepare(bot_id)
    self._run.options.tests = options_tests
    self._run.attrs.release_tag = release_tag

    task = self.RunTestsWithReleaseConfig
    # TODO: This test is broken atm with tag=None.
    steps = [lambda tag=x: task(tag) for x in (release_tag,)]
    parallel.RunParallelSteps(steps)

  def testUnifiedBuilds(self):
    pass


class CleanUpStageTest(generic_stages_unittest.StageTestCase):
  """Test CleanUpStage."""

  BOT_ID = 'master-paladin'

  def setUp(self):
    self.PatchObject(buildbucket_lib, 'GetServiceAccount',
                     return_value=True)
    self.PatchObject(auth.AuthorizedHttp, '__init__',
                     return_value=None)
    self.PatchObject(buildbucket_lib.BuildbucketClient,
                     '_GetHost',
                     return_value=buildbucket_lib.BUILDBUCKET_TEST_HOST)

    self.fake_db = fake_cidb.FakeCIDBConnection()
    cidb.CIDBConnectionFactory.SetupMockCidb(self.fake_db)

    self.fake_db.InsertBuild(
        'test_builder', waterfall.WATERFALL_TRYBOT, 666, 'test_config',
        'test_hostname',
        status=constants.BUILDER_STATUS_INFLIGHT,
        timeout_seconds=23456,
        buildbucket_id='100')

    self.fake_db.InsertBuild(
        'test_builder', waterfall.WATERFALL_TRYBOT, 666, 'test_config',
        'test_hostname',
        status=constants.BUILDER_STATUS_INFLIGHT,
        timeout_seconds=23456,
        buildbucket_id='200')

    self._Prepare(extra_config={'chroot_use_image': False})

  def ConstructStage(self):
    return build_stages.CleanUpStage(self._run)

  def testChrootReuseImageMismatch(self):
    chroot_path = os.path.join(self.build_root, 'chroot')
    osutils.Touch(chroot_path + '.img')
    stage = self.ConstructStage()
    self.assertFalse(stage.CanReuseChroot(chroot_path))

  def testChrootReuseChrootReplace(self):
    self._Prepare(
        extra_config={'chroot_use_image': False, 'chroot_replace': True})

    self.PatchObject(
        build_stages.CleanUpStage,
        '_GetPreviousBuildStatus',
        return_value=build_summary.BuildSummary(
            build_number=314,
            status=constants.BUILDER_STATUS_PASSED))

    chroot_path = os.path.join(self.build_root, 'chroot')
    stage = self.ConstructStage()
    self.assertFalse(stage.CanReuseChroot(chroot_path))

  def testChrootReusePreviousFailed(self):
    self.PatchObject(
        build_stages.CleanUpStage,
        '_GetPreviousBuildStatus',
        return_value=build_summary.BuildSummary(
            build_number=314,
            status=constants.BUILDER_STATUS_FAILED))

    chroot_path = os.path.join(self.build_root, 'chroot')
    stage = self.ConstructStage()
    self.assertFalse(stage.CanReuseChroot(chroot_path))

  def testChrootReusePreviousMasterMissing(self):
    self.PatchObject(
        build_stages.CleanUpStage,
        '_GetPreviousBuildStatus',
        return_value=build_summary.BuildSummary(
            build_number=314,
            master_build_id=2178,
            status=constants.BUILDER_STATUS_PASSED))

    chroot_path = os.path.join(self.build_root, 'chroot')
    stage = self.ConstructStage()
    self.assertFalse(stage.CanReuseChroot(chroot_path))

  def testChrootReusePreviousMasterFailed(self):
    master_id = self.fake_db.InsertBuild(
        'test_builder', waterfall.WATERFALL_TRYBOT, 123, 'test_config',
        'test_hostname', status=constants.BUILDER_STATUS_FAILED,
        buildbucket_id='2178')
    self.PatchObject(
        build_stages.CleanUpStage,
        '_GetPreviousBuildStatus',
        return_value=build_summary.BuildSummary(
            build_number=314,
            master_build_id=master_id,
            status=constants.BUILDER_STATUS_PASSED))

    chroot_path = os.path.join(self.build_root, 'chroot')
    stage = self.ConstructStage()
    self.assertFalse(stage.CanReuseChroot(chroot_path))

  def testChrootReuseAllPassed(self):
    master_id = self.fake_db.InsertBuild(
        'test_builder', waterfall.WATERFALL_TRYBOT, 123, 'test_config',
        'test_hostname', status=constants.BUILDER_STATUS_PASSED,
        buildbucket_id='2178')
    self.PatchObject(
        build_stages.CleanUpStage,
        '_GetPreviousBuildStatus',
        return_value=build_summary.BuildSummary(
            build_number=314,
            master_build_id=master_id,
            status=constants.BUILDER_STATUS_PASSED))

    chroot_path = os.path.join(self.build_root, 'chroot')
    stage = self.ConstructStage()
    self.assertTrue(stage.CanReuseChroot(chroot_path))

  def testChrootSnapshotClobber(self):
    self._Prepare(
        extra_cmd_args=['--clobber'],
        extra_config={'chroot_use_image': True, 'chroot_replace': False})
    chroot_path = os.path.join(self.build_root, 'chroot')
    osutils.Touch(chroot_path + '.img')
    stage = self.ConstructStage()
    self.assertFalse(stage.CanUseChrootSnapshotToDelete(chroot_path))

  def testChrootSnapshotReplace(self):
    self._Prepare(
        extra_config={'chroot_use_image': True, 'chroot_replace': True})
    chroot_path = os.path.join(self.build_root, 'chroot')
    osutils.Touch(chroot_path + '.img')
    stage = self.ConstructStage()
    self.assertFalse(stage.CanUseChrootSnapshotToDelete(chroot_path))

  def testChrootSnapshotNoUseImage(self):
    self._Prepare(
        extra_config={'chroot_use_image': False, 'chroot_replace': False})
    chroot_path = os.path.join(self.build_root, 'chroot')
    osutils.Touch(chroot_path + '.img')
    stage = self.ConstructStage()
    self.assertFalse(stage.CanUseChrootSnapshotToDelete(chroot_path))

  def testChrootSnapshotMissingImage(self):
    self._Prepare(
        extra_config={'chroot_use_image': True, 'chroot_replace': False})
    chroot_path = os.path.join(self.build_root, 'chroot')
    stage = self.ConstructStage()
    self.assertFalse(stage.CanUseChrootSnapshotToDelete(chroot_path))

  def testChrootSnapshotAllPass(self):
    self._Prepare(
        extra_config={'chroot_use_image': True, 'chroot_replace': False})
    chroot_path = os.path.join(self.build_root, 'chroot')
    osutils.Touch(chroot_path + '.img')
    stage = self.ConstructStage()
    self.assertTrue(stage.CanUseChrootSnapshotToDelete(chroot_path))

  def testChrootRevertNoSnapshots(self):
    self.PatchObject(commands, 'ListChrootSnapshots', return_value=[])
    self._Prepare(
        extra_config={'chroot_use_image': True, 'chroot_replace': False})
    chroot_path = os.path.join(self.build_root, 'chroot')
    osutils.Touch(chroot_path + '.img')
    stage = self.ConstructStage()
    self.assertFalse(stage._RevertChrootToCleanSnapshot())

  def testChrootRevertSnapshotNotFound(self):
    self.PatchObject(commands, 'ListChrootSnapshots', return_value=['snap'])
    self._Prepare(
        extra_config={'chroot_use_image': True, 'chroot_replace': False})
    chroot_path = os.path.join(self.build_root, 'chroot')
    osutils.Touch(chroot_path + '.img')
    stage = self.ConstructStage()
    self.assertFalse(stage._RevertChrootToCleanSnapshot())

  def testChrootCleanSnapshotReplacesAllExisting(self):
    self.PatchObject(commands, 'ListChrootSnapshots',
                     return_value=['snap1', 'snap2',
                                   constants.CHROOT_SNAPSHOT_CLEAN])
    delete_mock = self.PatchObject(commands, 'DeleteChrootSnapshot',
                                   return_value=True)
    create_mock = self.PatchObject(commands, 'CreateChrootSnapshot')

    self._Prepare(
        extra_config={'chroot_use_image': True, 'chroot_replace': False})
    chroot_path = os.path.join(self.build_root, 'chroot')
    osutils.Touch(chroot_path + '.img')
    stage = self.ConstructStage()
    stage._CreateCleanSnapshot()

    self.assertEqual(delete_mock.mock_calls, [
        mock.call(self.build_root, 'snap1'),
        mock.call(self.build_root, 'snap2'),
        mock.call(self.build_root, constants.CHROOT_SNAPSHOT_CLEAN)])
    create_mock.assert_called_with(self.build_root,
                                   constants.CHROOT_SNAPSHOT_CLEAN)

  def testChrootRevertFailsWhenCommandsRaiseExceptions(self):
    self.PatchObject(
        cros_build_lib,
        'SudoRunCommand',
        side_effect=cros_build_lib.RunCommandError(
            'error', cros_build_lib.CommandResult(cmd='error', returncode=5)))
    self._Prepare(
        extra_config={'chroot_use_image': True, 'chroot_replace': False})
    chroot_path = os.path.join(self.build_root, 'chroot')
    osutils.Touch(chroot_path + '.img')
    stage = self.ConstructStage()
    self.assertFalse(stage._RevertChrootToCleanSnapshot())


class CleanUpStageCancelSlaveBuilds(generic_stages_unittest.StageTestCase):
  """Test CleanUpStage.CancelObsoleteSlaveBuilds."""
  BOT_ID = 'master-paladin'

  def setUp(self):
    self.PatchObject(buildbucket_lib, 'GetServiceAccount',
                     return_value=True)
    self.PatchObject(auth.AuthorizedHttp, '__init__',
                     return_value=None)
    self.PatchObject(buildbucket_lib.BuildbucketClient,
                     '_GetHost',
                     return_value=buildbucket_lib.BUILDBUCKET_TEST_HOST)

    # Mock out the active APIs for both testing and safety.
    self.cancelMock = self.PatchObject(builder_status_lib,
                                       'CancelBuilds')

    self.searchMock = self.PatchObject(buildbucket_lib.BuildbucketClient,
                                       'SearchAllBuilds')

    self._Prepare(extra_config={'chroot_use_image': False})

  def ConstructStage(self):
    return build_stages.CleanUpStage(self._run)

  def testNoPreviousMasterBuilds(self):
    """Test cancellation if the master has never run."""
    search_results = [[]]
    self.searchMock.side_effect = search_results
    stage = self.ConstructStage()
    stage.CancelObsoleteSlaveBuilds()

    # Validate searches and cancellations match expections.
    self.assertEqual(self.searchMock.call_count, len(search_results))
    self.cancelMock.assert_not_called()

  def testNoPreviousSlaveBuilds(self):
    """Test cancellation if there are no running slave builds."""
    search_results = [
        [{'id': 'master_1'}],
        [],
        [],
    ]
    self.searchMock.side_effect = search_results

    stage = self.ConstructStage()
    stage.CancelObsoleteSlaveBuilds()

    # Validate searches and cancellations match expections.
    self.assertEqual(self.searchMock.call_count, len(search_results))
    self.cancelMock.assert_not_called()

  def testPreviousSlaveBuild(self):
    """Test cancellation if there is a running slave build."""
    search_results = [
        [{'id': 'master_1'}],
        [{'id': 'm1_slave_1'}],
        [],
    ]
    self.searchMock.side_effect = search_results

    stage = self.ConstructStage()
    stage.CancelObsoleteSlaveBuilds()

    # Validate searches and cancellations match expections.
    self.assertEqual(self.searchMock.call_count, len(search_results))
    self.assertEqual(self.cancelMock.call_count, 1)

    cancelled_ids = self.cancelMock.call_args[0][0]
    self.assertEqual(cancelled_ids, ['m1_slave_1'])

  def testManyPreviousSlaveBuilds(self):
    """Test cancellation with an assortment of running slave builds."""
    search_results = [
        [{'id': 'master_1'}, {'id': 'master_2'}],
        [{'id': 'm1_slave_1'}, {'id': 'm1_slave_2'}],
        [{'id': 'm1_slave_3'}, {'id': 'm1_slave_4'}],
        [{'id': 'm2_slave_1'}, {'id': 'm2_slave_2'}],
        [{'id': 'm2_slave_3'}, {'id': 'm2_slave_4'}],
    ]
    self.searchMock.side_effect = search_results

    stage = self.ConstructStage()
    stage.CancelObsoleteSlaveBuilds()

    # Validate searches and cancellations match expections.
    self.assertEqual(self.searchMock.call_count, len(search_results))
    self.assertEqual(self.cancelMock.call_count, 1)

    cancelled_ids = self.cancelMock.call_args[0][0]
    self.assertEqual(cancelled_ids, [
        'm1_slave_1', 'm1_slave_2', 'm1_slave_3', 'm1_slave_4',
        'm2_slave_1', 'm2_slave_2', 'm2_slave_3', 'm2_slave_4',
    ])
