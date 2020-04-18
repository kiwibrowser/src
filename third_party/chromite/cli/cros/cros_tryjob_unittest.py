# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests the `cros chroot` command."""

from __future__ import print_function

import mock

from chromite.cli import command_unittest
from chromite.lib import config_lib
from chromite.lib import cros_build_lib
from chromite.cli.cros import cros_tryjob
from chromite.lib import cros_test_lib


class MockTryjobCommand(command_unittest.MockCommand):
  """Mock out the `cros tryjob` command."""
  TARGET = 'chromite.cli.cros.cros_tryjob.TryjobCommand'
  TARGET_CLASS = cros_tryjob.TryjobCommand
  COMMAND = 'tryjob'


class TryjobTest(cros_test_lib.MockTestCase):
  """Base class for Tryjob command tests."""

  def setUp(self):
    self.cmd_mock = None

  def SetupCommandMock(self, cmd_args):
    """Sets up the `cros tryjob` command mock."""
    self.cmd_mock = MockTryjobCommand(cmd_args)
    self.StartPatcher(self.cmd_mock)

    return self.cmd_mock.inst.options


class TryjobTestPrintKnownConfigs(TryjobTest):
  """Test the PrintKnownConfigs function."""

  def setUp(self):
    self.site_config = config_lib.GetConfig()

  def testConfigsToPrintAllIncluded(self):
    """Test we can generate results for --list."""
    tryjob_configs = cros_tryjob.ConfigsToPrint(
        self.site_config, production=False, build_config_fragments=[])

    release_configs = cros_tryjob.ConfigsToPrint(
        self.site_config, production=True, build_config_fragments=[])

    self.assertEqual(len(self.site_config),
                     len(tryjob_configs) + len(release_configs))

  def testConfigsToPrintFiltered(self):
    """Test ConfigsToPrint filters correctly."""
    tryjob_configs = cros_tryjob.ConfigsToPrint(
        self.site_config, production=False, build_config_fragments=[])

    samus_tryjob_configs = cros_tryjob.ConfigsToPrint(
        self.site_config, production=False, build_config_fragments=['samus'])

    samus_release_tryjob_configs = cros_tryjob.ConfigsToPrint(
        self.site_config, production=False,
        build_config_fragments=['samus', 'release'])

    # Prove expecting things are there.
    self.assertIn(self.site_config['samus-release-tryjob'],
                  tryjob_configs)
    self.assertIn(self.site_config['samus-release-tryjob'],
                  samus_tryjob_configs)
    self.assertIn(self.site_config['samus-release-tryjob'],
                  samus_release_tryjob_configs)

    # Unexpecting things aren't.
    self.assertNotIn(self.site_config['samus-release'],
                     tryjob_configs)
    self.assertNotIn(self.site_config['glados-release'],
                     samus_tryjob_configs)
    self.assertNotIn(self.site_config['samus-full'],
                     samus_release_tryjob_configs)

    # And that we really filtered something out in every case.
    self.assertLess(len(samus_release_tryjob_configs),
                    len(samus_tryjob_configs))

    self.assertLess(len(samus_tryjob_configs), len(tryjob_configs))

  def testListTryjobs(self):
    """Test we can generate results for --list."""
    with cros_build_lib.OutputCapturer() as output:
      cros_tryjob.PrintKnownConfigs(
          self.site_config, production=False, build_config_fragments=[])

    # We have at least 100 lines of output, and no error out.
    self.assertGreater(len(output.GetStdoutLines()), 100)
    self.assertFalse(output.GetStderr())

  def testListProduction(self):
    """Test we can generate results for --production --list."""
    with cros_build_lib.OutputCapturer() as output:
      cros_tryjob.PrintKnownConfigs(
          self.site_config, production=True, build_config_fragments=[])

    # We have at least 100 lines of output, and no error out.
    self.assertGreater(len(output.GetStdoutLines()), 100)
    self.assertFalse(output.GetStderr())

class TryjobTestParsing(TryjobTest):
  """Test cros try command line parsing."""

  def setUp(self):
    self.expected = {
        'where': cros_tryjob.REMOTE,
        'buildroot': None,
        'branch': 'master',
        'production': False,
        'yes': False,
        'list': False,
        'gerrit_patches': [],
        'local_patches': [],
        'passthrough': None,
        'passthrough_raw': None,
        'build_configs': ['eve-pre-cq'],
    }

  def testMinimalParsing(self):
    """Tests flow for an interactive session."""
    self.SetupCommandMock(['eve-pre-cq'])
    options = self.cmd_mock.inst.options

    self.assertDictContainsSubset(self.expected, vars(options))

  def testComplexParsingRemote(self):
    """Tests flow for an interactive session."""
    self.SetupCommandMock([
        '--remote',
        '--yes',
        '--latest-toolchain', '--nochromesdk',
        '--hwtest', '--notests', '--novmtests', '--noimagetests',
        '--buildroot', '/buildroot',
        '--timeout', '5', '--sanity-check-build',
        '--gerrit-patches', '123', '-g', '*123', '-g', '123..456',
        '--local-patches', 'chromiumos/chromite:tryjob', '-p', 'other:other',
        '--chrome_version', 'chrome_git_hash',
        '--debug-cidb',
        '--pass-through=--cbuild-arg', '--pass-through', 'bar',
        '--list',
        'eve-pre-cq', 'eve-release',
    ])
    options = self.cmd_mock.inst.options

    self.expected.update({
        'where': cros_tryjob.REMOTE,
        'buildroot': '/buildroot',
        'branch': 'master',
        'yes': True,
        'list': True,
        'gerrit_patches': ['123', '*123', '123..456'],
        'local_patches': ['chromiumos/chromite:tryjob', 'other:other'],
        'passthrough': [
            '--latest-toolchain', '--nochromesdk',
            '--hwtest', '--notests', '--novmtests', '--noimagetests',
            '--timeout', '5', '--sanity-check-build',
            '--chrome_version', 'chrome_git_hash',
            '--debug-cidb',
        ],
        'passthrough_raw': ['--cbuild-arg', 'bar'],
        'build_configs': ['eve-pre-cq', 'eve-release'],
    })

    self.assertDictContainsSubset(self.expected, vars(options))

  def testComplexParsingLocal(self):
    """Tests flow for an interactive session."""
    self.SetupCommandMock([
        '--yes',
        '--latest-toolchain', '--nochromesdk',
        '--hwtest', '--notests', '--novmtests', '--noimagetests',
        '--local', '--buildroot', '/buildroot',
        '--timeout', '5', '--sanity-check-build',
        '--gerrit-patches', '123', '-g', '*123', '-g', '123..456',
        '--local-patches', 'chromiumos/chromite:tryjob', '-p', 'other:other',
        '--chrome_version', 'chrome_git_hash',
        '--debug-cidb',
        '--pass-through=--cbuild-arg', '--pass-through', 'bar',
        '--list',
        'eve-paladin', 'eve-release',
    ])
    options = self.cmd_mock.inst.options

    self.expected.update({
        'where': cros_tryjob.LOCAL,
        'buildroot': '/buildroot',
        'branch': 'master',
        'yes': True,
        'list': True,
        'gerrit_patches': ['123', '*123', '123..456'],
        'local_patches': ['chromiumos/chromite:tryjob', 'other:other'],
        'passthrough': [
            '--latest-toolchain', '--nochromesdk',
            '--hwtest', '--notests', '--novmtests', '--noimagetests',
            '--timeout', '5', '--sanity-check-build',
            '--chrome_version', 'chrome_git_hash',
            '--debug-cidb',
        ],
        'passthrough_raw': ['--cbuild-arg', 'bar'],
        'build_configs': ['eve-paladin', 'eve-release'],
    })

    self.assertDictContainsSubset(self.expected, vars(options))

  def testComplexParsingCbuildbot(self):
    """Tests flow for an interactive session."""
    self.SetupCommandMock([
        '--yes',
        '--latest-toolchain', '--nochromesdk',
        '--hwtest', '--notests', '--novmtests', '--noimagetests',
        '--cbuildbot', '--buildroot', '/buildroot',
        '--timeout', '5', '--sanity-check-build',
        '--gerrit-patches', '123', '-g', '*123', '-g', '123..456',
        '--local-patches', 'chromiumos/chromite:tryjob', '-p', 'other:other',
        '--chrome_version', 'chrome_git_hash',
        '--pass-through=--cbuild-arg', '--pass-through', 'bar',
        '--list',
        'eve-pre-cq', 'eve-release',
    ])
    options = self.cmd_mock.inst.options

    self.expected.update({
        'where': cros_tryjob.CBUILDBOT,
        'buildroot': '/buildroot',
        'branch': 'master',
        'yes': True,
        'list': True,
        'gerrit_patches': ['123', '*123', '123..456'],
        'local_patches': ['chromiumos/chromite:tryjob', 'other:other'],
        'passthrough': [
            '--latest-toolchain', '--nochromesdk',
            '--hwtest', '--notests', '--novmtests', '--noimagetests',
            '--timeout', '5', '--sanity-check-build',
            '--chrome_version', 'chrome_git_hash',
        ],
        'passthrough_raw': ['--cbuild-arg', 'bar'],
        'build_configs': ['eve-pre-cq', 'eve-release'],
    })

    self.assertDictContainsSubset(self.expected, vars(options))

  def testPayloadsParsing(self):
    """Tests flow for an interactive session."""
    self.SetupCommandMock([
        '--version', '9795.0.0', '--channel', 'canary', 'eve-payloads'
    ])
    options = self.cmd_mock.inst.options

    self.expected.update({
        'passthrough': ['--version', '9795.0.0', '--channel', 'canary'],
        'build_configs': ['eve-payloads'],
    })

    self.assertDictContainsSubset(self.expected, vars(options))


class TryjobTestAdjustOptions(TryjobTest):
  """Test cros_tryjob.AdjustOptions."""

  def testRemote(self):
    """Test default remote buildroot."""
    self.SetupCommandMock(['config'])
    options = self.cmd_mock.inst.options

    cros_tryjob.AdjustOptions(options)

    self.assertIsNone(options.buildroot)

  def testLocalDefault(self):
    """Test default local buildroot."""
    self.SetupCommandMock(['--local', 'config'])
    options = self.cmd_mock.inst.options

    cros_tryjob.AdjustOptions(options)

    self.assertTrue(options.buildroot.endswith('/tryjob'))

  def testLocalExplicit(self):
    """Test explicit local buildroot."""
    self.SetupCommandMock(['--local', '--buildroot', '/buildroot', 'config'])
    options = self.cmd_mock.inst.options

    cros_tryjob.AdjustOptions(options)

    self.assertEqual(options.buildroot, '/buildroot')

  def testCbuildbotDefault(self):
    """Test default cbuildbot buildroot."""
    self.SetupCommandMock(['--cbuildbot', 'config'])
    options = self.cmd_mock.inst.options

    cros_tryjob.AdjustOptions(options)

    self.assertTrue(options.buildroot.endswith('/cbuild'))

  def testCbuildbotExplicit(self):
    """Test explicit cbuildbot buildroot."""
    self.SetupCommandMock(['--cbuildbot',
                           '--buildroot', '/buildroot',
                           'config'])
    options = self.cmd_mock.inst.options

    cros_tryjob.AdjustOptions(options)

    self.assertEqual(options.buildroot, '/buildroot')


class TryjobTestVerifyOptions(TryjobTest):
  """Test cros_tryjob.VerifyOptions."""

  def setUp(self):
    self.site_config = config_lib.GetConfig()

  def testEmpty(self):
    """Test option verification with no options."""
    self.SetupCommandMock([])

    with self.assertRaises(cros_build_lib.DieSystemExit) as cm:
      cros_tryjob.VerifyOptions(self.cmd_mock.inst.options, self.site_config)
    self.assertEqual(cm.exception.code, 1)

  def testMinimal(self):
    """Test option verification with simplest normal options."""
    self.SetupCommandMock([
        '-g', '123',
        'amd64-generic-pre-cq',
    ])
    cros_tryjob.VerifyOptions(self.cmd_mock.inst.options, self.site_config)

    self.assertIsNone(self.cmd_mock.inst.options.buildroot)

  def testMinimalLocal(self):
    """Test option verification with simplest normal options."""
    self.SetupCommandMock([
        '-g', '123',
        '--local',
        'amd64-generic-pre-cq',
    ])
    cros_tryjob.VerifyOptions(self.cmd_mock.inst.options, self.site_config)

  def testMinimalCbuildbot(self):
    """Test option verification with simplest normal options."""
    self.SetupCommandMock([
        '--cbuildbot',
        'amd64-generic-paladin',
    ])
    cros_tryjob.VerifyOptions(self.cmd_mock.inst.options, self.site_config)

  def testComplexLocalTryjob(self):
    """Test option verification with complex mix of options."""
    self.SetupCommandMock([
        '--yes',
        '--latest-toolchain', '--nochromesdk',
        '--hwtest', '--notests', '--novmtests', '--noimagetests',
        '--local', '--buildroot', '/buildroot',
        '--timeout', '5', '--sanity-check-build',
        '--gerrit-patches', '123', '-g', '*123', '-g', '123..456',
        '--chrome_version', 'chrome_git_hash',
        '--committer-email', 'foo@bar',
        '--version', '1.2.3', '--channel', 'chan',
        '--pass-through=--cbuild-arg', '--pass-through=bar',
        'eve-pre-cq', 'eve-release-tryjob',
    ])
    cros_tryjob.VerifyOptions(self.cmd_mock.inst.options, self.site_config)

  def testComplexCbuildbot(self):
    """Test option verification with complex mix of options."""
    self.SetupCommandMock([
        '--yes',
        '--latest-toolchain', '--nochromesdk',
        '--hwtest', '--notests', '--novmtests', '--noimagetests',
        '--cbuildbot', '--buildroot', '/buildroot',
        '--timeout', '5', '--sanity-check-build',
        '--gerrit-patches', '123', '-g', '*123', '-g', '123..456',
        '--committer-email', 'foo@bar',
        '--version', '1.2.3', '--channel', 'chan',
        '--pass-through=--cbuild-arg', '--pass-through=bar',
        'eve-paladin', 'eve-release',
    ])
    cros_tryjob.VerifyOptions(self.cmd_mock.inst.options, self.site_config)

  def testComplexRemoteTryjob(self):
    """Test option verification with complex mix of options."""
    self.SetupCommandMock([
        '--swarming',
        '--yes',
        '--latest-toolchain', '--nochromesdk',
        '--hwtest', '--notests', '--novmtests', '--noimagetests',
        '--timeout', '5', '--sanity-check-build',
        '--gerrit-patches', '123', '-g', '*123', '-g', '123..456',
        '--chrome_version', 'chrome_git_hash',
        '--committer-email', 'foo@bar',
        '--version', '1.2.3', '--channel', 'chan',
        '--pass-through=--cbuild-arg', '--pass-through=bar',
        'eve-pre-cq', 'eve-release-tryjob',
    ])
    cros_tryjob.VerifyOptions(self.cmd_mock.inst.options, self.site_config)

  def testList(self):
    """Test option verification with config list behavior."""
    self.SetupCommandMock([
        '--list',
    ])

    with self.assertRaises(cros_build_lib.DieSystemExit) as cm:
      with cros_build_lib.OutputCapturer(quiet_fail=True):  # Hide list output.
        cros_tryjob.VerifyOptions(self.cmd_mock.inst.options, self.site_config)
    self.assertEqual(cm.exception.code, 0)

  def testListProduction(self):
    """Test option verification with config list behavior."""
    self.SetupCommandMock([
        '--list', '--production',
    ])

    with self.assertRaises(cros_build_lib.DieSystemExit) as cm:
      with cros_build_lib.OutputCapturer(quiet_fail=True):  # Hide list output.
        cros_tryjob.VerifyOptions(self.cmd_mock.inst.options, self.site_config)
    self.assertEqual(cm.exception.code, 0)

  def testProduction(self):
    """Test option verification with production/no patches."""
    self.SetupCommandMock([
        '--production',
        'eve-pre-cq', 'eve-release'
    ])
    cros_tryjob.VerifyOptions(self.cmd_mock.inst.options, self.site_config)

  def testProductionPatches(self):
    """Test option verification with production/patches."""
    self.SetupCommandMock([
        '--production',
        '--gerrit-patches', '123', '-g', '*123', '-g', '123..456',
        'eve-pre-cq', 'eve-release'
    ])

    with self.assertRaises(cros_build_lib.DieSystemExit) as cm:
      cros_tryjob.VerifyOptions(self.cmd_mock.inst.options, self.site_config)
    self.assertEqual(cm.exception.code, 1)

  def testRemoteTryjobProductionConfig(self):
    """Test option verification remote tryjob w/production config."""
    self.SetupCommandMock([
        'eve-pre-cq', 'eve-release'
    ])

    with self.assertRaises(cros_build_lib.DieSystemExit) as cm:
      cros_tryjob.VerifyOptions(self.cmd_mock.inst.options, self.site_config)
    self.assertEqual(cm.exception.code, 1)

  def testLocalTryjobProductionConfig(self):
    """Test option verification local tryjob w/production config."""
    self.SetupCommandMock([
        '--local', 'eve-pre-cq', 'eve-release'
    ])

    with self.assertRaises(cros_build_lib.DieSystemExit) as cm:
      cros_tryjob.VerifyOptions(self.cmd_mock.inst.options, self.site_config)
    self.assertEqual(cm.exception.code, 1)

  def testRemoteTryjobBranchProductionConfig(self):
    """Test a tryjob on a branch for a production config w/confirm."""
    self.SetupCommandMock([
        '--yes', '--branch', 'foo', 'eve-pre-cq', 'eve-release'
    ])

    cros_tryjob.VerifyOptions(self.cmd_mock.inst.options, self.site_config)

  def testRemoteProductionBranchProductionConfig(self):
    """Test a production job on a branch for a production config wo/confirm."""
    self.SetupCommandMock([
        '--production', '--branch', 'foo', 'eve-pre-cq', 'eve-release'
    ])

    cros_tryjob.VerifyOptions(self.cmd_mock.inst.options, self.site_config)

  def testUnknownBuildYes(self):
    """Test option using yes to force accepting an unknown config."""
    self.SetupCommandMock([
        '--yes',
        '-g', '123',
        'unknown-config'
    ])
    cros_tryjob.VerifyOptions(self.cmd_mock.inst.options, self.site_config)

  def testNoPatchesYes(self):
    """Test option using yes to force an unknown config, no patches."""
    self.SetupCommandMock([
        '--yes',
        'unknown-config'
    ])
    cros_tryjob.VerifyOptions(self.cmd_mock.inst.options, self.site_config)


class TryjobTestCbuildbotArgs(TryjobTest):
  """Test cros_tryjob.CbuildbotArgs."""

  def helperOptionsToCbuildbotArgs(self, args_in):
    """Convert cros tryjob arguments -> cbuildbot arguments.

    Does not do all intermediate steps, only for testing CbuildbotArgs.
    """
    self.SetupCommandMock(args_in)
    options = self.cmd_mock.inst.options
    cros_tryjob.AdjustOptions(options)
    args_out = cros_tryjob.CbuildbotArgs(options)
    return args_out

  def testCbuildbotArgsMinimal(self):
    args_in = ['foo-build']

    args_out = self.helperOptionsToCbuildbotArgs(args_in)

    self.assertEqual(args_out, [
        '--remote-trybot', '-b', 'master',
    ])

  def testCbuildbotArgsSimpleRemote(self):
    args_in = ['-g', '123', 'foo-build']

    args_out = self.helperOptionsToCbuildbotArgs(args_in)

    self.assertEqual(args_out, [
        '--remote-trybot', '-b', 'master', '-g', '123',
    ])

  def testCbuildbotArgsSimpleLocal(self):
    args_in = [
        '--local', '-g', '123', 'foo-build',
    ]

    args_out = self.helperOptionsToCbuildbotArgs(args_in)

    # Default buildroot changes.
    self.assertEqual(args_out, [
        '--no-buildbot-tags', '--debug',
        '--buildroot', mock.ANY,
        '-b', 'master',
        '-g', '123',
    ])

  def testCbuildbotArgsComplexRemote(self):
    args_in = [
        '--yes',
        '--latest-toolchain', '--nochromesdk',
        '--hwtest', '--notests', '--novmtests', '--noimagetests',
        '--timeout', '5', '--sanity-check-build',
        '--gerrit-patches', '123', '-g', '*123', '-g', '123..456',
        '--chrome_version', 'chrome_git_hash',
        '--committer-email', 'foo@bar',
        '--branch', 'source_branch',
        '--version', '1.2.3', '--channel', 'chan',
        '--branch-name', 'test_branch', '--rename-to', 'new_branch',
        '--delete-branch', '--force-create', '--skip-remote-push',
        '--pass-through=--cbuild-arg', '--pass-through=bar',
        'eve-pre-cq', 'eve-release',
    ]

    args_out = self.helperOptionsToCbuildbotArgs(args_in)

    self.assertEqual(args_out, [
        '--remote-trybot', '-b', 'source_branch',
        '-g', '123', '-g', '*123', '-g', '123..456',
        '--latest-toolchain', '--nochromesdk',
        '--hwtest', '--notests', '--novmtests', '--noimagetests',
        '--timeout', '5', '--sanity-check-build',
        '--chrome_version', 'chrome_git_hash',
        '--version', '1.2.3', '--channel', 'chan',
        '--branch-name', 'test_branch', '--rename-to', 'new_branch',
        '--delete-branch', '--force-create', '--skip-remote-push',
        '--cbuild-arg', 'bar'
    ])

  def testCbuildbotArgsComplexLocal(self):
    args_in = [
        '--local', '--yes',
        '--latest-toolchain', '--nochromesdk',
        '--hwtest', '--notests', '--novmtests', '--noimagetests',
        '--buildroot', '/buildroot',
        '--timeout', '5', '--sanity-check-build',
        '--gerrit-patches', '123', '-g', '*123', '-g', '123..456',
        '--chrome_version', 'chrome_git_hash',
        '--committer-email', 'foo@bar',
        '--branch', 'source_branch',
        '--version', '1.2.3', '--channel', 'chan',
        '--branch-name', 'test_branch', '--rename-to', 'new_branch',
        '--delete-branch', '--force-create', '--skip-remote-push',
        '--pass-through=--cbuild-arg', '--pass-through=bar',
        'eve-pre-cq', 'eve-release',
    ]

    args_out = self.helperOptionsToCbuildbotArgs(args_in)

    self.assertEqual(args_out, [
        '--no-buildbot-tags', '--debug',
        '--buildroot', '/buildroot',
        '-b', 'source_branch',
        '-g', '123', '-g', '*123', '-g', '123..456',
        '--latest-toolchain', '--nochromesdk',
        '--hwtest', '--notests', '--novmtests', '--noimagetests',
        '--timeout', '5', '--sanity-check-build',
        '--chrome_version', 'chrome_git_hash',
        '--version', '1.2.3', '--channel', 'chan',
        '--branch-name', 'test_branch', '--rename-to', 'new_branch',
        '--delete-branch', '--force-create', '--skip-remote-push',
        '--cbuild-arg', 'bar'
    ])

  def testCbuildbotArgsComplexCbuildbot(self):
    args_in = [
        '--cbuildbot', '--yes',
        '--latest-toolchain', '--nochromesdk',
        '--hwtest', '--notests', '--novmtests', '--noimagetests',
        '--buildroot', '/buildroot',
        '--timeout', '5', '--sanity-check-build',
        '--gerrit-patches', '123', '-g', '*123', '-g', '123..456',
        '--committer-email', 'foo@bar',
        '--branch', 'source_branch',
        '--version', '1.2.3', '--channel', 'chan',
        '--branch-name', 'test_branch', '--rename-to', 'new_branch',
        '--delete-branch', '--force-create', '--skip-remote-push',
        '--pass-through=--cbuild-arg', '--pass-through=bar',
        'eve-paladin', 'eve-release',
    ]

    args_out = self.helperOptionsToCbuildbotArgs(args_in)

    self.assertEqual(args_out, [
        '--buildbot', '--nobootstrap', '--noreexec',
        '--no-buildbot-tags', '--debug',
        '--buildroot', '/buildroot',
        '-b', 'source_branch',
        '-g', '123', '-g', '*123', '-g', '123..456',
        '--latest-toolchain', '--nochromesdk',
        '--hwtest', '--notests', '--novmtests', '--noimagetests',
        '--timeout', '5', '--sanity-check-build',
        '--version', '1.2.3', '--channel', 'chan',
        '--branch-name', 'test_branch', '--rename-to', 'new_branch',
        '--delete-branch', '--force-create', '--skip-remote-push',
        '--cbuild-arg', 'bar'
    ])

  def testCbuildbotArgsProductionRemote(self):
    args_in = [
        '--production', 'foo-build',
    ]

    args_out = self.helperOptionsToCbuildbotArgs(args_in)

    self.assertEqual(args_out, [
        '--buildbot', '-b', 'master',
    ])

  def testCbuildbotArgsProductionLocal(self):
    args_in = [
        '--local', '--production', 'foo-build',
    ]

    args_out = self.helperOptionsToCbuildbotArgs(args_in)

    # Default buildroot changes.
    self.assertEqual(args_out, [
        '--no-buildbot-tags', '--buildbot',
        '--buildroot', mock.ANY,
        '-b', 'master',
    ])

class TryjobTestDisplayLabel(TryjobTest):
  """Test the helper function DisplayLabel."""

  def FindLabel(self, args):
    site_config = config_lib.GetConfig()
    options = self.SetupCommandMock(args)
    config_name = options.build_configs[-1]
    return cros_tryjob.DisplayLabel(site_config, options, config_name)

  def testMasterTryjob(self):
    label = self.FindLabel(['eve-paladin-tryjob'])
    self.assertEqual(label, 'tryjob')

  def testMasterPreCQ(self):
    label = self.FindLabel(['eve-pre-cq'])
    self.assertEqual(label, 'pre_cq')

  def testMasterUnknown(self):
    label = self.FindLabel(['bogus-config'])
    self.assertEqual(label, 'tryjob')

  def testMasterKnownProduction(self):
    label = self.FindLabel(['--production', 'eve-paladin'])
    self.assertEqual(label, 'production_tryjob')

  def testMasterUnknownProduction(self):
    label = self.FindLabel(['--production', 'bogus-config'])
    self.assertEqual(label, 'production_tryjob')

  def testBranchTryjob(self):
    label = self.FindLabel(['--branch=test-branch', 'eve-pre-cq'])
    self.assertEqual(label, 'tryjob')

  def testBranchProduction(self):
    label = self.FindLabel(['--production', '--branch=test-branch',
                            'eve-pre-cq'])
    self.assertEqual(label, 'production_tryjob')
