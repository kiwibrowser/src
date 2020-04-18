# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for config."""

from __future__ import print_function

import copy
import json
import mock
import re
import cPickle

from chromite.cbuildbot import builders
from chromite.cbuildbot import chromeos_config
from chromite.lib import config_lib
from chromite.lib import config_lib_unittest
from chromite.lib import constants
from chromite.cbuildbot.builders import generic_builders
from chromite.lib import cros_test_lib
from chromite.lib import git
from chromite.lib import osutils
from chromite.scripts import cros_show_waterfall_layout

# pylint: disable=protected-access

CHROMIUM_WATCHING_URL = (
    'http://src.chromium.org/chrome/trunk/tools/build/masters/'
    'master.chromium.chromiumos/master_chromiumos_cros_cfg.py'
)


class ChromeosConfigTestBase(cros_test_lib.OutputTestCase):
  """Base class for tests of chromeos_config.."""

  def setUp(self):
    self.site_config = chromeos_config.GetConfig()

  def isReleaseBranch(self):
    ge_build_config = config_lib.LoadGEBuildConfigFromFile()
    return ge_build_config['release_branch']


class ConfigDumpTest(ChromeosConfigTestBase):
  """Tests related to config_dump.json & chromeos_config.py"""

  def testDump(self):
    """Ensure generated files are up to date."""
    # config_dump.json
    new_dump = self.site_config.SaveConfigToString()
    old_dump = osutils.ReadFile(constants.CHROMEOS_CONFIG_FILE).rstrip()

    if new_dump != old_dump:
      if cros_test_lib.GlobalTestConfig.UPDATE_GENERATED_FILES:
        osutils.WriteFile(constants.CHROMEOS_CONFIG_FILE, new_dump)
      else:
        self.fail('config_dump.json does not match the '
                  'defined configs. Run '
                  'cbuildbot/chromeos_config_unittest --update')

    # watefall_layout_dump.txt
    with self.OutputCapturer() as output:
      config_lib.ClearConfigCache()
      cros_show_waterfall_layout.main(['--format', 'text'])

    new_dump = output.GetStdout()
    old_dump = osutils.ReadFile(constants.WATERFALL_CONFIG_FILE)

    if new_dump != old_dump:
      if cros_test_lib.GlobalTestConfig.UPDATE_GENERATED_FILES:
        osutils.WriteFile(constants.WATERFALL_CONFIG_FILE, new_dump)
      else:
        self.fail('waterfall_layout_dump.txt does not match the '
                  'defined configs. Run '
                  'cbuildbot/chromeos_config_unittest --update')

  def testSaveLoadReload(self):
    """Make sure that loading and reloading the config is a no-op."""
    site_config_str = self.site_config.SaveConfigToString()
    loaded = config_lib.LoadConfigFromString(site_config_str)

    self.longMessage = True
    for name in self.site_config.iterkeys():
      self.assertDictEqual(loaded[name], self.site_config[name], name)

    # This includes templates and the default build config.
    self.assertEqual(self.site_config, loaded)

    loaded_str = loaded.SaveConfigToString()

    self.assertEqual(site_config_str, loaded_str)

    # Cycle through save load again, just for completeness.
    loaded2 = config_lib.LoadConfigFromString(loaded_str)
    loaded2_str = loaded2.SaveConfigToString()
    self.assertEqual(loaded_str, loaded2_str)

  def testFullDump(self):
    """Make sure we can dump long content without crashing."""
    # Note: This test takes ~ 1 second to run.
    self.site_config.DumpExpandedConfigToString()


class FindConfigsForBoardTest(cros_test_lib.TestCase):
  """Test locating of official build for a board.

  This test class used to live in config_lib_unittest, but was moved
  here to help make lib/ hermetic and not depend on chromite/cbuildbot.
  """

  def setUp(self):
    self.config = chromeos_config.GetConfig()

  def _CheckFullConfig(
      self, board, external_expected=None, internal_expected=None):
    """Check FindFullConfigsForBoard has expected results.

    Args:
      board: Argument to pass to FindFullConfigsForBoard.
      external_expected: Expected config name (singular) to be found.
      internal_expected: Expected config name (singular) to be found.
    """

    def check_expected(l, expected):
      if expected is not None:
        self.assertTrue(expected in [v['name'] for v in l])

    external, internal = self.config.FindFullConfigsForBoard(board)
    self.assertFalse(external_expected is None and internal_expected is None)
    check_expected(external, external_expected)
    check_expected(internal, internal_expected)

  def _CheckCanonicalConfig(self, board, ending):
    self.assertEquals(
        '-'.join((board, ending)),
        self.config.FindCanonicalConfigForBoard(board)['name'])

  def testExternal(self):
    """Test finding of a full builder."""
    self._CheckFullConfig(
        'amd64-generic', external_expected='amd64-generic-full')

  def testInternal(self):
    """Test finding of a release builder."""
    self._CheckFullConfig('eve', internal_expected='eve-release')

  def testBoth(self):
    """Both an external and internal config exist for board."""
    self._CheckFullConfig(
        'daisy', external_expected='daisy-full',
        internal_expected='daisy-release')

  def testExternalCanonicalResolution(self):
    """Test an external canonical config."""
    self._CheckCanonicalConfig('amd64-generic', 'full')

  def testInternalCanonicalResolution(self):
    """Test prefer internal over external when both exist."""
    self._CheckCanonicalConfig('daisy', 'release')

  def testAFDOCanonicalResolution(self):
    """Test prefer non-AFDO over AFDO builder."""
    self._CheckCanonicalConfig('eve', 'release')

  def testOneFullConfigPerBoard(self):
    """There is at most one 'full' config for a board."""
    # Verifies that there is one external 'full' and one internal 'release'
    # build per board.  This is to ensure that we fail any new configs that
    # wrongly have names like *-bla-release or *-bla-full. This case can also
    # be caught if the new suffix was added to
    # config_lib.CONFIG_TYPE_DUMP_ORDER
    # (see testNonOverlappingConfigTypes), but that's not guaranteed to happen.
    def AtMostOneConfig(board, label, configs):
      if len(configs) > 1:
        self.fail(
            'Found more than one %s config for %s: %r'
            % (label, board, [c['name'] for c in configs]))

    boards = set()
    for build_config in self.config.itervalues():
      boards.update(build_config['boards'])

    # Sanity check of the boards.
    self.assertTrue(boards)

    for b in boards:
      external, internal = self.config.FindFullConfigsForBoard(b)
      AtMostOneConfig(b, 'external', external)
      AtMostOneConfig(b, 'internal', internal)


class UnifiedBuildConfigTestCase(object):
  """Base test class that builds a fake config model based on unified builds"""

  def setUp(self):
    # Code assumes at least one non-unified build exists, so we're accommodating
    # that by keeping the non-unified reef board.
    self._fake_ge_build_config_json = '''
{
  "metadata_version": "1.0",
  "release_branch": true,
  "reference_board_unified_builds": [
    {
      "name": "coral",
      "reference_board_name": "coral",
      "builder": "RELEASE",
      "experimental": true,
      "arch": "X86_INTERNAL",
      "models" : [
        {
          "name": "coral",
          "board_name": "coral"
        },
        {
          "name": "robo",
          "board_name": "robo",
          "test_suites": ["sanity"],
          "cq_test_enabled": true
        }
      ]
    }
  ],
  "boards": [
    {
      "name": "reef",
      "configs": [
        {
          "builder": "RELEASE",
          "experimental": false,
          "leader_board": true,
          "board_group": "reef",
          "arch": "X86_INTERNAL"
        }
      ]
    }
  ]
}
    '''
    self._fake_ge_build_config = json.loads(self._fake_ge_build_config_json)

    site_params = chromeos_config.SiteParameters()
    defaults = chromeos_config.DefaultSettings(site_params)
    self._site_config = config_lib.SiteConfig(defaults=defaults,
                                              site_params=site_params)
    self._ge_build_config = config_lib.LoadGEBuildConfigFromFile()
    self._boards_dict = chromeos_config.GetBoardTypeToBoardsDict(
        self._ge_build_config)

    chromeos_config.GeneralTemplates(
        self._site_config, self._fake_ge_build_config)
    chromeos_config.ReleaseBuilders(
        self._site_config, self._boards_dict, self._fake_ge_build_config)
    chromeos_config.CqBuilders(
        self._site_config, self._boards_dict, self._fake_ge_build_config)

class UnifiedBuildReleaseBuilders(
    cros_test_lib.OutputTestCase, UnifiedBuildConfigTestCase):
  """Tests that verify how unified builder configs are generated"""

  def setUp(self):
    UnifiedBuildConfigTestCase.setUp(self)

  def testUnifiedReleaseBuilders(self):
    coral_release = self._site_config['coral-release']
    self.assertIsNotNone(coral_release)
    models = coral_release['models']
    self.assertIn(config_lib.ModelTestConfig('coral', 'coral', []), models)
    self.assertIn(
        config_lib.ModelTestConfig('robo', 'robo', ['sanity']), models)

    master_release = self._site_config['master-release']
    self.assertIn('coral-release', master_release['slave_configs'])

class UnifiedBuildCqBuilders(
    cros_test_lib.OutputTestCase, UnifiedBuildConfigTestCase):
  """Tests that verify how unified builder CQ configs are generated"""

  def setUp(self):
    UnifiedBuildConfigTestCase.setUp(self)

  def testUnifiedCqBuilders(self):
    coral_paladin = self._site_config['coral-paladin']
    self.assertIsNotNone(coral_paladin)
    models = coral_paladin['models']
    self.assertEquals(len(models), 1)
    self.assertIn(config_lib.ModelTestConfig('robo', 'robo'), models)

    master_paladin = self._site_config['master-paladin']
    self.assertIn('coral-paladin', master_paladin['slave_configs'])


class ConfigPickleTest(ChromeosConfigTestBase):
  """Test that a config object is pickleable."""

  def testPickle(self):
    bc1 = self.site_config['x86-mario-paladin']
    bc2 = cPickle.loads(cPickle.dumps(bc1))

    self.assertEquals(bc1.boards, bc2.boards)
    self.assertEquals(bc1.name, bc2.name)


class ConfigClassTest(ChromeosConfigTestBase):
  """Tests of the config class itself."""

  def testAppendUseflags(self):
    base_config = config_lib.BuildConfig(useflags=[])
    inherited_config_1 = base_config.derive(
        useflags=chromeos_config.append_useflags(
            ['foo', 'bar', '-baz']))
    inherited_config_2 = inherited_config_1.derive(
        useflags=chromeos_config.append_useflags(['-bar', 'baz']))
    self.assertEqual(base_config.useflags, [])
    self.assertEqual(inherited_config_1.useflags, ['-baz', 'bar', 'foo'])
    self.assertEqual(inherited_config_2.useflags, ['-bar', 'baz', 'foo'])


class CBuildBotTest(ChromeosConfigTestBase):
  """General tests of chromeos_config."""

  def _GetBoardTypeToBoardsDict(self):
    """Get boards dict.

    Returns:
      A dict mapping a board type to a collections of board names.
    """
    ge_build_config = config_lib.LoadGEBuildConfigFromFile()
    return chromeos_config.GetBoardTypeToBoardsDict(ge_build_config)

  def testConfigsKeysMismatch(self):
    """Verify that all configs contain exactly the default keys.

    This checks for mispelled keys, or keys that are somehow removed.
    """
    expected_keys = set(self.site_config.GetDefault().iterkeys())
    for build_name, config in self.site_config.iteritems():
      config_keys = set(config.keys())

      extra_keys = config_keys.difference(expected_keys)
      self.assertFalse(extra_keys, ('Config %s has extra values %s' %
                                    (build_name, list(extra_keys))))

      missing_keys = expected_keys.difference(config_keys)
      self.assertFalse(missing_keys, ('Config %s is missing values %s' %
                                      (build_name, list(missing_keys))))

  def testConfigsHaveName(self):
    """Configs must have names set."""
    for build_name, config in self.site_config.iteritems():
      self.assertTrue(build_name == config['name'])

  def testConfigsHaveValidDisplayLabel(self):
    """Configs must have names set."""
    for build_name, config in self.site_config.iteritems():
      self.assertIn(config.display_label, config_lib.ALL_DISPLAY_LABEL,
                    'Invalid display_label "%s" on "%s"' %
                    (config.display_label, build_name))

  def testConfigsHaveValidLuciBuilder(self):
    """Configs must have names set."""
    for build_name, config in self.site_config.iteritems():
      self.assertIn(config.luci_builder, config_lib.ALL_LUCI_BUILDER,
                    'Invalid luci_builder "%s" on "%s"' %
                    (config.luci_builder, build_name))

  def testMasterSlaveConfigsExist(self):
    """Configs listing slave configs, must list valid configs."""
    for config in self.site_config.itervalues():
      if config.master:
        # Any builder with slaves must set both of these.
        self.assertTrue(config.master)
        self.assertTrue(config.manifest_version)
        self.assertIsNotNone(config.slave_configs)

        # If a builder lists slave config names, ensure they are all valid, and
        # have an assigned waterfall.
        for slave_name in config.slave_configs:
          self.assertIn(slave_name, self.site_config)
          self.assertTrue(
              self.site_config[slave_name].active_waterfall,
              '"%s" is not in an active waterfall' % slave_name)
      else:
        self.assertIsNone(config.slave_configs)

  def testMasterSlaveConfigsSorted(self):
    """Configs listing slave configs, must list valid configs."""
    for config in self.site_config.itervalues():
      if config.slave_configs is not None:
        expected = sorted(config.slave_configs)

        self.assertEqual(config.slave_configs, expected)

  def testConfigUseflags(self):
    """Useflags must be lists.

    Strings are interpreted as arrays of characters for this, which is not
    useful.
    """
    for build_name, config in self.site_config.iteritems():
      useflags = config.get('useflags')
      if not useflags is None:
        self.assertTrue(
            isinstance(useflags, list),
            'Config %s: useflags should be a list.' % build_name)

  def testBoards(self):
    """Verify 'boards' is explicitly set for every config."""
    for build_name, config in self.site_config.iteritems():
      self.assertTrue(isinstance(config['boards'], (tuple, list)),
                      "Config %s doesn't have a list of boards." % build_name)
      self.assertEqual(len(set(config['boards'])), len(config['boards']),
                       'Config %s has duplicate boards.' % build_name)
      if config['builder_class_name'] in (
          'sdk_builders.ChrootSdkBuilder',
          'misc_builders.RefreshPackagesBuilder'):
        self.assertTrue(len(config['boards']) >= 1,
                        'Config %s requires 1 or more boards.' % build_name)

  def testOverlaySettings(self):
    """Verify overlays and push_overlays have legal values."""
    for build_name, config in self.site_config.iteritems():
      overlays = config['overlays']
      push_overlays = config['push_overlays']

      self.assertTrue(overlays in [None, 'public', 'private', 'both'],
                      'Config %s: has unexpected overlays value.' % build_name)
      self.assertTrue(
          push_overlays in [None, 'public', 'private', 'both'],
          'Config %s: has unexpected push_overlays value.' % build_name)

      if overlays == None:
        subset = [None]
      elif overlays == 'public':
        subset = [None, 'public']
      elif overlays == 'private':
        subset = [None, 'private']
      elif overlays == 'both':
        subset = [None, 'public', 'private', 'both']

      self.assertTrue(
          push_overlays in subset,
          ('Config %s: push_overlays should be a subset of overlays.' %
           build_name))

  def testOverlayMaster(self):
    """Verify that only one master is pushing uprevs for each overlay."""
    masters = {}
    for build_name, config in self.site_config.iteritems():
      overlays = config['overlays']
      push_overlays = config['push_overlays']
      if (overlays and push_overlays and config['uprev'] and config['master']
          and not config['branch']):
        other_master = masters.get(push_overlays)
        err_msg = 'Found two masters for push_overlays=%s: %s and %s'
        self.assertFalse(
            other_master, err_msg % (push_overlays, build_name, other_master))
        masters[push_overlays] = build_name

    if 'both' in masters:
      self.assertEquals(len(masters), 1, 'Found too many masters.')

  def testChromeRev(self):
    """Verify chrome_rev has an expected value"""
    for build_name, config in self.site_config.iteritems():
      self.assertTrue(
          config['chrome_rev'] in constants.VALID_CHROME_REVISIONS + [None],
          'Config %s: has unexpected chrome_rev value.' % build_name)
      self.assertFalse(
          config['chrome_rev'] == constants.CHROME_REV_LOCAL,
          'Config %s: has unexpected chrome_rev_local value.' % build_name)
      if config['chrome_rev']:
        self.assertTrue(
            config_lib.IsPFQType(config['build_type']),
            'Config %s: has chrome_rev but is not a PFQ.' % build_name)

  def testValidVMTestType(self):
    """Verify vm_tests has an expected value"""
    for build_name, config in self.site_config.iteritems():
      if config['vm_tests'] is None:
        continue
      for vm_test in config['vm_tests']:
        self.assertTrue(
            vm_test.test_type in constants.VALID_VM_TEST_TYPES,
            'Config %s: has unexpected vm test type value.' % build_name)
        if vm_test.test_type == constants.VM_SUITE_TEST_TYPE:
          self.assertTrue(
              vm_test.test_suite is not None,
              'Config %s: has unexpected vm test suite value.' % build_name)

  def testValidGCETestType(self):
    """Verify gce_tests has an expected value"""
    for build_name, config in self.site_config.iteritems():
      if config['gce_tests'] is None:
        continue
      for gce_test in config['gce_tests']:
        self.assertTrue(
            gce_test.test_type == constants.GCE_SUITE_TEST_TYPE,
            'Config %s: has unexpected gce test type value.' % build_name)
        self.assertTrue(
            gce_test.test_suite in constants.VALID_GCE_TEST_SUITES,
            'Config %s: has unexpected gce test suite value.' % build_name)

  def testImageTestMustHaveBaseImage(self):
    """Verify image_test build is only enabled with 'base' in images."""
    for build_name, config in self.site_config.iteritems():
      if config.get('image_test', False):
        self.assertTrue(
            'base' in config['images'],
            'Build %s runs image_test but does not have base image' %
            build_name)

  def testDisableHWQualWithoutTestImage(self):
    """Don't run steps that need a test image, without a test image."""
    for build_name, config in self.site_config.iteritems():
      if config.hwqual and config.upload_hw_test_artifacts:
        self.assertIn('test', config.images,
                      'Build %s must create a test image '
                      'to enable hwqual' % build_name)

  def testBuildType(self):
    """Verifies that all configs use valid build types."""
    for build_name, config in self.site_config.iteritems():
      # For builders that have explicit classes, this check doesn't make sense.
      if config['builder_class_name']:
        continue
      self.assertIn(config['build_type'], constants.VALID_BUILD_TYPES,
                    'Config %s: has unexpected build_type value.' % build_name)

  def testGCCGitHash(self):
    """Verifies that gcc_githash is not set without setting latest_toolchain."""
    for build_name, config in self.site_config.iteritems():
      if config['gcc_githash']:
        self.assertTrue(
            config['latest_toolchain'],
            'Config %s: has gcc_githash but not latest_toolchain.' % build_name)

  def testBuildToRun(self):
    """Verify we don't try to run tests without building them."""
    for build_name, config in self.site_config.iteritems():
      self.assertFalse(
          isinstance(config['useflags'], list) and
          '-build_tests' in config['useflags'] and config['vm_tests'],
          'Config %s: has vm_tests and use -build_tests.' % build_name)

  def testSyncToChromeSdk(self):
    """Verify none of the configs build chrome sdk but don't sync chrome."""
    for build_name, config in self.site_config.iteritems():
      if config['sync_chrome'] is not None and not config['sync_chrome']:
        self.assertFalse(
            config['chrome_sdk'],
            'Config %s: has chrome_sdk but not sync_chrome.' % build_name)

  def testOverrideVmTestsOnly(self):
    """VM/unit tests listed should also be supported."""
    for build_name, config in self.site_config.iteritems():
      if config.vm_tests_override is not None:
        for test in config.vm_tests:
          self.assertIn(
              test, config.vm_tests_override,
              'Config %s: has %s VM test, not in override (%s, %s).' % \
              (build_name, test, config.vm_tests, config.vm_tests_override))

  def testVmTestsOnlyOnVmTestBoards(self):
    """Verify that only VM capable boards run VM tests."""
    for _, config in self.site_config.iteritems():
      if config['vm_tests'] or config['vm_tests_override']:
        for board in config['boards']:
          self.assertIn(board, chromeos_config._vmtest_boards,
                        'Board %s not able to run VM tests.' % board)
      for child_config in config.child_configs:
        if child_config['vm_tests'] or child_config['vm_tests_override']:
          for board in config['boards']:
            self.assertIn(board, chromeos_config._vmtest_boards,
                          'Board %s not able to run VM tests.' % board)

  def testHWTestsArchivingHWTestArtifacts(self):
    """Make sure all configs upload artifacts that need them for hw testing."""
    for build_name, config in self.site_config.iteritems():
      if config.hw_tests or config.hw_tests_override:
        self.assertTrue(
            config.upload_hw_test_artifacts,
            "%s is trying to run hw tests without uploading payloads." %
            build_name)

  def testTryjobConfigsDontDefineOverrides(self):
    """Make sure that no tryjob safe configs define test overrides."""
    for build_name, config in self.site_config.iteritems():
      if config.display_label not in config_lib.TRYJOB_DISPLAY_LABEL:
        continue

      self.assertIsNone(
          config.vm_tests_override,
          'Config %s: is tryjob safe, but defines vm_tests_override.' % \
          build_name)

      self.assertIsNone(
          config.hw_tests_override,
          'Config %s: is tryjob safe, but defines hw_tests_override.' % \
          build_name)

  def testHWTestsReleaseBuilderRequirement(self):
    """Make sure all release configs run hw tests."""
    for build_name, config in self.site_config.iteritems():
      if (config.build_type == 'canary' and 'test' in config.images and
          config.upload_hw_test_artifacts and config.hwqual):
        self.assertTrue(
            config.hw_tests,
            "Release builder %s must run hw tests." % build_name)

  def testValidUnifiedMasterConfig(self):
    """Make sure any unified master configurations are valid."""
    for build_name, config in self.site_config.iteritems():
      error = 'Unified config for %s has invalid values' % build_name
      # Unified masters must be internal and must rev both overlays.
      if config['master']:
        self.assertTrue(
            config['internal'] and config['manifest_version'], error)
      elif not config['master'] and config['manifest_version']:
        # Unified slaves can rev either public or both depending on whether
        # they are internal or not.
        if not config['internal']:
          self.assertEqual(config['overlays'], constants.PUBLIC_OVERLAYS, error)
        elif config_lib.IsCQType(config['build_type']):
          self.assertEqual(config['overlays'], constants.BOTH_OVERLAYS, error)

  def testGetSlaves(self):
    """Make sure every master has a sane list of slaves"""
    for build_name, config in self.site_config.iteritems():
      if config.master:
        configs = self.site_config.GetSlavesForMaster(config)
        self.assertEqual(
            len(map(repr, configs)), len(set(map(repr, configs))),
            'Duplicate board in slaves of %s will cause upload prebuilts'
            ' failures' % build_name)

        # Our logic for calculating what slaves have completed their critical
        # stages will break if the master is considered a slave of itself,
        # because db.GetSlaveStages(...) doesn't include master stages.
        if config.build_type == constants.PALADIN_TYPE:
          self.assertEquals(
              config.boards, [],
              'Master paladin %s cannot have boards.' % build_name)
          self.assertNotIn(
              build_name, [x.name for x in configs],
              'Master paladin %s cannot be a slave of itself.' % build_name)

  def testMasterBuildTypes(self):
    """Test that all masters are of a whitelisted unique build type."""
    # Note: This is a whitelist of build type that are allowed to have a
    # master config. Do not add entries to this list without consulting with the
    # chrome-infra team.
    # TODO(akeshet): Remove this whitelist requirement once buildbot master
    # logic is fully chromite-driven.
    BUILD_TYPE_WHITELIST = (
        'canary',
        'pfq',
        'paladin',
        'toolchain',
        'chrome',
        'android',
    )

    found_types = set()
    for _, config in self.site_config.iteritems():
      if config.display_label in config_lib.TRYJOB_DISPLAY_LABEL:
        continue

      if config.master:
        self.assertTrue(config.build_type in BUILD_TYPE_WHITELIST,
                        'Config %s has build_type %s, which is not an allowed '
                        'type for a master build. Please consult with '
                        'chrome-infra before adding this config.' %
                        (config.name, config.build_type))
        # We have multiple masters for Android PFQ.
        self.assertTrue(config.build_type not in found_types or
                        config.build_type in ('pfq', 'android'),
                        'Duplicate master configs of build type %s' %
                        config.build_type)
        found_types.add(config.build_type)

  def _getSlaveConfigsForMaster(self, master_config_name):
    """Helper to fetch the configs for all slaves of a given master."""
    master_config = self.site_config[master_config_name]

    # Get a list of all active Paladins.
    return [self.site_config[n] for n in master_config.slave_configs]

  def testPreCQHasVMTests(self):
    """Make sure that at least one pre-cq builder enables VM tests."""
    pre_cq_configs = constants.PRE_CQ_DEFAULT_CONFIGS
    have_vm_tests = any([self.site_config[name].vm_tests
                         for name in pre_cq_configs])
    self.assertTrue(have_vm_tests, 'No Pre-CQ builder has VM tests enabled')

  def testPfqsHavePaladins(self):
    """Make sure that every active PFQ has an associated Paladin.

    This checks that every configured active PFQ on the external or internal
    main waterfall has an associated active Paladin config.
    """
    # Get a list of all active Paladins boards.
    paladin_boards = set()
    for slave_config in self._getSlaveConfigsForMaster('master-paladin'):
      paladin_boards.update(slave_config.boards)

    for pfq_master in (constants.PFQ_MASTER,
                       constants.NYC_ANDROID_PFQ_MASTER):
      pfq_configs = self._getSlaveConfigsForMaster(pfq_master)

      failures = set()
      for config in pfq_configs:
        self.assertEqual(len(config.boards), 1)
        if config.boards[0] not in paladin_boards:
          failures.add(config.name)

      if failures:
        self.fail("Some active PFQ configs don't have active Paladins: %s" % (
            ', '.join(sorted(failures))))

  def testGetSlavesOnTrybot(self):
    """Make sure every master has a sane list of slaves"""
    mock_options = mock.Mock()
    mock_options.remote_trybot = True
    for _, config in self.site_config.iteritems():
      if config['master']:
        configs = self.site_config.GetSlavesForMaster(config, mock_options)
        self.assertEqual([], configs)

  def testFactoryFirmwareValidity(self):
    """Ensures that firmware/factory branches have at least 1 valid name."""
    tracking_branch = git.GetChromiteTrackingBranch()
    for branch in ['firmware', 'factory']:
      if tracking_branch.startswith(branch):
        saw_config_for_branch = False
        for build_name in self.site_config:
          if build_name.endswith('-%s' % branch):
            self.assertFalse('release' in build_name,
                             'Factory|Firmware release builders should not '
                             'contain release in their name.')
            saw_config_for_branch = True

        self.assertTrue(
            saw_config_for_branch, 'No config found for %s branch. '
            'As this is the %s branch, all release configs that are being used '
            'must end in %s.' % (branch, tracking_branch, branch))

  def testNeverUseBackgroundBuildFlag(self):
    """build_packages_in_background is deprecated.

    Make sure nobody uses it, until we can remove it without breaking
    builds.
    """
    for build_name, config in self.site_config.iteritems():
      self.assertFalse(
          config.build_packages_in_background,
          'Deprecated flag build_packages_in_background used: %s' %
          build_name)

  def testNoNewBuildersOnlyGroups(self):
    """Grouped builders are deprecated.

    Ensure now new users are created. See crbug.com/691810.
    """
    for build_name, config in self.site_config.iteritems():
      # These group builders are whitelisted, for now.
      if not (build_name in ('test-ap-group',
                             'test-ap-group-tryjob',
                             'mixed-wificell-pre-cq') or
              build_name.endswith('release-afdo') or
              build_name.endswith('release-afdo-tryjob')):
        self.assertFalse(
            config.child_configs,
            'Unexpected group builder found: %s' % build_name)

  def testGroupBuildersHaveMaximumConfigs(self):
    """Verify that release group builders don't exceed a maximum board count.

    The count ignores variant boards (detected as "chrome_sdk" == False), since
    they don't add significant build time.
    """
    msg = 'Group config %s has %d child configurations (maximum is %d).'
    for build_name, config in self.site_config.iteritems():
      if build_name.endswith('-release-group'):
        child_count = 0
        for child in config.child_configs:
          if not child.chrome_sdk:
            continue
          child_count += 1
        self.assertLessEqual(child_count, constants.MAX_RELEASE_GROUP_BOARDS,
                             msg % (build_name, child_count,
                                    constants.MAX_RELEASE_GROUP_BOARDS))

  def testAFDOSameInChildConfigs(self):
    """Verify that 'afdo_use' is the same for all children in a group."""
    msg = ('Child config %s for %s should have same value for afdo_use '
           'as other children')
    for build_name, config in self.site_config.iteritems():
      if build_name.endswith('-group'):
        prev_value = None
        self.assertTrue(config.child_configs,
                        'Config %s should have child configs' % build_name)
        for child_config in config.child_configs:
          if prev_value is None:
            prev_value = child_config.afdo_use
          else:
            self.assertEqual(child_config.afdo_use, prev_value,
                             msg % (child_config.name, build_name))

  def testReleaseAFDOConfigs(self):
    """Verify that <board>-release-afdo config have generate and use children.

    These configs should have a 'generate' and a 'use' child config. Also,
    any 'generate' and 'use' configs should be children of a release-afdo
    config.
    """
    msg = 'Config %s should have %s as a parent'
    parent_suffix = config_lib.CONFIG_TYPE_RELEASE_AFDO
    generate_suffix = '%s-generate' % parent_suffix
    use_suffix = '%s-use' % parent_suffix
    for build_name, config in self.site_config.iteritems():
      if build_name.endswith(parent_suffix):
        self.assertEqual(
            len(config.child_configs), 2,
            'Config %s should have 2 child configs' % build_name)
        for child_config in config.child_configs:
          child_name = child_config.name
          self.assertTrue(child_name.endswith(generate_suffix) or
                          child_name.endswith(use_suffix),
                          'Config %s has wrong %s child' %
                          (build_name, child_config))
      if build_name.endswith(generate_suffix):
        parent_config_name = build_name.replace(generate_suffix,
                                                parent_suffix)
        self.assertTrue(parent_config_name in self.site_config,
                        msg % (build_name, parent_config_name))
      if build_name.endswith(use_suffix):
        parent_config_name = build_name.replace(use_suffix,
                                                parent_suffix)
        self.assertTrue(parent_config_name in self.site_config,
                        msg % (build_name, parent_config_name))

  def testNoGrandChildConfigs(self):
    """Verify that no child configs have a child config."""
    for build_name, config in self.site_config.iteritems():
      for child_config in config.child_configs:
        for grandchild_config in child_config.child_configs:
          self.fail('Config %s has grandchild %s' % (build_name,
                                                     grandchild_config.name))

  def testUseChromeLKGMImpliesInternal(self):
    """Currently use_chrome_lkgm refers only to internal manifests."""
    for build_name, config in self.site_config.iteritems():
      if config['use_chrome_lkgm']:
        self.assertTrue(
            config['internal'],
            'Chrome lkgm currently only works with an internal manifest: %s' % (
                build_name,))

  def _HasValidSuffix(self, config_name, config_types):
    """Given a config_name, see if it has a suffix in config_types.

    Args:
      config_name: Name of config to compare.
      config_types: A tuple/list of config suffixes.

    Returns:
      True, if the config has a suffix matching one of the types.
    """
    for config_type in config_types:
      if config_name.endswith('-' + config_type) or config_name == config_type:
        return True

    return False

  def testCantBeBothTypesOfLKGM(self):
    """Using lkgm and chrome_lkgm doesn't make sense."""
    for config in self.site_config.values():
      self.assertFalse(config['use_lkgm'] and config['use_chrome_lkgm'])

  def testNoDuplicateSlavePrebuilts(self):
    """Test that no two same-board paladin slaves upload prebuilts."""
    for cfg in self.site_config.values():
      if cfg['build_type'] == constants.PALADIN_TYPE and cfg['master']:
        slaves = self.site_config.GetSlavesForMaster(cfg)
        prebuilt_slaves = [s for s in slaves if s['prebuilts']]
        # Dictionary from board name to builder name that uploads prebuilt
        prebuilt_slave_boards = {}
        for slave in prebuilt_slaves:
          for board in slave['boards']:
            self.assertFalse(prebuilt_slave_boards.has_key(board),
                             'Configs %s and %s both upload prebuilts for '
                             'board %s.' % (prebuilt_slave_boards.get(board),
                                            slave['name'],
                                            board))
            prebuilt_slave_boards[board] = slave['name']

  def testNoDuplicateWaterfallNames(self):
    """Tests that no two configs specify same waterfall name."""
    waterfall_names = set()
    for config in self.site_config.values():
      wn = config['buildbot_waterfall_name']
      if wn is not None:
        self.assertNotIn(wn, waterfall_names,
                         'Duplicate waterfall name %s.' % wn)
        waterfall_names.add(wn)

  def testCantBeBothTypesOfAFDO(self):
    """Using afdo_generate and afdo_use together doesn't work."""
    for config in self.site_config.values():
      self.assertFalse(config['afdo_use'] and config['afdo_generate'])
      self.assertFalse(config['afdo_use'] and config['afdo_generate_min'])
      self.assertFalse(config['afdo_generate'] and config['afdo_generate_min'])

  def testValidPrebuilts(self):
    """Verify all builders have valid prebuilt values."""
    for build_name, config in self.site_config.iteritems():
      msg = 'Config %s: has unexpected prebuilts value.' % build_name
      valid_values = (False, constants.PRIVATE, constants.PUBLIC)
      self.assertTrue(config['prebuilts'] in valid_values, msg)

  def testInternalPrebuilts(self):
    for build_name, config in self.site_config.iteritems():
      if (config['internal'] and
          config['build_type'] not in [constants.CHROME_PFQ_TYPE,
                                       constants.PALADIN_TYPE]):
        msg = 'Config %s is internal but has public prebuilts.' % build_name
        self.assertNotEqual(config['prebuilts'], constants.PUBLIC, msg)

  def testValidHWTestPriority(self):
    """Verify that hw test priority is valid."""
    for build_name, config in self.site_config.iteritems():
      for test_config in config['hw_tests']:
        if isinstance(test_config.priority, (int, long)):
          self.assertTrue(0 <= test_config.priority <= 100)
        else:
          self.assertTrue(
              test_config.priority in constants.HWTEST_VALID_PRIORITIES,
              '%s has an invalid hwtest priority.' % build_name)

  def testAllBoardsExist(self):
    """Verifies that all config boards are in _all_boards."""
    boards_dict = self._GetBoardTypeToBoardsDict()
    for build_name, config in self.site_config.iteritems():
      self.assertIsNotNone(config['boards'],
                           'Config %s has boards = None' % build_name)
      for board in config['boards']:
        self.assertIn(board, boards_dict['all_boards'],
                      'Config %s has unknown board %s.' %
                      (build_name, board))

  def testPushImagePaygenDependancies(self):
    """Paygen requires PushImage."""
    for build_name, config in self.site_config.iteritems():

      # paygen can't complete without push_image, except for payloads
      # where --channel arguments meet the requirements.
      if config['paygen']:
        self.assertTrue(config['push_image'] or
                        config['build_type'] == constants.PAYLOADS_TYPE,
                        '%s has paygen without push_image' % build_name)

  def testPaygenTestDependancies(self):
    """paygen testing requires upload_hw_test_artifacts."""
    for build_name, config in self.site_config.iteritems():

      # This requirement doesn't apply to payloads builds. Payloads are
      # using artifacts from a previous build.
      if build_name.endswith('-payloads'):
        continue

      if config['paygen'] and not config['paygen_skip_testing']:
        self.assertTrue(config['upload_hw_test_artifacts'],
                        '%s is not upload_hw_test_artifacts, but also not'
                        ' paygen_skip_testing' % build_name)

  def testPayloadImageIsBuilt(self):
    for build_name, config in self.site_config.iteritems():
      if config.payload_image is not None:
        self.assertNotEqual('recovery', config.payload_image,
                            '%s wants to generate payloads from recovery '
                            'images, which is not allowed.' % build_name)
        self.assertIn(config.payload_image, config.images,
                      '%s builds payloads from %s, which is not in images '
                      'list %s' % (build_name, config.payload_image,
                                   config.images))

  def testBuildPackagesForRecoveryImage(self):
    """Tests that we build the packages required for recovery image."""
    for build_name, config in self.site_config.iteritems():
      if 'recovery' in config.images:
        if not config.packages:
          # No packages are specified. Defaults to build all packages.
          continue

        self.assertIn('chromeos-base/chromeos-initramfs',
                      config.packages,
                      '%s does not build chromeos-initramfs, which is required '
                      'for creating the recovery image' % build_name)

  def testBuildRecoveryImageFlags(self):
    """Ensure the right flags are disabled when building the recovery image."""
    incompatible_flags = ['paygen', 'signer_tests']
    for build_name, config in self.site_config.iteritems():
      for flag in incompatible_flags:
        if config[flag] and config.build_type != constants.PAYLOADS_TYPE:
          self.assertIn('recovery', config.images,
                        '%s does not build the recovery image, which is '
                        'incompatible with %s=True' % (build_name, flag))

  def testBuildBaseImageForRecoveryImage(self):
    """Tests that we build the packages required for recovery image."""
    for build_name, config in self.site_config.iteritems():
      if 'recovery' in config.images:
        self.assertIn('base', config.images,
                      '%s does not build the base image, which is required for '
                      'building the recovery image' % build_name)

  def testChildConfigsNotImportantInReleaseGroup(self):
    """Verify that configs in an important group are not important."""
    msg = ('Child config %s for %s should not be important because %s is '
           'already important')
    for build_name, config in self.site_config.iteritems():
      if build_name.endswith('-release-group') and config['important']:
        for child_config in config.child_configs:
          self.assertFalse(child_config.important,
                           msg % (child_config.name, build_name, build_name))

  def testExternalConfigsDoNotUseInternalFeatures(self):
    """External configs should not use chrome_internal, or official.xml."""
    msg = ('%s is not internal, so should not use chrome_internal, or an '
           'internal manifest')
    for build_name, config in self.site_config.iteritems():
      if not config['internal']:
        self.assertFalse('chrome_internal' in config['useflags'],
                         msg % build_name)
        self.assertNotEqual(config.get('manifest'),
                            constants.OFFICIAL_MANIFEST,
                            msg % build_name)

  def testNoShadowedUseflags(self):
    """Configs should not have both useflags x and -x."""
    msg = ('%s contains useflag %s and -%s.')
    for build_name, config in self.site_config.iteritems():
      useflag_set = set(config['useflags'])
      for flag in useflag_set:
        if not flag.startswith('-'):
          self.assertFalse('-' + flag in useflag_set,
                           msg % (build_name, flag, flag))

  def testHealthCheckEmails(self):
    """Configs should only have valid email addresses or aliases"""
    msg = ('%s contains an invalid tree alias or email address: %s')
    for build_name, config in self.site_config.iteritems():
      health_alert_recipients = config['health_alert_recipients']
      for recipient in health_alert_recipients:
        self.assertTrue(re.match(r'[^@]+@[^@]+\.[^@]+', recipient) or
                        recipient in constants.SHERIFF_TYPE_TO_URL.keys(),
                        msg % (build_name, recipient))

  def testCheckBuilderClass(self):
    """Verify builder_class_name is a valid value."""
    for build_name, config in self.site_config.iteritems():
      builder_class_name = config['builder_class_name']
      if builder_class_name is None:
        continue

      cls = builders.GetBuilderClass(builder_class_name)
      self.assertTrue(issubclass(cls, generic_builders.Builder),
                      msg=('config %s has a broken builder_class_name' %
                           build_name))

  def testDistinctBoardSets(self):
    """Verify that distinct board sets are distinct."""
    boards_dict = self._GetBoardTypeToBoardsDict()
    # Every board should be in exactly one of the distinct board sets.
    for board in boards_dict['all_boards']:
      found = False
      for s in boards_dict['distinct_board_sets']:
        if board in s:
          if found:
            assert False, '%s in multiple board sets.' % board
          else:
            found = True
      if not found:
        assert False, '%s in no board sets' % board
    for s in boards_dict['distinct_board_sets']:
      for board in s - boards_dict['all_boards']:
        assert False, ('%s in distinct_board_sets but not in all_boards' %
                       board)

  def testCanaryBuildTimeouts(self):
    """Verify we get the expected timeout values."""
    msg = ("%s doesn't have expected timout: (%s != %s)")
    for build_name, config in self.site_config.iteritems():
      if config.build_type != constants.CANARY_TYPE:
        continue
      if self.isReleaseBranch():
        expected = 12 * 60 * 60
      else:
        expected = (7 * 60 + 50) * 60

      self.assertEqual(
          config.build_timeout, expected,
          msg % (build_name, config.build_timeout, expected))

  def testBuildTimeouts(self):
    """Verify that timeout values are sane."""
    for build_name, config in self.site_config.iteritems():
      # Chrome infra has a hard limit of 24h.
      self.assertLessEqual(
          config.build_timeout, 24 * 60 * 60,
          '%s timeout %s is greater than 24h'
          % (build_name, config.build_timeout))

  def testWaterfallManualConfigIsValid(self):
    """Verify the correctness of the manual waterfall configuration."""

    # TODO: Start setting this value for the test, instead of just reading.
    if self.isReleaseBranch():
      return

    all_build_names = set(self.site_config.iterkeys())
    redundant = set()
    seen = set()
    for wfall, names in chromeos_config._waterfall_config_map.iteritems():
      for build_name in names:
        # Every build in the configuration map must be valid.
        self.assertTrue(build_name in all_build_names,
                        "Invalid build name in manual waterfall config: %s" % (
                            build_name,))
        # No build should appear in multiple waterfalls.
        self.assertFalse(build_name in seen,
                         "Duplicate manual config for board: %s" % (
                             build_name,))
        seen.add(build_name)

        # The manual configuration must be applied and override any default
        # configuration.
        config = self.site_config[build_name]
        self.assertEqual(config['active_waterfall'], wfall,
                         "Manual waterfall membership is not in the "
                         "configuration for: %s" % (build_name,))


        default_waterfall = chromeos_config.GetDefaultWaterfall(config, False)
        if config['active_waterfall'] == default_waterfall:
          redundant.add(build_name)

    # No configurations should be redundant with defaults.
    self.assertFalse(redundant,
                     "Manual waterfall membership in "
                     "`_waterfall_config_map` is redundant for these "
                     "configs: %s" % (sorted(redundant),))

  def testNoDuplicateCanaryBuildersOnWaterfall(self):
    seen = {}
    for config in self.site_config.itervalues():
      wfall = config['active_waterfall']
      btype = config['build_type']
      if not (wfall and config_lib.IsCanaryType(btype)):
        continue

      waterfall_seen = seen.setdefault(wfall, set())
      stack = [config]
      while stack:
        current_config = stack.pop()
        self.assertNotIn(current_config['name'], waterfall_seen,
                         "Multiple builders for '%s' on '%s' waterfall" % (
                             current_config['name'], wfall))
        waterfall_seen.add(current_config['name'])
        stack += current_config['child_configs']

  def testBinhostTest(self):
    """Builders with the binhost_test setting shouldn't have boards."""
    for config in self.site_config.values():
      if config.binhost_test:
        self.assertEqual(config.boards, [])


class TemplateTest(ChromeosConfigTestBase):
  """Tests for templates."""

  def testConfigNamesMatchTemplate(self):
    """Test that all configs have names that match their templates."""
    for name, config in self.site_config.iteritems():
      # Tryjob configs should be tested based on what they are mirrored from.
      if name.endswith('-tryjob'):
        name = name[:-len('-tryjob')]

      template = config._template
      if template:
        # We mix '-' and '_' in various name spaces.
        name = name.replace('_', '-')
        template = template.replace('_', '-')
        child_configs = config.child_configs
        if not child_configs:
          msg = '%s should end with %s to match its template'
          self.assertTrue(name.endswith(template), msg % (name, template))
        else:
          msg = 'Child config of %s has name that does not match its template'
          self.assertTrue(child_configs[0].name.endswith(template),
                          msg % name)

      for other in self.site_config.GetTemplates():
        if name.endswith(other) and other != template:
          if template:
            msg = '%s has more specific template: %s' % (name, other)
            self.assertGreater(len(template), len(other), msg)
          else:
            msg = '%s should have %s as template' % (name, other)
            self.assertFalse(name, msg)


class BoardConfigsTest(ChromeosConfigTestBase):
  """Tests for the per-board templates."""
  def setUp(self):
    ge_build_config = config_lib.LoadGEBuildConfigFromFile()
    boards_dict = chromeos_config.GetBoardTypeToBoardsDict(ge_build_config)

    self.external_board_configs = chromeos_config.CreateBoardConfigs(
        self.site_config, boards_dict, ge_build_config)

    self.internal_board_configs = chromeos_config.CreateInternalBoardConfigs(
        self.site_config, boards_dict, ge_build_config)

  def testBoardConfigsSuperset(self):
    """Ensure all external boards are listed as internal, also."""
    for board in self.external_board_configs.keys():
      self.assertIn(board, self.internal_board_configs)

  def verifyNoTests(self, board_configs_iter):
    """Defining tests in board specific templates doesn't work as expected."""
    for board, template in board_configs_iter:
      self.assertFalse(
          'vm_tests' in template and template.vm_tests,
          'Per-board template for %s defining vm_tests' % board)
      self.assertFalse(
          'vm_tests_override' in template and template.vm_tests_override,
          'Per-board template for %s defining vm_tests_override' % board)
      self.assertFalse(
          'gce_tests' in template and template.gce_tests,
          'Per-board template for %s defining gce_tests' % board)
      self.assertFalse(
          'hw_tests' in template and template.hw_tests,
          'Per-board template for %s defining hw_tests' % board)
      self.assertFalse(
          'hw_tests_override' in template and template.hw_tests_override,
          'Per-board template for %s defining hw_tests_override' % board)

  def testExternalsDontDefineTests(self):
    """Verify no external boards define tests at the board level."""
    self.verifyNoTests(self.external_board_configs.items())

  def testInternalsDontDefineTests(self):
    """Verify no internal boards define tests at the board level."""
    self.verifyNoTests(self.internal_board_configs.items())

  def testUpdateBoardConfigs(self):
    """Test UpdateBoardConfigs."""
    pre_test = copy.deepcopy(self.internal_board_configs)
    update_boards = pre_test.keys()[2:5]

    result = chromeos_config.UpdateBoardConfigs(
        self.internal_board_configs, update_boards,
        test_specific_flag=True,
    )

    # The source wasn't modified.
    self.assertEqual(self.internal_board_configs, pre_test)

    # The result as the same list of boards.
    self.assertItemsEqual(result.keys(), pre_test.keys())

    # And only appropriate values were updated.
    for b in pre_test:
      if b in update_boards:
        # Has new key.
        self.assertTrue(result[b].test_specific_flag, 'Failed in %s' % b)
      else:
        # Was not updated.
        self.assertEqual(result[b], pre_test[b], 'Failed in %s' % b)


class SiteInterfaceTest(ChromeosConfigTestBase):
  """Test enforcing site parameters for a chromeos SiteConfig."""

  def testAssertSiteParameters(self):
    """Test that a chromeos SiteConfig contains the necessary parameters."""
    # Check that our config contains site-independent parameters.
    self.assertTrue(
        config_lib_unittest.AssertSiteIndependentParameters(self.site_config))

    # Enumerate the necessary chromeos site parameter keys.
    chromeos_params = config_lib.DefaultSiteParameters().keys()

    # Check that our config contains all chromeos specific site parameters.
    site_params = self.site_config.params
    self.assertTrue(all([x in site_params for x in chromeos_params]))
