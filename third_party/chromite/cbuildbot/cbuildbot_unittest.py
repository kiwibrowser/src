# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for the cbuildbot script."""

from __future__ import print_function

import argparse
import glob
import optparse  # pylint: disable=deprecated-module
import os

from chromite.cbuildbot import cbuildbot_run
from chromite.lib import cgroups
from chromite.cbuildbot import commands
from chromite.lib import config_lib_unittest
from chromite.lib import constants
from chromite.cbuildbot import manifest_version
from chromite.cbuildbot.builders import simple_builders
from chromite.lib import cidb
from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import osutils
from chromite.lib import parallel
from chromite.lib import partial_mock
from chromite.scripts import cbuildbot


# pylint: disable=protected-access


class BuilderRunMock(partial_mock.PartialMock):
  """Partial mock for BuilderRun class."""

  TARGET = 'chromite.cbuildbot.cbuildbot_run._BuilderRunBase'
  ATTRS = ('GetVersionInfo', 'DetermineChromeVersion',)

  def __init__(self, verinfo):
    super(BuilderRunMock, self).__init__()
    self._version_info = verinfo

  def GetVersionInfo(self, _inst):
    """This way builders don't have to set the version from the overlay"""
    return self._version_info

  def DetermineChromeVersion(self, _inst):
    """Normaly this runs a portage command to look at the chrome ebuild"""
    return self._version_info.chrome_branch


class SimpleBuilderTestCase(cros_test_lib.MockTestCase):
  """Common stubs for SimpleBuilder tests."""

  CHROME_BRANCH = '27'
  VERSION = '1234.5.6'

  def setUp(self):
    verinfo = manifest_version.VersionInfo(
        version_string=self.VERSION, chrome_branch=self.CHROME_BRANCH)

    self.StartPatcher(BuilderRunMock(verinfo))

    self.PatchObject(simple_builders.SimpleBuilder, 'GetVersionInfo',
                     return_value=verinfo)


class TestArgsparseError(Exception):
  """Exception used by parser.error() mock to halt execution."""


class TestHaltedException(Exception):
  """Exception used by mocks to halt execution without indicating failure."""


class RunBuildStagesTest(cros_test_lib.RunCommandTempDirTestCase,
                         SimpleBuilderTestCase):
  """Test that cbuildbot runs the appropriate stages for a given config."""

  def setUp(self):
    self.buildroot = os.path.join(self.tempdir, 'buildroot')
    osutils.SafeMakedirs(self.buildroot)
    # Always stub RunCommmand out as we use it in every method.
    self.site_config = config_lib_unittest.MockSiteConfig()
    self.build_config = config_lib_unittest.MockBuildConfig()
    self.bot_id = self.build_config.name
    self.build_config['master'] = False
    self.build_config['important'] = False

    # Use the cbuildbot parser to create properties and populate default values.
    self.parser = cbuildbot._CreateParser()

    argv = ['-r', self.buildroot, '--buildbot', '--debug', self.bot_id]
    self.options = cbuildbot.ParseCommandLine(self.parser, argv)
    self.options.bootstrap = False
    self.options.clean = False
    self.options.resume = False
    self.options.sync = False
    self.options.build = False
    self.options.uprev = False
    self.options.tests = False
    self.options.archive = False
    self.options.remote_test_status = False
    self.options.patches = None
    self.options.prebuilts = False

    self._manager = parallel.Manager()
    self._manager.__enter__()
    self.run = cbuildbot_run.BuilderRun(self.options, self.site_config,
                                        self.build_config, self._manager)

    self.rc.AddCmdResult(
        [constants.PATH_TO_CBUILDBOT, '--reexec-api-version'],
        output=constants.REEXEC_API_VERSION)

  def tearDown(self):
    # Mimic exiting a 'with' statement.
    if hasattr(self, '_manager'):
      self._manager.__exit__(None, None, None)

  def testChromeosOfficialSet(self):
    """Verify that CHROMEOS_OFFICIAL is set correctly."""
    self.build_config['chromeos_official'] = True

    cidb.CIDBConnectionFactory.SetupNoCidb()

    # Clean up before.
    os.environ.pop('CHROMEOS_OFFICIAL', None)
    simple_builders.SimpleBuilder(self.run).Run()
    self.assertIn('CHROMEOS_OFFICIAL', os.environ)

  def testChromeosOfficialNotSet(self):
    """Verify that CHROMEOS_OFFICIAL is not always set."""
    self.build_config['chromeos_official'] = False

    cidb.CIDBConnectionFactory.SetupNoCidb()

    # Clean up before.
    os.environ.pop('CHROMEOS_OFFICIAL', None)
    simple_builders.SimpleBuilder(self.run).Run()
    self.assertNotIn('CHROMEOS_OFFICIAL', os.environ)


class LogTest(cros_test_lib.TempDirTestCase):
  """Test logging functionality."""

  def _generateLogs(self, num):
    """Generates cbuildbot.log and num backups."""
    with open(os.path.join(self.tempdir, 'cbuildbot.log'), 'w') as f:
      f.write(str(num + 1))

    for i in range(1, num + 1):
      with open(os.path.join(self.tempdir, 'cbuildbot.log.' + str(i)),
                'w') as f:
        f.write(str(i))

  def testZeroToOneLogs(self):
    """Test beginning corner case."""
    self._generateLogs(0)
    cbuildbot._BackupPreviousLog(os.path.join(self.tempdir, 'cbuildbot.log'),
                                 backup_limit=25)
    with open(os.path.join(self.tempdir, 'cbuildbot.log.1')) as f:
      self.assertEquals(f.readline(), '1')

  def testNineToTenLogs(self):
    """Test handling *.log.9 to *.log.10 (correct sorting)."""
    self._generateLogs(9)
    cbuildbot._BackupPreviousLog(os.path.join(self.tempdir, 'cbuildbot.log'),
                                 backup_limit=25)
    with open(os.path.join(self.tempdir, 'cbuildbot.log.10')) as f:
      self.assertEquals(f.readline(), '10')

  def testOverLimit(self):
    """Test going over the limit and having to purge old logs."""
    self._generateLogs(25)
    cbuildbot._BackupPreviousLog(os.path.join(self.tempdir, 'cbuildbot.log'),
                                 backup_limit=25)
    with open(os.path.join(self.tempdir, 'cbuildbot.log.26')) as f:
      self.assertEquals(f.readline(), '26')

    self.assertEquals(len(glob.glob(os.path.join(self.tempdir, 'cbuildbot*'))),
                      25)


class InterfaceTest(cros_test_lib.MockTestCase, cros_test_lib.LoggingTestCase):
  """Test the command line interface."""

  _GENERIC_PREFLIGHT = 'amd64-generic-paladin'
  _BUILD_ROOT = '/b/test_build1'

  def setUp(self):
    self.parser = cbuildbot._CreateParser()
    self.site_config = config_lib_unittest.MockSiteConfig()

  def assertDieSysExit(self, *args, **kwargs):
    self.assertRaises(cros_build_lib.DieSystemExit, *args, **kwargs)

  def testDepotTools(self):
    """Test that the entry point used by depot_tools works."""
    path = os.path.join(constants.SOURCE_ROOT, 'chromite', 'bin', 'cbuildbot')

    # Verify the tests below actually are testing correct behaviour;
    # specifically that it doesn't always just return 0.
    self.assertRaises(cros_build_lib.RunCommandError,
                      cros_build_lib.RunCommand,
                      ['cbuildbot', '--monkeys'], cwd=constants.SOURCE_ROOT)

    # Validate depot_tools lookup.
    cros_build_lib.RunCommand(
        ['cbuildbot', '--help'], cwd=constants.SOURCE_ROOT, capture_output=True)

    # Validate buildbot invocation pathway.
    cros_build_lib.RunCommand(
        [path, '--help'], cwd=constants.SOURCE_ROOT, capture_output=True)

  def testBuildBotOption(self):
    """Test that --buildbot option unsets debug flag."""
    args = ['-r', self._BUILD_ROOT, '--buildbot', self._GENERIC_PREFLIGHT]
    options = cbuildbot.ParseCommandLine(self.parser, args)
    self.assertFalse(options.debug)
    self.assertTrue(options.buildbot)

  def testBuildBotWithDebugOption(self):
    """Test that --debug option overrides --buildbot option."""
    args = ['-r', self._BUILD_ROOT, '--buildbot', '--debug',
            self._GENERIC_PREFLIGHT]
    options = cbuildbot.ParseCommandLine(self.parser, args)
    self.assertTrue(options.debug)
    self.assertTrue(options.buildbot)

  def testBuildBotWithRemotePatches(self):
    """Test that --buildbot errors out with patches."""
    args = ['-r', self._BUILD_ROOT, '--buildbot', '-g', '1234',
            self._GENERIC_PREFLIGHT]
    self.assertDieSysExit(cbuildbot.ParseCommandLine, self.parser, args)

  def testBuildbotDebugWithPatches(self):
    """Test we can test patches with --buildbot --debug."""
    args = ['-r', self._BUILD_ROOT, '--buildbot', '--debug', '-g', '1234',
            self._GENERIC_PREFLIGHT]
    cbuildbot.ParseCommandLine(self.parser, args)

  def testBuildBotWithoutProfileOption(self):
    """Test that no --profile option gets defaulted."""
    args = ['-r', self._BUILD_ROOT, '--buildbot', self._GENERIC_PREFLIGHT]
    options = cbuildbot.ParseCommandLine(self.parser, args)
    self.assertEquals(options.profile, None)

  def testBuildBotWithProfileOption(self):
    """Test that --profile option gets parsed."""
    args = ['-r', self._BUILD_ROOT, '--buildbot',
            '--profile', 'carp', self._GENERIC_PREFLIGHT]
    options = cbuildbot.ParseCommandLine(self.parser, args)
    self.assertEquals(options.profile, 'carp')

  def testValidateClobberUserDeclines_1(self):
    """Test case where user declines in prompt."""
    self.PatchObject(os.path, 'exists', return_value=True)
    self.PatchObject(cros_build_lib, 'GetInput', return_value='No')
    self.assertFalse(commands.ValidateClobber(self._BUILD_ROOT))

  def testValidateClobberUserDeclines_2(self):
    """Test case where user does not enter the full 'yes' pattern."""
    self.PatchObject(os.path, 'exists', return_value=True)
    m = self.PatchObject(cros_build_lib, 'GetInput', side_effect=['asdf', 'No'])
    self.assertFalse(commands.ValidateClobber(self._BUILD_ROOT))
    self.assertEqual(m.call_count, 2)

  def testValidateClobberProtectRunningChromite(self):
    """User should not be clobbering our own source."""
    cwd = os.path.dirname(os.path.realpath(__file__))
    buildroot = os.path.dirname(cwd)
    self.assertDieSysExit(commands.ValidateClobber, buildroot)

  def testValidateClobberProtectRoot(self):
    """User should not be clobbering /"""
    self.assertDieSysExit(commands.ValidateClobber, '/')

  def testBuildBotWithBadChromeRevOption(self):
    """chrome_rev can't be passed an invalid option after chrome_root."""
    args = [
        '--local',
        '--buildroot=/tmp',
        '--chrome_root=.',
        '--chrome_rev=%s' % constants.CHROME_REV_TOT,
        self._GENERIC_PREFLIGHT,
    ]
    self.assertDieSysExit(cbuildbot.ParseCommandLine, self.parser, args)

  def testBuildBotWithBadChromeRootOption(self):
    """chrome_root can't get passed after non-local chrome_rev."""
    args = [
        '--buildbot',
        '--buildroot=/tmp',
        '--chrome_rev=%s' % constants.CHROME_REV_TOT,
        '--chrome_root=.',
        self._GENERIC_PREFLIGHT,
    ]
    self.assertDieSysExit(cbuildbot.ParseCommandLine, self.parser, args)

  def testBuildBotWithBadChromeRevOptionLocal(self):
    """chrome_rev can't be local without chrome_root."""
    args = [
        '--buildbot',
        '--buildroot=/tmp',
        '--chrome_rev=%s' % constants.CHROME_REV_LOCAL,
        self._GENERIC_PREFLIGHT,
    ]
    self.assertDieSysExit(cbuildbot.ParseCommandLine, self.parser, args)

  def testBuildBotWithGoodChromeRootOption(self):
    """chrome_root can be set without chrome_rev."""
    args = [
        '--buildbot',
        '--buildroot=/tmp',
        '--chrome_root=.',
        self._GENERIC_PREFLIGHT,
    ]
    options = cbuildbot.ParseCommandLine(self.parser, args)
    self.assertEquals(options.chrome_rev, constants.CHROME_REV_LOCAL)
    self.assertNotEquals(options.chrome_root, None)

  def testBuildBotWithGoodChromeRevAndRootOption(self):
    """chrome_rev can get reset around chrome_root."""
    args = [
        '--buildbot',
        '--buildroot=/tmp',
        '--chrome_rev=%s' % constants.CHROME_REV_LATEST,
        '--chrome_rev=%s' % constants.CHROME_REV_STICKY,
        '--chrome_rev=%s' % constants.CHROME_REV_TOT,
        '--chrome_rev=%s' % constants.CHROME_REV_TOT,
        '--chrome_rev=%s' % constants.CHROME_REV_STICKY,
        '--chrome_rev=%s' % constants.CHROME_REV_LATEST,
        '--chrome_rev=%s' % constants.CHROME_REV_LOCAL,
        '--chrome_root=.',
        '--chrome_rev=%s' % constants.CHROME_REV_TOT,
        '--chrome_rev=%s' % constants.CHROME_REV_LOCAL,
        self._GENERIC_PREFLIGHT,
    ]
    options = cbuildbot.ParseCommandLine(self.parser, args)
    self.assertEquals(options.chrome_rev, constants.CHROME_REV_LOCAL)
    self.assertNotEquals(options.chrome_root, None)

  def testCreateBranch(self):
    """Test a normal create branch run."""
    args = ['-r', self._BUILD_ROOT,
            '--branch-name', 'refs/heads/test', constants.BRANCH_UTIL_CONFIG]
    self.assertDieSysExit(cbuildbot.ParseCommandLine, self.parser, args)

  def testCreateBranchNoVersion(self):
    """Test we require --version with branch-util."""
    with cros_test_lib.LoggingCapturer() as logger:
      args = ['-r', self._BUILD_ROOT, constants.BRANCH_UTIL_CONFIG]
      self.assertDieSysExit(cbuildbot.ParseCommandLine, self.parser, args)
      self.AssertLogsContain(logger, '--branch-name')

  def testCreateBranchDelete(self):
    """Test we don't require --version with --delete."""
    args = ['-r', self._BUILD_ROOT,
            '--delete-branch', '--branch-name', 'refs/heads/test',
            constants.BRANCH_UTIL_CONFIG]
    cbuildbot.ParseCommandLine(self.parser, args)

  def testBranchOptionsWithoutBranchConfig(self):
    """Error out when branch options passed in without branch-util config."""
    for extra_args in [['--delete-branch'],
                       ['--branch-name', 'refs/heads/test'],
                       ['--rename-to', 'abc']]:
      with cros_test_lib.LoggingCapturer() as logger:
        args = ['-r', self._BUILD_ROOT, self._GENERIC_PREFLIGHT] + extra_args
        self.assertDieSysExit(cbuildbot.ParseCommandLine, self.parser, args)
        self.AssertLogsContain(logger, 'Cannot specify')


class FullInterfaceTest(cros_test_lib.MockTempDirTestCase):
  """Tests that run the cbuildbot.main() function directly.

  Note this explicitly suppresses automatic VerifyAll() calls; thus if you want
  that checked, you have to invoke it yourself.
  """

  def MakeTestRootDir(self, relpath):
    abspath = os.path.join(self.root, relpath)
    osutils.SafeMakedirs(abspath)
    return abspath

  def setUp(self):
    self.root = self.tempdir
    self.buildroot = self.MakeTestRootDir('build_root')
    self.sourceroot = self.MakeTestRootDir('source_root')
    self.trybot_root = self.MakeTestRootDir('trybot')
    self.trybot_internal_root = self.MakeTestRootDir('trybot-internal')
    self.external_marker = os.path.join(self.trybot_root, '.trybot')
    self.internal_marker = os.path.join(self.trybot_internal_root, '.trybot')

    osutils.SafeMakedirs(os.path.join(self.sourceroot, '.repo', 'manifests'))
    osutils.SafeMakedirs(os.path.join(self.sourceroot, '.repo', 'repo'))

    # Stub out all relevant methods regardless of whether they are called in the
    # specific test case.
    self.PatchObject(optparse.OptionParser, 'error',
                     side_effect=TestArgsparseError())
    self.PatchObject(argparse.ArgumentParser, 'error',
                     side_effect=TestArgsparseError())
    self.inchroot_mock = self.PatchObject(cros_build_lib, 'IsInsideChroot',
                                          return_value=False)
    self.input_mock = self.PatchObject(cros_build_lib, 'GetInput',
                                       side_effect=Exception())
    self.PatchObject(cbuildbot, '_RunBuildStagesWrapper', return_value=True)
    # Suppress cgroups code.  For cbuildbot invocation, it doesn't hugely
    # care about cgroups- that's a blackbox to it.  As such these unittests
    # should not be sensitive to it.
    self.PatchObject(cgroups.Cgroup, 'IsSupported',
                     return_value=True)
    self.PatchObject(cgroups, 'SimpleContainChildren')

  def assertMain(self, args, common_options=True):
    if common_options:
      args.extend(['--sourceroot', self.sourceroot, '--notee'])
    return cbuildbot.main(args)

  def testNullArgsStripped(self):
    """Test that null args are stripped out and don't cause error."""
    self.assertMain(['-r', self.buildroot, '', '',
                     'amd64-generic-pre-cq'])

  def testMultipleConfigsError(self):
    """Test that multiple configs cause error."""
    with self.assertRaises(cros_build_lib.DieSystemExit):
      self.assertMain(['-r', self.buildroot,
                       'arm-generic-pre-cq',
                       'amd64-generic-pre-cq'])

  def testBuildbotDiesInChroot(self):
    """Buildbot should quit if run inside a chroot."""
    self.inchroot_mock.return_value = True
    with self.assertRaises(cros_build_lib.DieSystemExit):
      self.assertMain(['--debug', '-r', self.buildroot,
                       'amd64-generic-pre-cq'])

  def testBuildBotOnNonCIBuilder(self):
    """Test BuildBot On Non-CIBuilder

    Buildbot should quite if run in a non-CIBuilder without
    both debug and remote.
    """
    if not cros_build_lib.HostIsCIBuilder():
      with self.assertRaises(cros_build_lib.DieSystemExit):
        self.assertMain(['--buildbot', 'amd64-generic-pre-cq'])
