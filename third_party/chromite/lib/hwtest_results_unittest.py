# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for hwtest_results."""

from __future__ import print_function

import mock

from chromite.lib.const import waterfall
from chromite.lib import constants
from chromite.lib import cros_test_lib
from chromite.lib import fake_cidb
from chromite.lib import git
from chromite.lib import hwtest_results
from chromite.lib import patch_unittest


class HWTestResultTest(cros_test_lib.MockTestCase):
  """Tests for HWTestResult."""

  def testNormalizeTestName(self):
    """Test NormalizeTestName."""
    self.assertEqual(hwtest_results.HWTestResult.NormalizeTestName(
        'Suite job'), None)
    self.assertEqual(hwtest_results.HWTestResult.NormalizeTestName(
        'cheets_CTS.com.android.cts.dram'), 'cheets_CTS')
    self.assertEqual(hwtest_results.HWTestResult.NormalizeTestName(
        'security_NetworkListeners'), 'security_NetworkListeners')


class HWTestResultManagerTest(cros_test_lib.MockTestCase):
  """Tests for HWTestResultManager."""

  def setUp(self):
    self.manager = hwtest_results.HWTestResultManager()
    self.db = fake_cidb.FakeCIDBConnection()
    self._patch_factory = patch_unittest.MockPatchFactory()

  def _ReportHWTestResults(self):
    build_1 = self.db.InsertBuild('build_1', waterfall.WATERFALL_INTERNAL, 1,
                                  'build_1', 'bot_hostname')
    build_2 = self.db.InsertBuild('build_2', waterfall.WATERFALL_INTERNAL, 2,
                                  'build_2', 'bot_hostname')
    r1 = hwtest_results.HWTestResult.FromReport(
        build_1, 'Suite job', 'pass')
    r2 = hwtest_results.HWTestResult.FromReport(
        build_1, 'test_b.test', 'fail')
    r3 = hwtest_results.HWTestResult.FromReport(
        build_1, 'test_c.test', 'abort')
    r4 = hwtest_results.HWTestResult.FromReport(
        build_2, 'test_d.test', 'other')
    r5 = hwtest_results.HWTestResult.FromReport(
        build_2, 'test_e', 'pass')

    self.db.InsertHWTestResults([r1, r2, r3, r4, r5])

    return ((build_1, build_2), (r1, r2, r3, r4, r5))

  def GetHWTestResultsFromCIDB(self):
    """Test GetHWTestResultsFromCIDB."""
    (build_1, build_2), _ = self._ReportHWTestResults()

    expect_r1 = hwtest_results.HWTestResult(0, build_1, 'Suite job', 'pass')
    expect_r2 = hwtest_results.HWTestResult(1, build_1, 'test_b.test', 'fail')
    expect_r3 = hwtest_results.HWTestResult(2, build_1, 'test_c.test', 'abort')
    expect_r4 = hwtest_results.HWTestResult(3, build_2, 'test_d.test', 'other')
    expect_r5 = hwtest_results.HWTestResult(4, build_2, 'test_e', 'pass')

    results = self.manager.GetHWTestResultsFromCIDB(self.db, [build_1])
    self.assertItemsEqual(results, [expect_r1, expect_r2, expect_r3])

    results = self.manager.GetHWTestResultsFromCIDB(
        self.db, [build_1], test_statues=constants.HWTEST_STATUES_NOT_PASSED)
    self.assertItemsEqual(results, [expect_r2, expect_r3])

    results = self.manager.GetHWTestResultsFromCIDB(self.db, [build_1, build_2])
    self.assertItemsEqual(
        results, [expect_r1, expect_r2, expect_r3, expect_r4, expect_r5])

    results = self.manager.GetHWTestResultsFromCIDB(
        self.db, [build_1, build_2],
        test_statues=constants.HWTEST_STATUES_NOT_PASSED)
    self.assertItemsEqual(results, [expect_r2, expect_r3, expect_r4])

  def testGetFailedHWTestsFromCIDB(self):
    """Test GetFailedHWTestsFromCIDB."""
    (build_1, build_2), (r1, r2, r3, r4, r5) = self._ReportHWTestResults()
    mock_get_hwtest_results = self.PatchObject(
        hwtest_results.HWTestResultManager, 'GetHWTestResultsFromCIDB',
        return_value=[r1, r2, r3, r4, r5])
    failed_tests = self.manager.GetFailedHWTestsFromCIDB(
        self.db, [build_1, build_2])

    self.assertItemsEqual(failed_tests,
                          ['test_b', 'test_c', 'test_d', 'test_e'])
    mock_get_hwtest_results.assert_called_once_with(
        self.db, [build_1, build_2],
        test_statues=constants.HWTEST_STATUES_NOT_PASSED)

  def testGetFailedHwtestsAffectedByChange(self):
    """Test GetFailedHwtestsAffectedByChange."""
    manifest = mock.Mock()
    mock_change = mock.Mock()
    diffs = {'client/site_tests/graphics_dEQP/graphics_dEQP.py': 'M',
             'client/site_tests/graphics_Gbm/graphics_Gbm.py': 'M'}
    mock_change.GetDiffStatus.return_value = diffs
    self.PatchObject(git.ProjectCheckout, 'GetPath')

    failed_hwtests = {'graphics_dEQP'}
    self.assertItemsEqual(self.manager.GetFailedHwtestsAffectedByChange(
        mock_change, manifest, failed_hwtests), failed_hwtests)

    failed_hwtests = {'graphics_Gbm'}
    self.assertItemsEqual(self.manager.GetFailedHwtestsAffectedByChange(
        mock_change, manifest, failed_hwtests), failed_hwtests)

    failed_hwtests = {'graphics_dEQP', 'graphics_Gbm'}
    self.assertItemsEqual(self.manager.GetFailedHwtestsAffectedByChange(
        mock_change, manifest, failed_hwtests), failed_hwtests)

    failed_hwtests = {'audio_ActiveStreamStress'}
    self.assertItemsEqual(self.manager.GetFailedHwtestsAffectedByChange(
        mock_change, manifest, failed_hwtests), set())

    failed_hwtests = {'graphics_dEQP', 'graphics_Gbm',
                      'audio_ActiveStreamStress'}
    self.assertItemsEqual(self.manager.GetFailedHwtestsAffectedByChange(
        mock_change, manifest, failed_hwtests),
                          {'graphics_dEQP', 'graphics_Gbm'})

  def testFindHWTestFailureSuspects(self):
    """Test FindHWTestFailureSuspects."""
    self.PatchObject(git.ManifestCheckout, 'Cached')
    c1 = self._patch_factory.MockPatch(change_id=1, patch_number=1)
    c2 = self._patch_factory.MockPatch(change_id=2, patch_number=1)
    test_1 = mock.Mock()
    test_2 = mock.Mock()
    self.PatchObject(hwtest_results.HWTestResultManager,
                     'GetFailedHwtestsAffectedByChange',
                     return_value={test_1})
    suspects, no_assignee_hwtests = self.manager.FindHWTestFailureSuspects(
        [c1, c2], mock.Mock(), {test_1, test_2})

    self.assertItemsEqual(suspects, {c1, c2})
    self.assertTrue(no_assignee_hwtests)

  def testFindHWTestFailureSuspectsNoAssignees(self):
    """Test FindHWTestFailureSuspects when failures don't have assignees."""
    self.PatchObject(git.ManifestCheckout, 'Cached')
    c1 = self._patch_factory.MockPatch(change_id=1, patch_number=1)
    c2 = self._patch_factory.MockPatch(change_id=2, patch_number=1)
    test_1 = mock.Mock()
    test_2 = mock.Mock()
    self.PatchObject(hwtest_results.HWTestResultManager,
                     'GetFailedHwtestsAffectedByChange',
                     return_value=set())
    suspects, no_assignee_hwtests = self.manager.FindHWTestFailureSuspects(
        [c1, c2], mock.Mock(), {test_1, test_2})

    self.assertItemsEqual(suspects, set())
    self.assertTrue(no_assignee_hwtests)

  def testFindHWTestFailureSuspectsNoFailedHWTests(self):
    """Test FindHWTestFailureSuspects with empty failed HWTests."""
    self.PatchObject(git.ManifestCheckout, 'Cached')
    c1 = self._patch_factory.MockPatch(change_id=1, patch_number=1)
    c2 = self._patch_factory.MockPatch(change_id=2, patch_number=1)
    suspects, no_assignee_hwtests = self.manager.FindHWTestFailureSuspects(
        [c1, c2], mock.Mock(), set())

    self.assertItemsEqual(suspects, set())
    self.assertFalse(no_assignee_hwtests)
