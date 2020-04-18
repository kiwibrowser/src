# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module tests the cros image command."""

from __future__ import print_function

import copy
import mock
import os
import shutil

from chromite.lib import constants
from chromite.cli import command_unittest
from chromite.cli.cros import cros_chrome_sdk
from chromite.lib import cache
from chromite.lib import cros_test_lib
from chromite.lib import gs
from chromite.lib import gs_unittest
from chromite.lib import osutils
from chromite.lib import partial_mock
from gn_helpers import gn_helpers


# pylint: disable=W0212


class MockChromeSDKCommand(command_unittest.MockCommand):
  """Mock out the build command."""
  TARGET = 'chromite.cli.cros.cros_chrome_sdk.ChromeSDKCommand'
  TARGET_CLASS = cros_chrome_sdk.ChromeSDKCommand
  COMMAND = 'chrome-sdk'
  ATTRS = (('_GOMA_URL', '_SetupEnvironment') +
           command_unittest.MockCommand.ATTRS)

  _GOMA_URL = 'Invalid URL'

  def __init__(self, *args, **kwargs):
    command_unittest.MockCommand.__init__(self, *args, **kwargs)
    self.env = None

  def _SetupEnvironment(self, *args, **kwargs):
    env = self.backup['_SetupEnvironment'](*args, **kwargs)
    self.env = copy.deepcopy(env)
    return env


class ParserTest(cros_test_lib.MockTempDirTestCase):
  """Test the parser."""
  def testNormal(self):
    """Tests that our example parser works normally."""
    with MockChromeSDKCommand(
        ['--board', SDKFetcherMock.BOARD],
        base_args=['--cache-dir', self.tempdir]) as bootstrap:
      self.assertEquals(bootstrap.inst.options.board, SDKFetcherMock.BOARD)
      self.assertEquals(bootstrap.inst.options.cache_dir, self.tempdir)


def _GSCopyMock(_self, path, dest, **_kwargs):
  """Used to simulate a GS Copy operation."""
  with osutils.TempDir() as tempdir:
    local_path = os.path.join(tempdir, os.path.basename(path))
    osutils.Touch(local_path)
    shutil.move(local_path, dest)


def _DependencyMockCtx(f):
  """Attribute that ensures dependency PartialMocks are started.

  Since PartialMock does not support nested mocking, we need to first call
  stop() on the outer level PartialMock (which is passed in to us).  We then
  re-start() the outer level upon exiting the context.
  """
  def new_f(self, *args, **kwargs):
    if not self.entered:
      try:
        self.entered = True
        # Temporarily disable outer GSContext mock before starting our mock.
        # TODO(rcui): Generalize this attribute and include in partial_mock.py.
        for emock in self.external_mocks:
          emock.stop()

        with self.gs_mock:
          return f(self, *args, **kwargs)
      finally:
        self.entered = False
        for emock in self.external_mocks:
          emock.start()
    else:
      return f(self, *args, **kwargs)
  return new_f


class SDKFetcherMock(partial_mock.PartialMock):
  """Provides mocking functionality for SDKFetcher."""

  TARGET = 'chromite.cli.cros.cros_chrome_sdk.SDKFetcher'
  ATTRS = ('__init__', 'GetFullVersion', '_GetMetadata', '_UpdateTarball',
           'UpdateDefaultVersion')

  FAKE_METADATA = """
{
  "boards": ["x86-alex"],
  "cros-version": "25.3543.2",
  "metadata-version": "1",
  "bot-hostname": "build82-m2.golo.chromium.org",
  "bot-config": "x86-alex-release",
  "toolchain-tuple": ["i686-pc-linux-gnu"],
  "toolchain-url": "2013/01/%(target)s-2013.01.23.003823.tar.xz",
  "sdk-version": "2013.01.23.003823"
}"""

  BOARD = 'x86-alex'
  VERSION = 'XXXX.X.X'

  def __init__(self, external_mocks=None):
    """Initializes the mock.

    Args:
      external_mocks: A list of already started PartialMock/patcher instances.
        stop() will be called on each element every time execution enters one of
        our the mocked out methods, and start() called on it once execution
        leaves the mocked out method.
    """
    partial_mock.PartialMock.__init__(self)
    self.external_mocks = external_mocks or []
    self.entered = False
    self.gs_mock = gs_unittest.GSContextMock()
    self.gs_mock.SetDefaultCmdResult()
    self.env = None

  @_DependencyMockCtx
  def _target__init__(self, inst, *args, **kwargs):
    self.backup['__init__'](inst, *args, **kwargs)
    if not inst.cache_base.startswith('/tmp'):
      raise AssertionError('For testing, SDKFetcher cache_dir needs to be a '
                           'dir under /tmp')

  @_DependencyMockCtx
  def UpdateDefaultVersion(self, inst, *_args, **_kwargs):
    inst._SetDefaultVersion(self.VERSION)
    return self.VERSION, True

  @_DependencyMockCtx
  def _UpdateTarball(self, inst, *args, **kwargs):
    with mock.patch.object(gs.GSContext, 'Copy', autospec=True,
                           side_effect=_GSCopyMock):
      with mock.patch.object(cache, 'Untar'):
        return self.backup['_UpdateTarball'](inst, *args, **kwargs)

  @_DependencyMockCtx
  def GetFullVersion(self, _inst, version):
    return 'R26-%s' % version

  @_DependencyMockCtx
  def _GetMetadata(self, inst, *args, **kwargs):
    self.gs_mock.SetDefaultCmdResult()
    self.gs_mock.AddCmdResult(
        partial_mock.ListRegex('cat .*/%s' % constants.METADATA_JSON),
        output=self.FAKE_METADATA)
    return self.backup['_GetMetadata'](inst, *args, **kwargs)


class RunThroughTest(cros_test_lib.MockTempDirTestCase,
                     cros_test_lib.LoggingTestCase):
  """Run the script with most things mocked out."""

  VERSION_KEY = (SDKFetcherMock.BOARD, SDKFetcherMock.VERSION,
                 constants.CHROME_SYSROOT_TAR)

  FAKE_ENV = {
      'GN_ARGS': 'target_sysroot="/path/to/sysroot" is_clang=false',
      'CXX': 'x86_64-cros-linux-gnu-clang++ -B /path/to/gold',
      'CC': 'x86_64-cros-linux-gnu-clang -B /path/to/gold',
      'LD': 'x86_64-cros-linux-gnu-clang++ -B /path/to/gold',
      'CFLAGS': '-O2',
      'CXXFLAGS': '-O2',
  }

  def SetupCommandMock(self, extra_args=None):
    cmd_args = ['--board', SDKFetcherMock.BOARD, '--chrome-src',
                self.chrome_src_dir, 'true']
    if extra_args:
      cmd_args.extend(extra_args)

    self.cmd_mock = MockChromeSDKCommand(
        cmd_args, base_args=['--cache-dir', self.tempdir])
    self.StartPatcher(self.cmd_mock)
    self.cmd_mock.UnMockAttr('Run')

  def SourceEnvironmentMock(self, path, *_args, **_kwargs):
    if path.endswith('environment'):
      return copy.deepcopy(self.FAKE_ENV)
    return {}

  def setUp(self):
    self.rc_mock = cros_test_lib.RunCommandMock()
    self.rc_mock.SetDefaultCmdResult()
    self.StartPatcher(self.rc_mock)

    self.sdk_mock = self.StartPatcher(SDKFetcherMock(
        external_mocks=[self.rc_mock]))

    # This needs to occur before initializing MockChromeSDKCommand.
    self.bashrc = os.path.join(self.tempdir, 'bashrc')
    self.PatchObject(constants, 'CHROME_SDK_BASHRC', new=self.bashrc)

    self.PatchObject(osutils, 'SourceEnvironment',
                     autospec=True, side_effect=self.SourceEnvironmentMock)
    self.rc_mock.AddCmdResult(cros_chrome_sdk.ChromeSDKCommand.GOMACC_PORT_CMD,
                              output='8088')

    # Initialized by SetupCommandMock.
    self.cmd_mock = None

    # Set up a fake Chrome src/ directory
    self.chrome_root = os.path.join(self.tempdir, 'chrome_root')
    self.chrome_src_dir = os.path.join(self.chrome_root, 'src')
    osutils.SafeMakedirs(self.chrome_src_dir)
    osutils.Touch(os.path.join(self.chrome_root, '.gclient'))

  @property
  def cache(self):
    return self.cmd_mock.inst.sdk.tarball_cache

  def testIt(self):
    """Test a runthrough of the script."""
    self.SetupCommandMock()
    with cros_test_lib.LoggingCapturer() as logs:
      self.cmd_mock.inst.Run()
      self.AssertLogsContain(logs, 'Goma:', inverted=True)

  def testErrorCodePassthrough(self):
    """Test that error codes are passed through."""
    self.SetupCommandMock()
    with cros_test_lib.LoggingCapturer():
      self.rc_mock.AddCmdResult(partial_mock.ListRegex('-- true'),
                                returncode=5)
      returncode = self.cmd_mock.inst.Run()
      self.assertEquals(returncode, 5)

  def testLocalSDKPath(self):
    """Fetch components from a local --sdk-path."""
    sdk_dir = os.path.join(self.tempdir, 'sdk_dir')
    osutils.SafeMakedirs(sdk_dir)
    osutils.WriteFile(os.path.join(sdk_dir, constants.METADATA_JSON),
                      SDKFetcherMock.FAKE_METADATA)
    self.SetupCommandMock(extra_args=['--sdk-path', sdk_dir])
    with cros_test_lib.LoggingCapturer():
      self.cmd_mock.inst.Run()

  def testGomaError(self):
    """We print an error message when GomaError is raised."""
    self.SetupCommandMock()
    with cros_test_lib.LoggingCapturer() as logs:
      self.PatchObject(cros_chrome_sdk.ChromeSDKCommand, '_FetchGoma',
                       side_effect=cros_chrome_sdk.GomaError())
      self.cmd_mock.inst.Run()
      self.AssertLogsContain(logs, 'Goma:')

  def testSpecificComponent(self):
    """Tests that SDKFetcher.Prepare() handles |components| param properly."""
    sdk = cros_chrome_sdk.SDKFetcher(os.path.join(self.tempdir),
                                     SDKFetcherMock.BOARD)
    components = [constants.BASE_IMAGE_TAR, constants.CHROME_SYSROOT_TAR]
    with sdk.Prepare(components=components) as ctx:
      for c in components:
        self.assertExists(ctx.key_map[c].path)
      for c in [constants.IMAGE_SCRIPTS_TAR, constants.CHROME_ENV_TAR]:
        self.assertFalse(c in ctx.key_map)

  @staticmethod
  def FindInPath(paths, endswith):
    for path in paths.split(':'):
      if path.endswith(endswith):
        return True
    return False

  def testGomaInPath(self):
    """Verify that we do indeed add Goma to the PATH."""
    self.SetupCommandMock()
    self.cmd_mock.inst.Run()

    self.assertIn('use_goma = true', self.cmd_mock.env['GN_ARGS'])

  def testNoGoma(self):
    """Verify that we do not add Goma to the PATH."""
    self.SetupCommandMock(extra_args=['--nogoma'])
    self.cmd_mock.inst.Run()

    self.assertIn('use_goma = false', self.cmd_mock.env['GN_ARGS'])

  def testClang(self):
    """Verifies clang codepath."""
    with cros_test_lib.LoggingCapturer():
      self.SetupCommandMock(extra_args=['--clang'])
      self.cmd_mock.inst.Run()

  def testGnArgsStalenessCheckNoMatch(self):
    """Verifies the GN args are checked for staleness with a mismatch."""
    with cros_test_lib.LoggingCapturer() as logs:
      out_dir = 'out_%s' % SDKFetcherMock.BOARD
      build_label = 'Release'
      gn_args_file_dir = os.path.join(self.chrome_src_dir, out_dir, build_label)
      gn_args_file_path = os.path.join(gn_args_file_dir, 'args.gn')
      osutils.SafeMakedirs(gn_args_file_dir)
      osutils.WriteFile(gn_args_file_path, 'foo = "no match"')

      self.SetupCommandMock()
      self.cmd_mock.inst.Run()

      self.AssertLogsContain(logs, 'Stale args.gn file')

  def testGnArgsStalenessCheckMatch(self):
    """Verifies the GN args are checked for staleness with a match."""
    with cros_test_lib.LoggingCapturer() as logs:
      self.SetupCommandMock()
      self.cmd_mock.inst.Run()

      out_dir = 'out_%s' % SDKFetcherMock.BOARD
      build_label = 'Release'
      gn_args_file_dir = os.path.join(self.chrome_src_dir, out_dir, build_label)
      gn_args_file_path = os.path.join(gn_args_file_dir, 'args.gn')

      osutils.SafeMakedirs(gn_args_file_dir)
      osutils.WriteFile(gn_args_file_path, self.cmd_mock.env['GN_ARGS'])

      self.cmd_mock.inst.Run()

      self.AssertLogsContain(logs, 'Stale args.gn file', inverted=True)

  def testGnArgsStalenessIgnoreStripped(self):
    """Verifies the GN args ignore stripped args."""
    with cros_test_lib.LoggingCapturer() as logs:
      self.SetupCommandMock()
      self.cmd_mock.inst.Run()

      out_dir = 'out_%s' % SDKFetcherMock.BOARD
      build_label = 'Release'
      gn_args_file_dir = os.path.join(self.chrome_src_dir, out_dir, build_label)
      gn_args_file_path = os.path.join(gn_args_file_dir, 'args.gn')

      osutils.SafeMakedirs(gn_args_file_dir)
      gn_args_dict = gn_helpers.FromGNArgs(self.cmd_mock.env['GN_ARGS'])
      # 'dcheck_always_on' should be ignored.
      gn_args_dict['dcheck_always_on'] = True
      osutils.WriteFile(gn_args_file_path, gn_helpers.ToGNString(gn_args_dict))

      self.cmd_mock.inst.Run()

      self.AssertLogsContain(logs, 'Stale args.gn file', inverted=True)

  def testChromiumOutDirSet(self):
    """Verify that CHROMIUM_OUT_DIR is set."""
    self.SetupCommandMock()
    self.cmd_mock.inst.Run()

    out_dir = os.path.join(self.chrome_src_dir, 'out_%s' % SDKFetcherMock.BOARD)

    self.assertEquals(out_dir, self.cmd_mock.env['CHROMIUM_OUT_DIR'])

class GomaTest(cros_test_lib.MockTempDirTestCase,
               cros_test_lib.LoggingTestCase):
  """Test Goma setup functionality."""

  def setUp(self):
    self.rc_mock = cros_test_lib.RunCommandMock()
    self.rc_mock.SetDefaultCmdResult()
    self.StartPatcher(self.rc_mock)

    self.cmd_mock = MockChromeSDKCommand(
        ['--board', SDKFetcherMock.BOARD, 'true'],
        base_args=['--cache-dir', self.tempdir])
    self.StartPatcher(self.cmd_mock)

  def VerifyGomaError(self):
    self.assertRaises(cros_chrome_sdk.GomaError, self.cmd_mock.inst._FetchGoma)

  def testNoGomaPort(self):
    """We print an error when gomacc is not returning a port."""
    self.rc_mock.AddCmdResult(
        cros_chrome_sdk.ChromeSDKCommand.GOMACC_PORT_CMD)
    self.VerifyGomaError()

  def testGomaccError(self):
    """We print an error when gomacc exits with nonzero returncode."""
    self.rc_mock.AddCmdResult(
        cros_chrome_sdk.ChromeSDKCommand.GOMACC_PORT_CMD, returncode=1)
    self.VerifyGomaError()

  def testFetchError(self):
    """We print an error when we can't fetch Goma."""
    self.rc_mock.AddCmdResult(
        cros_chrome_sdk.ChromeSDKCommand.GOMACC_PORT_CMD, returncode=1)
    self.VerifyGomaError()

  def testGomaStart(self):
    """Test that we start Goma if it's not already started."""
    # Duplicate return values.
    self.PatchObject(cros_chrome_sdk.ChromeSDKCommand, '_GomaPort',
                     side_effect=['XXXX', 'XXXX'])
    # Run it twice to exercise caching.
    for _ in range(2):
      goma_dir, goma_port = self.cmd_mock.inst._FetchGoma()
      self.assertEquals(goma_port, 'XXXX')
      self.assertTrue(bool(goma_dir))


class VersionTest(cros_test_lib.MockTempDirTestCase):
  """Tests the determination of which SDK version to use."""

  VERSION = '3543.0.0'
  FULL_VERSION = 'R55-%s' % VERSION
  RECENT_VERSION_MISSING = '3542.0.0'
  RECENT_VERSION_FOUND = '3541.0.0'
  FULL_VERSION_RECENT = 'R55-%s' % RECENT_VERSION_FOUND
  NON_CANARY_VERSION = '3543.2.1'
  FULL_VERSION_NON_CANARY = 'R55-%s' % NON_CANARY_VERSION
  BOARD = 'eve'

  VERSION_BASE = ('gs://chromeos-image-archive/%s-release/LATEST-%s'
                  % (BOARD, VERSION))

  CAT_ERROR = 'CommandException: No URLs matched %s' % VERSION_BASE
  LS_ERROR = 'CommandException: One or more URLs matched no objects.'

  def setUp(self):
    self.gs_mock = self.StartPatcher(gs_unittest.GSContextMock())
    self.gs_mock.SetDefaultCmdResult()
    self.sdk_mock = self.StartPatcher(SDKFetcherMock(
        external_mocks=[self.gs_mock]))

    os.environ.pop(cros_chrome_sdk.SDKFetcher.SDK_VERSION_ENV, None)
    self.sdk = cros_chrome_sdk.SDKFetcher(
        os.path.join(self.tempdir, 'cache'), self.BOARD)

  def testUpdateDefaultChromeVersion(self):
    """We pick up the right LKGM version from the Chrome tree."""
    dir_struct = [
        'gclient_root/.gclient'
    ]
    cros_test_lib.CreateOnDiskHierarchy(self.tempdir, dir_struct)
    gclient_root = os.path.join(self.tempdir, 'gclient_root')
    self.PatchObject(os, 'getcwd', return_value=gclient_root)

    lkgm_file = os.path.join(gclient_root, 'src', constants.PATH_TO_CHROME_LKGM)
    osutils.Touch(lkgm_file, makedirs=True)
    osutils.WriteFile(lkgm_file, self.VERSION)
    self.sdk_mock.UnMockAttr('UpdateDefaultVersion')
    self.sdk.UpdateDefaultVersion()
    self.assertEquals(self.sdk.GetDefaultVersion(),
                      self.VERSION)

  def testFullVersion(self):
    """Test full version calculation."""
    self.sdk_mock.UnMockAttr('GetFullVersion')
    self.gs_mock.AddCmdResult(
        partial_mock.ListRegex('cat .*/LATEST-%s' % self.VERSION),
        output=self.FULL_VERSION)
    self.assertEquals(
        self.FULL_VERSION,
        self.sdk.GetFullVersion(self.VERSION))

  def testFullVersionFromRecentLatest(self):
    """Test full version calculation when there is no matching LATEST- file."""
    def _RaiseGSNoSuchKey(*_args, **_kwargs):
      raise gs.GSNoSuchKey('file does not exist')
    self.sdk_mock.UnMockAttr('GetFullVersion')
    self.gs_mock.AddCmdResult(
        partial_mock.ListRegex('cat .*/LATEST-%s' % self.VERSION),
        side_effect=_RaiseGSNoSuchKey)
    self.gs_mock.AddCmdResult(
        partial_mock.ListRegex(
            'cat .*/LATEST-%s' % self.RECENT_VERSION_MISSING),
        side_effect=_RaiseGSNoSuchKey)
    self.gs_mock.AddCmdResult(
        partial_mock.ListRegex('cat .*/LATEST-%s' % self.RECENT_VERSION_FOUND),
        output=self.FULL_VERSION_RECENT)
    self.assertEquals(
        self.FULL_VERSION_RECENT,
        self.sdk.GetFullVersion(self.VERSION))

  def testFullVersionCaching(self):
    """Test full version calculation and caching."""
    def RaiseException(*_args, **_kwargs):
      raise Exception('boom')

    self.sdk_mock.UnMockAttr('GetFullVersion')
    self.gs_mock.AddCmdResult(
        partial_mock.ListRegex('cat .*/LATEST-%s' % self.VERSION),
        output=self.FULL_VERSION)
    self.assertEquals(
        self.FULL_VERSION,
        self.sdk.GetFullVersion(self.VERSION))
    # Test that we access the cache on the next call, rather than checking GS.
    self.gs_mock.AddCmdResult(
        partial_mock.ListRegex('cat .*/LATEST-%s' % self.VERSION),
        side_effect=RaiseException)
    self.assertEquals(
        self.FULL_VERSION,
        self.sdk.GetFullVersion(self.VERSION))
    # Test that we access GS again if the board is changed.
    self.sdk.board += '2'
    self.gs_mock.AddCmdResult(
        partial_mock.ListRegex('cat .*/LATEST-%s' % self.VERSION),
        output=self.FULL_VERSION + '2')
    self.assertEquals(
        self.FULL_VERSION + '2',
        self.sdk.GetFullVersion(self.VERSION))

  def testNoLatestVersion(self):
    """We raise an exception when there is no recent latest version."""
    self.sdk_mock.UnMockAttr('GetFullVersion')
    self.gs_mock.AddCmdResult(
        partial_mock.ListRegex('cat .*/LATEST-*'),
        output='', error=self.CAT_ERROR, returncode=1)
    self.gs_mock.AddCmdResult(
        partial_mock.ListRegex('ls .*%s' % self.VERSION),
        output='', error=self.LS_ERROR, returncode=1)
    self.assertRaises(cros_chrome_sdk.MissingSDK, self.sdk.GetFullVersion,
                      self.VERSION)

  def testNonCanaryFullVersion(self):
    """Test full version calculation for a non canary version."""
    self.sdk_mock.UnMockAttr('GetFullVersion')
    self.gs_mock.AddCmdResult(
        partial_mock.ListRegex('cat .*/LATEST-%s' % self.NON_CANARY_VERSION),
        output=self.FULL_VERSION_NON_CANARY)
    self.assertEquals(
        self.FULL_VERSION_NON_CANARY,
        self.sdk.GetFullVersion(self.NON_CANARY_VERSION))

  def testNonCanaryNoLatestVersion(self):
    """We raise an exception when there is no matching latest non canary."""
    self.sdk_mock.UnMockAttr('GetFullVersion')
    self.gs_mock.AddCmdResult(
        partial_mock.ListRegex('cat .*/LATEST-%s' % self.NON_CANARY_VERSION),
        output='', error=self.CAT_ERROR, returncode=1)
    # Set any other query to return a valid version, but we don't expect that
    # to occur for non canary versions.
    self.gs_mock.SetDefaultCmdResult(output=self.FULL_VERSION_NON_CANARY)
    self.assertRaises(cros_chrome_sdk.MissingSDK, self.sdk.GetFullVersion,
                      self.NON_CANARY_VERSION)

  def testDefaultEnvBadBoard(self):
    """We don't use the version in the environment if board doesn't match."""
    os.environ[cros_chrome_sdk.SDKFetcher.SDK_VERSION_ENV] = self.VERSION
    self.assertNotEquals(self.VERSION, self.sdk_mock.VERSION)
    self.assertEquals(self.sdk.GetDefaultVersion(), None)

  def testDefaultEnvGoodBoard(self):
    """We use the version in the environment if board matches."""
    sdk_version_env = cros_chrome_sdk.SDKFetcher.SDK_VERSION_ENV
    os.environ[sdk_version_env] = self.VERSION
    os.environ[cros_chrome_sdk.SDKFetcher.SDK_BOARD_ENV] = self.BOARD
    self.assertEquals(self.sdk.GetDefaultVersion(), self.VERSION)


class PathVerifyTest(cros_test_lib.MockTempDirTestCase,
                     cros_test_lib.LoggingTestCase):
  """Tests user_rc PATH validation and warnings."""

  def testPathVerifyWarnings(self):
    """Test the user rc PATH verification codepath."""
    def SourceEnvironmentMock(*_args, **_kwargs):
      return {
          'PATH': ':'.join([os.path.dirname(p) for p in abs_paths]),
      }

    self.PatchObject(osutils, 'SourceEnvironment',
                     side_effect=SourceEnvironmentMock)
    file_list = (
        'goma/goma_ctl.py',
        'clang/clang',
        'chromite/parallel_emerge',
    )
    abs_paths = [os.path.join(self.tempdir, relpath) for relpath in file_list]
    for p in abs_paths:
      osutils.Touch(p, makedirs=True, mode=0o755)

    with cros_test_lib.LoggingCapturer() as logs:
      cros_chrome_sdk.ChromeSDKCommand._VerifyGoma(None)
      cros_chrome_sdk.ChromeSDKCommand._VerifyChromiteBin(None)

    for msg in ['managed Goma', 'default Chromite']:
      self.AssertLogsMatch(logs, msg)
