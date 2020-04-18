# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for gclient.py."""

from __future__ import print_function

import os

from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import gclient
from chromite.lib import osutils


class TestGclientWriteConfigFile(cros_test_lib.RunCommandTempDirTestCase):
  """Unit tests for gclient.WriteConfigFile."""

  _TEST_CWD = '/work/chrome'

  def _AssertGclientConfigSpec(self, expected_spec, use_cache=True):
    if cros_build_lib.HostIsCIBuilder() and use_cache:
      if cros_build_lib.IsInsideChroot():
        expected_spec += "cache_dir = '/tmp/b/git-cache'\n"
      else:
        expected_spec += "cache_dir = '/b/git-cache'\n"
    self.rc.assertCommandContains(('gclient', 'config', '--spec',
                                   expected_spec),
                                  cwd=self._TEST_CWD)

  def _CreateGclientTemplate(self, template_content):
    template_path = os.path.join(self.tempdir, 'gclient_template')
    osutils.WriteFile(template_path, template_content)
    return template_path

  def testChromiumSpec(self):
    """Test WriteConfigFile with chromium checkout and no revision."""
    gclient.WriteConfigFile('gclient', self._TEST_CWD, False, None)
    self._AssertGclientConfigSpec("""solutions = [{'custom_deps': {},
  'custom_vars': {},
  'deps_file': '.DEPS.git',
  'managed': True,
  'name': 'src',
  'url': 'https://chromium.googlesource.com/chromium/src.git'}]
target_os = ['chromeos']
""")

  def testChromiumSpecNotUseCache(self):
    """Test WriteConfigFile with chromium checkout and no revision."""
    gclient.WriteConfigFile('gclient', self._TEST_CWD, False, None,
                            use_cache=False)
    self._AssertGclientConfigSpec("""solutions = [{'custom_deps': {},
  'custom_vars': {},
  'deps_file': '.DEPS.git',
  'managed': True,
  'name': 'src',
  'url': 'https://chromium.googlesource.com/chromium/src.git'}]
target_os = ['chromeos']
""", use_cache=False)

  def testChromeSpec(self):
    """Test WriteConfigFile with chrome checkout and no revision."""
    gclient.WriteConfigFile('gclient', self._TEST_CWD, True, None)
    self._AssertGclientConfigSpec("""solutions = [{'custom_deps': {},
  'custom_vars': {},
  'deps_file': '.DEPS.git',
  'managed': True,
  'name': 'src',
  'url': 'https://chromium.googlesource.com/chromium/src.git'},
 {'custom_deps': {},
  'custom_vars': {},
  'deps_file': '.DEPS.git',
  'managed': True,
  'name': 'src-internal',
  'url': 'https://chrome-internal.googlesource.com/chrome/src-internal.git'}]
target_os = ['chromeos']
""")

  def testChromiumSpecWithGitHash(self):
    """Test WriteConfigFile with chromium checkout at a given git revision."""
    gclient.WriteConfigFile('gclient', self._TEST_CWD, False,
                            '7becbe4afb42b3301d42149d7d1cade017f150ff')
    self._AssertGclientConfigSpec("""solutions = [{'custom_deps': {},
  'custom_vars': {},
  'deps_file': '.DEPS.git',
  'managed': True,
  'name': 'src',
  'url': 'https://chromium.googlesource.com/chromium/src.git@7becbe4afb42b3301d42149d7d1cade017f150ff'}]
target_os = ['chromeos']
""")

  def testChromeSpecWithGitHash(self):
    """Test WriteConfigFile with chrome checkout at a given git revision."""
    gclient.WriteConfigFile('gclient', self._TEST_CWD, True,
                            '7becbe4afb42b3301d42149d7d1cade017f150ff')
    self._AssertGclientConfigSpec("""solutions = [{'custom_deps': {},
  'custom_vars': {},
  'deps_file': '.DEPS.git',
  'managed': True,
  'name': 'src',
  'url': 'https://chromium.googlesource.com/chromium/src.git@7becbe4afb42b3301d42149d7d1cade017f150ff'},
 {'custom_deps': {},
  'custom_vars': {},
  'deps_file': '.DEPS.git',
  'managed': True,
  'name': 'src-internal',
  'url': 'https://chrome-internal.googlesource.com/chrome/src-internal.git'}]
target_os = ['chromeos']
""")

  def testChromiumSpecWithGitHead(self):
    """Test WriteConfigFile with chromium checkout at a given git revision."""
    gclient.WriteConfigFile('gclient', self._TEST_CWD, False, 'HEAD')
    self._AssertGclientConfigSpec("""solutions = [{'custom_deps': {},
  'custom_vars': {},
  'deps_file': '.DEPS.git',
  'managed': True,
  'name': 'src',
  'url': 'https://chromium.googlesource.com/chromium/src.git@HEAD'}]
target_os = ['chromeos']
""")

  def testChromeSpecWithGitHead(self):
    """Test WriteConfigFile with chrome checkout at a given git revision."""
    gclient.WriteConfigFile('gclient', self._TEST_CWD, True, 'HEAD')
    self._AssertGclientConfigSpec("""solutions = [{'custom_deps': {},
  'custom_vars': {},
  'deps_file': '.DEPS.git',
  'managed': True,
  'name': 'src',
  'url': 'https://chromium.googlesource.com/chromium/src.git@HEAD'},
 {'custom_deps': {},
  'custom_vars': {},
  'deps_file': '.DEPS.git',
  'managed': True,
  'name': 'src-internal',
  'url': 'https://chrome-internal.googlesource.com/chrome/src-internal.git'}]
target_os = ['chromeos']
""")

  def testChromeSpecWithGitHashNoManaged(self):
    """Like testChromeSpecWithGitHash() but with "managed" sets to False."""
    gclient.WriteConfigFile('gclient', self._TEST_CWD, True,
                            '7becbe4afb42b3301d42149d7d1cade017f150ff',
                            managed=False)
    self._AssertGclientConfigSpec("""solutions = [{'custom_deps': {},
  'custom_vars': {},
  'deps_file': '.DEPS.git',
  'managed': False,
  'name': 'src',
  'url': 'https://chromium.googlesource.com/chromium/src.git@7becbe4afb42b3301d42149d7d1cade017f150ff'},
 {'custom_deps': {},
  'custom_vars': {},
  'deps_file': '.DEPS.git',
  'managed': False,
  'name': 'src-internal',
  'url': 'https://chrome-internal.googlesource.com/chrome/src-internal.git'}]
target_os = ['chromeos']
""")

  def testChromeSpecWithReleaseTag(self):
    """Test WriteConfigFile with chrome checkout at a given release tag."""
    gclient.WriteConfigFile('gclient', self._TEST_CWD, True, '45.0.2431.1')
    self._AssertGclientConfigSpec("""solutions = [{'custom_deps': {},
  'custom_vars': {},
  'deps_file': 'releases/45.0.2431.1/DEPS',
  'managed': True,
  'name': 'CHROME_DEPS',
  'url': 'https://chrome-internal.googlesource.com/chrome/tools/buildspec.git'}]
target_os = ['chromeos']
""")

  def testChromiumSpecWithReleaseTag(self):
    """Test WriteConfigFile with chromium checkout at a given release tag."""
    gclient.WriteConfigFile('gclient', self._TEST_CWD, False, '41.0.2270.0')
    self._AssertGclientConfigSpec("""solutions = [{'custom_deps': {},
  'custom_vars': {},
  'deps_file': '.DEPS.git',
  'managed': True,
  'name': 'src',
  'url': 'https://chromium.googlesource.com/chromium/src.git@refs/tags/41.0.2270.0'}]
target_os = ['chromeos']
""")

  def testChromeSpecWithReleaseTagDepsGit(self):
    """Test WriteConfigFile with chrome checkout at a given release tag."""
    gclient.WriteConfigFile('gclient', self._TEST_CWD, True, '41.0.2270.0')
    self._AssertGclientConfigSpec("""solutions = [{'custom_deps': {},
  'custom_vars': {},
  'deps_file': 'releases/41.0.2270.0/.DEPS.git',
  'managed': True,
  'name': 'CHROME_DEPS',
  'url': 'https://chrome-internal.googlesource.com/chrome/tools/buildspec.git'}]
target_os = ['chromeos']
""")

  def testChromeSpecDepsResolution(self):
    """Test BuildspecUsesDepsGit at release thresholds."""
    for rev, uses_deps_git in (
        ('41.0.2270.0', True),
        ('45.0.2430.3', True),
        ('45.0.2431.0', False),
        ('44.0.2403.48', True),
        ('44.0.2404.0', False),
        ('43.0.2357.125', True),
        ('43.0.2357.126', False)):
      self.assertEqual(gclient.BuildspecUsesDepsGit(rev), uses_deps_git)

  def testChromeSpecWithGclientTemplate(self):
    """Test WriteConfigFile with chrome checkout with a gclient template."""
    template_path = self._CreateGclientTemplate("""solutions = [
  {
    'name': 'src',
    'custom_deps': {'dep1': '1'},
    'custom_vars': {'var1': 'test1', 'var2': 'test2'},
  },
  { 'name': 'no-vars', 'custom_deps': {'dep2': '2', 'dep3': '3'} },
  { 'name': 'no-deps', 'custom_vars': {'var3': 'a', 'var4': 'b'} }
]""")
    gclient.WriteConfigFile('gclient', self._TEST_CWD, True,
                            '7becbe4afb42b3301d42149d7d1cade017f150ff',
                            template=template_path)
    self._AssertGclientConfigSpec("""solutions = [{'custom_deps': {'dep1': '1'},
  'custom_vars': {'var1': 'test1', 'var2': 'test2'},
  'deps_file': '.DEPS.git',
  'managed': True,
  'name': 'src',
  'url': 'https://chromium.googlesource.com/chromium/src.git@7becbe4afb42b3301d42149d7d1cade017f150ff'},
 {'custom_deps': {'dep2': '2', 'dep3': '3'}, 'name': 'no-vars'},
 {'custom_vars': {'var3': 'a', 'var4': 'b'}, 'name': 'no-deps'},
 {'custom_deps': {},
  'custom_vars': {},
  'deps_file': '.DEPS.git',
  'managed': True,
  'name': 'src-internal',
  'url': 'https://chrome-internal.googlesource.com/chrome/src-internal.git'}]
target_os = ['chromeos']
""")


class GclientWrappersTest(cros_test_lib.RunCommandTempDirTestCase):
  """Tests for small gclient wrappers"""

  def setUp(self):
    self.fake_gclient = os.path.join(self.tempdir, 'gclient')
    self.cwd = self.tempdir

  def testRevert(self):
    gclient.Revert(self.fake_gclient, self.cwd)
    self.assertCommandCalled([self.fake_gclient, 'revert', '--nohooks'],
                             cwd=self.cwd)

  def testSync(self):
    """Test gclient.Sync() without optional arguments."""
    gclient.Sync(self.fake_gclient, self.cwd)
    self.assertCommandCalled(
        [self.fake_gclient, 'sync', '--with_branch_heads', '--with_tags',
         '--nohooks', '--verbose'], cwd=self.cwd)

  def testSyncWithOptions(self):
    """Test gclient.Sync() with optional arguments."""
    gclient.Sync(self.fake_gclient, self.cwd, reset=True)
    self.assertCommandCalled(
        [self.fake_gclient, 'sync', '--with_branch_heads', '--with_tags',
         '--reset', '--force', '--delete_unversioned_trees',
         '--nohooks', '--verbose'], cwd=self.cwd)

    gclient.Sync(self.fake_gclient, self.cwd, nohooks=False, verbose=False)
    self.assertCommandCalled(
        [self.fake_gclient, 'sync', '--with_branch_heads', '--with_tags'],
        cwd=self.cwd)

    gclient.Sync(self.fake_gclient, self.cwd, nohooks=False, verbose=False,
                 ignore_locks=True)
    self.assertCommandCalled(
        [self.fake_gclient, 'sync', '--with_branch_heads', '--with_tags',
         '--ignore_locks'],
        cwd=self.cwd)

  def testSyncWithRunArgs(self):
    """Test gclient.Sync() with run_args.

    run_args is an optional argument for RunCommand kwargs.
    """
    gclient.Sync(self.fake_gclient, self.cwd, run_args={'log_output': True})
    self.assertCommandCalled(
        [self.fake_gclient, 'sync', '--with_branch_heads', '--with_tags',
         '--nohooks', '--verbose'],
        cwd=self.cwd, log_output=True)
