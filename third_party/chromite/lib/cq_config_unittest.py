# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for cq_config."""

from __future__ import print_function

import ConfigParser
import mock
import os
import osutils

from chromite.lib import constants
from chromite.lib import cq_config
from chromite.lib import cros_test_lib
from chromite.lib import git
from chromite.lib import patch as cros_patch
from chromite.lib import patch_unittest

# pylint: disable=protected-access

class ConfigFileTest(cros_test_lib.MockTestCase):
  """Tests for functions that read config information for a patch."""

  def _GetPatch(self, affected_files):
    p = patch_unittest.MockPatchFactory().MockPatch()
    mock_diff_status = {path: 'M' for path in affected_files}
    self.PatchObject(cros_patch.GerritPatch, 'GetDiffStatus',
                     return_value=mock_diff_status)
    return p

  def testAffectedSubdir(self):
    """Test AffectedSubdir."""
    p = self._GetPatch(['a', 'b', 'c'])
    self.assertEqual(
        cq_config.CQConfigParser._GetCommonAffectedSubdir(p, '/a/b'), '/a/b')

    p = self._GetPatch(['a/a', 'a/b', 'a/c'])
    self.assertEqual(
        cq_config.CQConfigParser._GetCommonAffectedSubdir(p, '/a/b'), '/a/b/a')

    p = self._GetPatch(['a/a', 'a/b', 'a/c'])
    self.assertEqual(
        cq_config.CQConfigParser._GetCommonAffectedSubdir(p, '/a/b'), '/a/b/a')

  def testGetCommonConfigFileForChange(self):
    """Test GetCommonConfigFileForChange."""
    self.PatchObject(git.ManifestCheckout, 'Cached')
    p = self._GetPatch(['a/a', 'a/b', 'a/c'])
    mock_checkout = mock.Mock()
    self.PatchObject(cq_config.CQConfigParser, 'GetCheckout',
                     return_value=mock_checkout)

    self.PatchObject(os.path, 'isfile', return_value=True)
    mock_checkout.GetPath.return_value = '/a/b'
    self.assertEqual(
        cq_config.CQConfigParser.GetCommonConfigFileForChange('/', p),
        '/a/b/a/COMMIT-QUEUE.ini')
    mock_checkout.GetPath.return_value = '/a/b/'
    self.assertEqual(
        cq_config.CQConfigParser.GetCommonConfigFileForChange('/', p),
        '/a/b/a/COMMIT-QUEUE.ini')

    self.PatchObject(os.path, 'isfile', return_value=False)
    mock_checkout.GetPath.return_value = '/a/b'
    self.assertEqual(
        cq_config.CQConfigParser.GetCommonConfigFileForChange('/', p),
        '/a/b/COMMIT-QUEUE.ini')
    mock_checkout.GetPath.return_value = '/a/b/'
    self.assertEqual(
        cq_config.CQConfigParser.GetCommonConfigFileForChange('/', p),
        '/a/b/COMMIT-QUEUE.ini')

class CQConfigParserTest(cros_test_lib.MockTestCase):
  """Tests for CQConfigParser."""

  def setUp(self):
    self._patch_factory = patch_unittest.MockPatchFactory()
    self.build_root = '/build_root/'
    self.change = self._patch_factory.MockPatch()
    self.checkout = mock.Mock()
    self.common_config_file = '/build_root/repo/COMMIT-QUEUE.ini'

  def CreateCQConfigParser(self, build_root=None, change=None, checkout=None,
                           common_config_file=None, forgiving=True):
    if build_root is None:
      build_root = self.build_root
    if change is None:
      change = self.change
    if checkout is None:
      checkout = self.checkout
    if common_config_file is None:
      common_config_file = self.common_config_file

    self.PatchObject(cq_config.CQConfigParser, 'GetCheckout',
                     return_value=checkout)
    self.PatchObject(cq_config.CQConfigParser, 'GetCommonConfigFileForChange',
                     return_value=common_config_file)
    return cq_config.CQConfigParser(build_root, change, forgiving=forgiving)

  def testGetOption(self):
    """Test GetOption."""
    with osutils.TempDir(set_global=True) as tempdir:
      foo_path = os.path.join(tempdir, 'foo.ini')
      osutils.WriteFile(foo_path, '[GENERAL]\npre-cq-configs: binhost-pre-cq\n')
      bar_path = os.path.join(tempdir, 'bar', 'bar.ini')
      osutils.WriteFile(bar_path, '[GENERAL]\npre-cq-configs: lumpy-pre-cq\n',
                        makedirs=True)
      parser = self.CreateCQConfigParser(common_config_file=foo_path)

      self.assertEqual(parser.GetOption(constants.CQ_CONFIG_SECTION_GENERAL,
                                        constants.CQ_CONFIG_PRE_CQ_CONFIGS),
                       'binhost-pre-cq')
      self.assertEqual(parser.GetOption(constants.CQ_CONFIG_SECTION_GENERAL,
                                        constants.CQ_CONFIG_PRE_CQ_CONFIGS,
                                        config_path=bar_path),
                       'lumpy-pre-cq')

  def testGetOptionForBadConfigFileWithTrueForgiven(self):
    """Test whether the return is None when handle a malformat config file."""
    with osutils.TempDir(set_global=True) as tempdir:
      path = os.path.join(tempdir, 'COMMIT-QUEUE.ini')
      osutils.WriteFile(path, 'foo\n')
      parser = self.CreateCQConfigParser(common_config_file=path)
      result = parser.GetOption('a', 'b')

      self.assertEqual(None, result)

  def testGetOptionForBadConfigFileWithFalseForgiving(self):
    """Test whether exception is raised when handle a malformat config file."""
    with osutils.TempDir(set_global=True) as tempdir:
      path = os.path.join(tempdir, 'COMMIT-QUEUE.ini')
      osutils.WriteFile(path, 'foo\n')
      parser = self.CreateCQConfigParser(common_config_file=path,
                                         forgiving=False)

      with self.assertRaises(cq_config.MalformedCQConfigException):
        parser.GetOption('a', 'b')

  def testGetPreCQConfigs(self):
    """Test GetPreCQConfigs."""
    with osutils.TempDir(set_global=True) as tempdir:
      path = os.path.join(tempdir, 'foo.ini')
      osutils.WriteFile(path,
                        '[GENERAL]\npre-cq-configs: default binhost-pre-cq\n')
      parser = self.CreateCQConfigParser(common_config_file=path)
      pre_cq_configs = parser.GetPreCQConfigs()

      self.assertItemsEqual(pre_cq_configs, ['default', 'binhost-pre-cq'])

  def testGetStagesToIgnore(self):
    """Test if we can get the ignored stages from a good config file."""
    with osutils.TempDir(set_global=True) as tempdir:
      path = os.path.join(tempdir, 'foo.ini')
      osutils.WriteFile(path, '[GENERAL]\nignored-stages: bar baz\n')
      parser = self.CreateCQConfigParser(common_config_file=path)
      ignored = parser.GetStagesToIgnore()

      self.assertEqual(ignored, ['bar', 'baz'])

  def testGetSubsystems(self):
    """Test if we can get the subsystem label from a good config file."""
    with osutils.TempDir(set_global=True) as tempdir:
      path = os.path.join(tempdir, 'foo.ini')
      osutils.WriteFile(path, '[GENERAL]\nsubsystem: power light\n')
      parser = self.CreateCQConfigParser(common_config_file=path)
      subsystems = parser.GetSubsystems()

      self.assertItemsEqual(subsystems, ['power', 'light'])

  def testGetConfigFlagReturnsTrue(self):
    """Test GetConfigFlag which returns True."""
    with osutils.TempDir(set_global=True) as tempdir:
      path = os.path.join(tempdir, 'foo.ini')
      osutils.WriteFile(path, '[a]\nb: yes\n')
      parser = self.CreateCQConfigParser(common_config_file=path)
      flag = parser.GetConfigFlag('a', 'b')

      self.assertTrue(flag)

  def testGetConfigFlagReturnsFalse(self):
    """Test GetConfigFlag which returns False."""
    with osutils.TempDir(set_global=True) as tempdir:
      path = os.path.join(tempdir, 'foo.ini')
      osutils.WriteFile(path, '[a]\nc: yes\n')
      parser = self.CreateCQConfigParser(common_config_file=path)
      flag = parser.GetConfigFlag('a', 'b')
      self.assertFalse(flag)

  def testGetUnionPreCQSubConfigsFlag(self):
    """Test ShouldUnionPreCQFromSubConfigs."""
    with osutils.TempDir(set_global=True) as tempdir:
      path = os.path.join(tempdir, 'foo.ini')
      osutils.WriteFile(path, '[GENERAL]\nunion-pre-cq-sub-configs: yes\n')
      parser = self.CreateCQConfigParser(common_config_file=path)
      self.assertTrue(parser.GetUnionPreCQSubConfigsFlag())

  def _CreateOverlayPaths(self, root_dir, sub_dir,
                          config_str,
                          file_rel_path=os.path.join('test', 'test.py')):
    sub_dir = os.path.join(root_dir, sub_dir)
    sub_init = os.path.join(sub_dir, 'COMMIT-QUEUE.ini')
    osutils.WriteFile(sub_init, config_str, makedirs=True)
    changed_file = os.path.join(sub_dir, file_rel_path)
    osutils.WriteFile(changed_file, '#test#', makedirs=True)
    return os.path.relpath(changed_file, root_dir)

  def testGetUnionedPreCQConfigs(self):
    """Test GetUnionedPreCQConfigs."""
    mock_checkout = mock.Mock()
    with osutils.TempDir(set_global=True) as tempdir:
      root_dir = os.path.join(tempdir, 'overlays')
      mock_checkout.GetPath.return_value = root_dir
      root_ini = os.path.join(root_dir, 'COMMIT-QUEUE.ini')
      osutils.WriteFile(
          root_ini,
          '[GENERAL]\npre-cq-configs: default\n',
          makedirs=True)
      f_1 = self._CreateOverlayPaths(
          root_dir, 'overlay-lumpy',
          '[GENERAL]\npre-cq-configs: default lumpy-pre-cq\n')
      f_2 = self._CreateOverlayPaths(
          root_dir, 'overlay-stumpy',
          '[GENERAL]\npre-cq-configs: stumpy-pre-cq\n')
      f_3 = self._CreateOverlayPaths(
          root_dir, 'overlay-lakitu',
          '[GENERAL]\npre-cq-configs: lakitu-pre-cq\n')
      f_4 = self._CreateOverlayPaths(
          root_dir, 'overlay-lakitu',
          '[GENERAL]\npre-cq-configs: lakitu-pre-cq\n',
          file_rel_path=os.path.join('test', 'test-copy.py'))
      diff_dict = {f: 'M' for f in (f_1, f_2, f_3, f_4)}
      change = self._patch_factory.MockPatch()
      self.PatchObject(cros_patch.GerritPatch, 'GetDiffStatus',
                       return_value=diff_dict)

      parser = self.CreateCQConfigParser(
          change=change, common_config_file=root_ini, checkout=mock_checkout)
      pre_cq_configs = parser.GetUnionedPreCQConfigs()

      self.assertItemsEqual(pre_cq_configs,
                            {'default', 'lumpy-pre-cq', 'lakitu-pre-cq',
                             'stumpy-pre-cq'})

  def testCanSubmitChangeInPreCqMissingSubConfigsOption(self):
    """Test CanSubmitChangeInPreCq when sub-configs option is missing."""
    mock_checkout = mock.Mock()
    with osutils.TempDir(set_global=True) as tempdir:
      root_dir = os.path.join(tempdir, 'overlays')
      mock_checkout.GetPath.return_value = root_dir
      root_ini = os.path.join(root_dir, 'COMMIT-QUEUE.ini')
      osutils.WriteFile(
          root_ini,
          '[GENERAL]\nsubmit-in-pre-cq: no\n',
          makedirs=True)
      f_1 = self._CreateOverlayPaths(
          root_dir, 'overlay-lumpy',
          '[GENERAL]\nsubmit-in-pre-cq: yes\n')
      diff_dict = {f_1: 'M'}
      change = self._patch_factory.MockPatch()
      self.PatchObject(cros_patch.GerritPatch, 'GetDiffStatus',
                       return_value=diff_dict)

      parser = self.CreateCQConfigParser(
          change=change, common_config_file=root_ini, checkout=mock_checkout)
      self.assertFalse(parser.CanSubmitChangeInPreCQ())

  def testCanSubmitChangeInPreCqNoExplicitNoSubConfigs(self):
    """Test CanSubmitChangeInPreCq when sub-configs are disabled."""
    mock_checkout = mock.Mock()
    with osutils.TempDir(set_global=True) as tempdir:
      root_dir = os.path.join(tempdir, 'overlays')
      mock_checkout.GetPath.return_value = root_dir
      root_ini = os.path.join(root_dir, 'COMMIT-QUEUE.ini')
      osutils.WriteFile(
          root_ini,
          '[GENERAL]\n'
          'submit-in-pre-cq: no\n'
          'union-pre-cq-sub-configs: no\n',
          makedirs=True)
      f_1 = self._CreateOverlayPaths(
          root_dir, 'overlay-lumpy',
          '[GENERAL]\nsubmit-in-pre-cq: yes\n')
      diff_dict = {f_1: 'M'}
      change = self._patch_factory.MockPatch()
      self.PatchObject(cros_patch.GerritPatch, 'GetDiffStatus',
                       return_value=diff_dict)

      parser = self.CreateCQConfigParser(
          change=change, common_config_file=root_ini, checkout=mock_checkout)
      self.assertFalse(parser.CanSubmitChangeInPreCQ())

  def testCanSubmitChangeAllowedByAllSubConfigs(self):
    """Test CanSubmitChangeInPreCq when all sub-configs allow it."""
    mock_checkout = mock.Mock()
    with osutils.TempDir(set_global=True) as tempdir:
      root_dir = os.path.join(tempdir, 'overlays')
      mock_checkout.GetPath.return_value = root_dir
      root_ini = os.path.join(root_dir, 'COMMIT-QUEUE.ini')
      osutils.WriteFile(
          root_ini,
          '[GENERAL]\n'
          'submit-in-pre-cq: no\n'
          'union-pre-cq-sub-configs: yes\n',
          makedirs=True)
      f_1 = self._CreateOverlayPaths(
          root_dir, 'overlay-lumpy',
          '[GENERAL]\nsubmit-in-pre-cq: yes\n')
      f_2 = self._CreateOverlayPaths(
          root_dir, 'overlay-link',
          '[GENERAL]\nsubmit-in-pre-cq: yes\n')
      diff_dict = {f: 'M' for f in (f_1, f_2)}
      change = self._patch_factory.MockPatch()
      self.PatchObject(cros_patch.GerritPatch, 'GetDiffStatus',
                       return_value=diff_dict)

      parser = self.CreateCQConfigParser(
          change=change, common_config_file=root_ini, checkout=mock_checkout)
      self.assertTrue(parser.CanSubmitChangeInPreCQ())

  def testCanSubmitChangeDisallwedByParentConfig(self):
    """Test CanSubmitChangeInPreCq when sub-config allows it, but not root."""
    mock_checkout = mock.Mock()
    with osutils.TempDir(set_global=True) as tempdir:
      root_dir = os.path.join(tempdir, 'overlays')
      mock_checkout.GetPath.return_value = root_dir
      root_ini = os.path.join(root_dir, 'COMMIT-QUEUE.ini')
      osutils.WriteFile(
          root_ini,
          '[GENERAL]\n'
          'submit-in-pre-cq: no\n'
          'union-pre-cq-sub-configs: yes\n',
          makedirs=True)
      f_1 = self._CreateOverlayPaths(
          root_dir, 'overlay-lumpy',
          '[GENERAL]\nsubmit-in-pre-cq: yes\n')
      f_2 = self._CreateOverlayPaths(
          root_dir, 'overlay-link',
          '[GENERAL]\n')
      diff_dict = {f: 'M' for f in (f_1, f_2)}
      change = self._patch_factory.MockPatch()
      self.PatchObject(cros_patch.GerritPatch, 'GetDiffStatus',
                       return_value=diff_dict)

      parser = self.CreateCQConfigParser(
          change=change, common_config_file=root_ini, checkout=mock_checkout)
      self.assertFalse(parser.CanSubmitChangeInPreCQ())

  def testCanSubmitChangeDisallwedByParentConfigByDefault(self):
    """Test CanSubmitChangeInPreCq when sub-config allows it, but not root."""
    mock_checkout = mock.Mock()
    with osutils.TempDir(set_global=True) as tempdir:
      root_dir = os.path.join(tempdir, 'overlays')
      mock_checkout.GetPath.return_value = root_dir
      root_ini = os.path.join(root_dir, 'COMMIT-QUEUE.ini')
      osutils.WriteFile(
          root_ini,
          '[GENERAL]\n'
          'union-pre-cq-sub-configs: yes\n',
          makedirs=True)
      f_1 = self._CreateOverlayPaths(
          root_dir, 'overlay-lumpy',
          '[GENERAL]\nsubmit-in-pre-cq: yes\n')
      f_2 = self._CreateOverlayPaths(
          root_dir, 'overlay-link',
          '[GENERAL]\n')
      diff_dict = {f: 'M' for f in (f_1, f_2)}
      change = self._patch_factory.MockPatch()
      self.PatchObject(cros_patch.GerritPatch, 'GetDiffStatus',
                       return_value=diff_dict)

      parser = self.CreateCQConfigParser(
          change=change, common_config_file=root_ini, checkout=mock_checkout)
      self.assertFalse(parser.CanSubmitChangeInPreCQ())

  def GetOption(self, path, section='a', option='b'):
    # pylint: disable=protected-access
    return cq_config.CQConfigParser._GetOptionFromConfigFile(
        path, section, option)

  def testGetOptionFromConfigFileOnBadConfigFile(self):
    """Test if we can handle an incorrectly formatted config file."""
    with osutils.TempDir(set_global=True) as tempdir:
      path = os.path.join(tempdir, 'foo.ini')
      osutils.WriteFile(path, 'foobar')
      self.assertRaises(ConfigParser.Error, self.GetOption, path)

  def testGetOptionFromConfigFileOnMissingConfigFile(self):
    """Test if we can handle a missing config file."""
    with osutils.TempDir(set_global=True) as tempdir:
      path = os.path.join(tempdir, 'foo.ini')
      self.assertEqual(None, self.GetOption(path))

  def testGetOptionFromConfigFileOnGoodConfigFile(self):
    """Test if we can handle a good config file."""
    with osutils.TempDir(set_global=True) as tempdir:
      path = os.path.join(tempdir, 'foo.ini')
      osutils.WriteFile(path, '[a]\nb: bar baz\n')
      ignored = self.GetOption(path)
      self.assertEqual('bar baz', ignored)

  def testGetConfigFileForAffectedPathReturnsSubdirPath(self):
    """Test _GetConfigFileForAffectedPath which returns subdir path."""
    # Create paths:
    # /tmpdir/overlays/COMMIT-QUEUE.ini
    # /tmpdir/overlays/overlay-lumpy/COMMIT-QUEUE.ini
    # /tmpdir/overlays/overlay-lumpy/test/test.py
    parser = self.CreateCQConfigParser()
    with osutils.TempDir(set_global=True) as tempdir:
      root_dir = os.path.join(tempdir, 'overlays')
      root_ini = os.path.join(root_dir, 'COMMIT-QUEUE.ini')
      osutils.WriteFile(root_ini, '[GENERAL]\npre-cq-configs: default\n',
                        makedirs=True)
      sub_dir = os.path.join(root_dir, 'overlay-lumpy')
      sub_ini = os.path.join(sub_dir, 'COMMIT-QUEUE.ini')
      osutils.WriteFile(sub_ini, '[GENERAL]\npre-cq-configs: lumpy-pre-cq\n',
                        makedirs=True)
      changed_file = os.path.join(sub_dir, 'test', 'test.py')
      osutils.WriteFile(changed_file, '#test#', makedirs=True)
      config_path = parser._GetConfigFileForAffectedPath(
          changed_file, root_dir)

      self.assertEqual(config_path, sub_ini)

  def testGetConfigFileForAffectedPathReturnsRootPath(self):
    """Test _GetConfigFileForAffectedPath which returns root path."""
    # Create paths:
    # /tmpdir/overlays/COMMIT-QUEUE.ini
    # /tmpdir/overlays/overlay-lumpy/test/test.py
    parser = self.CreateCQConfigParser()
    with osutils.TempDir(set_global=True) as tempdir:
      root_dir = os.path.join(tempdir, 'overlays')
      root_ini = os.path.join(root_dir, 'COMMIT-QUEUE.ini')
      osutils.WriteFile(root_ini, '[GENERAL]\npre-cq-configs: default\n',
                        makedirs=True)
      sub_dir = os.path.join(root_dir, 'overlay-lumpy')
      changed_file = os.path.join(sub_dir, 'test', 'test.py')
      osutils.WriteFile(changed_file, '#test#', makedirs=True)
      config_path = parser._GetConfigFileForAffectedPath(
          changed_file, root_dir)

      self.assertEqual(config_path, root_ini)

  def testGetConfigFileForAffectedPathWithoutConfigFiles(self):
    """Test _GetConfigFileForAffectedPath without ConfigFiles."""
    # Create paths:
    # /tmpdir/overlays/overlay-lumpy/test/test.py
    parser = self.CreateCQConfigParser()
    with osutils.TempDir(set_global=True) as tempdir:
      root_dir = os.path.join(tempdir, 'overlays')
      root_ini = os.path.join(root_dir, 'COMMIT-QUEUE.ini')
      sub_dir = os.path.join(root_dir, 'overlay-lumpy')
      changed_file = os.path.join(sub_dir, 'test', 'test.py')
      osutils.WriteFile(changed_file, '#test#', makedirs=True)
      config_path = parser._GetConfigFileForAffectedPath(
          changed_file, root_dir)

      self.assertEqual(config_path, root_ini)

  def testGetConfigFileForAffectedPathOnDeletedFiles(self):
    """Test _GetConfigFileForAffectedPath on deleted files."""
    # Create paths:
    # /tmpdir/overlays/COMMIT-QUEUE.ini
    parser = self.CreateCQConfigParser()
    with osutils.TempDir(set_global=True) as tempdir:
      root_dir = os.path.join(tempdir, 'overlays')
      root_ini = os.path.join(root_dir, 'COMMIT-QUEUE.ini')
      sub_dir = os.path.join(root_dir, 'overlay-lumpy')
      changed_file = os.path.join(sub_dir, 'test', 'test.py')
      config_path = parser._GetConfigFileForAffectedPath(
          changed_file, root_dir)

      self.assertEqual(config_path, root_ini)
