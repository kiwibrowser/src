# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for config stages"""

from __future__ import print_function

import os
import mock

from chromite.cbuildbot import repository
from chromite.cbuildbot.stages import config_stages
from chromite.cbuildbot.stages import generic_stages_unittest
from chromite.cbuildbot.stages import test_stages
from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import git
from chromite.lib import gs


# pylint: disable=protected-access

class CheckTemplateStageTest(generic_stages_unittest.AbstractStageTestCase):
  """Tests for CheckTemplateStage."""

  TOT_PATH = (config_stages.GS_GE_TEMPLATE_BUCKET +
              'build_config.ToT.json')
  R52_PATH = (config_stages.GS_GE_TEMPLATE_BUCKET +
              'build_config.release-R52-7978.B.json')
  R53_PATH = (config_stages.GS_GE_TEMPLATE_BUCKET +
              'build_config.release-R53-7978.B.json')
  R54_PATH = (config_stages.GS_GE_TEMPLATE_BUCKET +
              'build_config.release-R54-7978.B.json')

  def setUp(self):
    self._Prepare()
    self.PatchObject(repository, 'CloneWorkingRepo')
    self.PatchObject(gs, 'GSContext')
    self.update_mock = self.PatchObject(config_stages.UpdateConfigStage, 'Run')

  def ConstructStage(self):
    return config_stages.CheckTemplateStage(self._run)

  def testListTemplates(self):
    """Test _ListTemplates."""
    self.PatchObject(config_stages.CheckTemplateStage, 'SortAndGetReleasePaths',
                     return_value=['R_template.json'])
    stage = self.ConstructStage()
    stage.ctx = mock.Mock()
    stage.ctx.LS.return_value = ['template.json']

    gs_paths = stage._ListTemplates()
    self.assertItemsEqual(gs_paths, ['R_template.json', 'template.json'])

  def test_ListTemplatesWithNoSuchKeyError(self):
    """Test _ListTemplates with NoSuchKeyError."""
    stage = self.ConstructStage()
    stage.ctx = mock.Mock()
    stage.ctx.LS.side_effect = gs.GSNoSuchKey()

    gs_paths = stage._ListTemplates()
    self.assertEqual(gs_paths, [])

  def testBasicPerformStage(self):
    """Test basic PerformStage."""
    self.PatchObject(config_stages.CheckTemplateStage, '_ListTemplates',
                     return_value=[self.TOT_PATH, self.R54_PATH])
    stage = self.ConstructStage()

    stage.PerformStage()
    self.assertTrue(self.update_mock.call_count == 2)

  def testSortAndGetReleasePaths(self):
    """Test SortAndGetReleasePaths."""
    stage = self.ConstructStage()

    paths = stage.SortAndGetReleasePaths(
        [self.R54_PATH, self.R53_PATH, self.R52_PATH])
    self.assertTrue(len(paths) == 1)
    self.assertEqual(paths[0], self.R54_PATH)


class UpdateConfigStageTest(generic_stages_unittest.AbstractStageTestCase):
  """Tests for UpdateConfigStage."""

  def setUp(self):
    self._Prepare()
    self.PatchObject(config_stages.UpdateConfigStage, '_DownloadTemplate')
    self.PatchObject(config_stages.UpdateConfigStage, '_CheckoutBranch')
    self.PatchObject(config_stages.UpdateConfigStage, '_UpdateConfigDump')
    self.PatchObject(git, 'PushBranch')
    self.PatchObject(git, 'RunGit')
    self.PatchObject(repository, 'CloneWorkingRepo')
    self.PatchObject(cros_build_lib, 'RunCommand')

    self.project = 'chromite'
    self.chromite_dir = config_stages.GetProjectRepoDir(
        'chromite', constants.CHROMITE_URL)

  # pylint: disable=W0221
  def ConstructStage(self, template):
    template_path = config_stages.GS_GE_TEMPLATE_BUCKET + template
    branch = config_stages.GetBranchName(template)
    return config_stages.UpdateConfigStage(
        self._run, template_path, branch, self.chromite_dir, True)

  def testCreateConfigPatch(self):
    """Test _CreateConfigPatch."""
    template = 'build_config.ToT.json'
    stage = self.ConstructStage(template)

    with mock.patch('__builtin__.open'):
      config_change_patch = stage._CreateConfigPatch()
      self.assertEqual(os.path.basename(config_change_patch),
                       'config_change.patch')

  def testRunBinhostTest(self):
    """Test RunBinhostTest."""
    self.PatchObject(config_stages.UpdateConfigStage,
                     '_CreateConfigPatch', return_value='patch')
    mock_binhost_run = self.PatchObject(test_stages.BinhostTestStage, 'Run')
    mock_run_git = self.PatchObject(git, 'RunGit')
    template = 'build_config.ToT.json'
    stage = self.ConstructStage(template)

    stage._RunBinhostTest()
    self.assertEqual(mock_run_git.call_count, 2)
    mock_binhost_run.assert_called_once_with()

  def testMasterBasic(self):
    """Basic test on master branch."""
    mock_binhost_test = self.PatchObject(config_stages.UpdateConfigStage,
                                         '_RunBinhostTest')
    template = 'build_config.ToT.json'
    stage = self.ConstructStage(template)
    stage.PerformStage()
    self.assertTrue(stage.branch == 'master')
    mock_binhost_test.assert_called_once_with()

  def testReleaseBasic(self):
    """Basic test on release branch."""
    template = 'build_config.release-R50-7978.B.json'
    mock_binhost_test = self.PatchObject(config_stages.UpdateConfigStage,
                                         '_RunBinhostTest')
    stage = self.ConstructStage(template)
    stage.PerformStage()
    self.assertTrue(stage.branch == 'release-R50-7978.B')
    mock_binhost_test.assert_not_called()
