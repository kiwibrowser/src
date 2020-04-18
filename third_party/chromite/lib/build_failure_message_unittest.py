# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing unit tests for build_failure_message."""

from __future__ import print_function

import mock

from chromite.lib import build_failure_message
from chromite.lib import constants
from chromite.lib import cros_test_lib
from chromite.lib import failure_message_lib
from chromite.lib import failure_message_lib_unittest
from chromite.lib import hwtest_results
from chromite.lib import patch_unittest
from chromite.lib import portage_util
from chromite.lib import triage_lib


failure_message_helper = failure_message_lib_unittest.FailureMessageHelper()


class BuildFailureMessageTests(cros_test_lib.MockTestCase):
  """Tests for BuildFailureMessage."""

  def ConstructBuildFailureMessage(self, message_summary="message_summary",
                                   failure_messages=None, internal=True,
                                   reason='reason', builder='builder'):
    return build_failure_message.BuildFailureMessage(
        message_summary, failure_messages, internal, reason, builder)

  def setUp(self):
    self._patch_factory = patch_unittest.MockPatchFactory()

  def _GetBuildFailureMessageWithMixedMsgs(self):
    failure_messages = (
        failure_message_helper.GetBuildFailureMessageWithMixedMsgs())
    build_failure = self.ConstructBuildFailureMessage(
        failure_messages=failure_messages)

    return build_failure

  def testBuildFailureMessageToStr(self):
    """Test BuildFailureMessageToStr."""
    build_failure = self._GetBuildFailureMessageWithMixedMsgs()

    self.assertIsNotNone(build_failure.BuildFailureMessageToStr())

  def testGetFailingStages(self):
    """Test GetFailingStages."""
    build_failure = self._GetBuildFailureMessageWithMixedMsgs()
    failing_stages = build_failure.GetFailingStages()

    self.assertItemsEqual(failing_stages, ['Paygen', 'InitSDK', 'BuildImage'])

  def testMatchesExceptionCategoriesOnMixedFailuresReturnsFalse(self):
    """Test MatchesExceptionCategories on mixed failures returns False."""
    build_failure = self._GetBuildFailureMessageWithMixedMsgs()

    self.assertFalse(build_failure.MatchesExceptionCategories(
        {constants.EXCEPTION_CATEGORY_BUILD}))

  def testMatchesExceptionCategoriesOnBuildFailuresReturnsTrue(self):
    """Test MatchesExceptionCategories on build failures returns True."""
    failure_messages = [failure_message_helper.GetBuildScriptFailureMessage(),
                        failure_message_helper.GetPackageBuildFailureMessage()]
    build_failure = self.ConstructBuildFailureMessage(
        failure_messages=failure_messages)

    self.assertTrue(build_failure.MatchesExceptionCategories(
        {constants.EXCEPTION_CATEGORY_BUILD}))

  def testMatchesExceptionCategoriesOnCompoundFailuresReturnsTrue(self):
    """Test MatchesExceptionCategories on CompoundFailures returns True."""
    f_1 = failure_message_helper.GetBuildScriptFailureMessage(
        failure_id=1, outer_failure_id=3)
    f_2 = failure_message_helper.GetPackageBuildFailureMessage(
        failure_id=2, outer_failure_id=3)
    f_3 = failure_message_helper.GetStageFailureMessage(failure_id=3)
    f_4 = failure_message_helper.GetBuildScriptFailureMessage(failure_id=4)
    failures = (failure_message_lib.FailureMessageManager.ReconstructMessages(
        [f_1, f_2, f_3, f_4]))
    build_failure = self.ConstructBuildFailureMessage(
        failure_messages=failures)

    self.assertTrue(build_failure.MatchesExceptionCategories(
        {constants.EXCEPTION_CATEGORY_BUILD}))

  def testMatchesExceptionCategoriesOnCompoundFailuresReturnsFalse(self):
    """Test MatchesExceptionCategories on CompoundFailures returns False."""
    f_1 = failure_message_helper.GetStageFailureMessage(
        failure_id=1, outer_failure_id=3)
    f_2 = failure_message_helper.GetPackageBuildFailureMessage(
        failure_id=2, outer_failure_id=3)
    f_3 = failure_message_helper.GetStageFailureMessage(failure_id=3)
    f_4 = failure_message_helper.GetBuildScriptFailureMessage(failure_id=4)
    failures = (failure_message_lib.FailureMessageManager.ReconstructMessages(
        [f_1, f_2, f_3, f_4]))
    build_failure = self.ConstructBuildFailureMessage(
        failure_messages=failures)

    self.assertFalse(build_failure.MatchesExceptionCategories(
        {constants.EXCEPTION_CATEGORY_BUILD}))

  def testHasExceptionCategoriesOnMixedFailures(self):
    """Test HasExceptionCategories on mixed failures."""
    build_failure = self._GetBuildFailureMessageWithMixedMsgs()

    self.assertTrue(build_failure.HasExceptionCategories(
        {constants.EXCEPTION_CATEGORY_BUILD}))
    self.assertTrue(build_failure.HasExceptionCategories(
        {constants.EXCEPTION_CATEGORY_UNKNOWN}))
    self.assertFalse(build_failure.HasExceptionCategories(
        {constants.EXCEPTION_CATEGORY_INFRA}))
    self.assertFalse(build_failure.HasExceptionCategories(
        {constants.EXCEPTION_CATEGORY_LAB}))

  def tesHasExceptionCategoriesOnCompoundFailures(self):
    """Test HasExceptionCategories on CompoundFailures."""
    f_1 = failure_message_helper.GetBuildScriptFailureMessage(
        failure_id=1, outer_failure_id=3)
    f_2 = failure_message_helper.GetPackageBuildFailureMessage(
        failure_id=2, outer_failure_id=3)
    f_3 = failure_message_helper.GetStageFailureMessage(failure_id=3)
    failures = (failure_message_lib.FailureMessageManager.ReconstructMessages(
        [f_1, f_2, f_3]))
    build_failure = self.ConstructBuildFailureMessage(
        failure_messages=failures)

    self.assertTrue(build_failure.HasExceptionCategories(
        {constants.EXCEPTION_CATEGORY_BUILD}))
    self.assertTrue(build_failure.HasExceptionCategories(
        {constants.EXCEPTION_CATEGORY_UNKNOWN}))
    self.assertFalse(build_failure.HasExceptionCategories(
        {constants.EXCEPTION_CATEGORY_INFRA}))
    self.assertFalse(build_failure.HasExceptionCategories(
        {constants.EXCEPTION_CATEGORY_LAB}))

  def _GetMockChanges(self):
    mock_change_1 = self._patch_factory.MockPatch(
        project='chromiumos/overlays/chromiumos-overlay')
    mock_change_2 = self._patch_factory.MockPatch(
        project='chromiumos/overlays/chromiumos-overlay')
    mock_change_3 = self._patch_factory.MockPatch(
        project='chromiumos/chromite')
    mock_change_4 = self._patch_factory.MockPatch(
        project='chromeos/chromeos-admin')
    return [mock_change_1, mock_change_2, mock_change_3, mock_change_4]

  def _CreateBuildFailure(self):
    f_1 = failure_message_helper.GetPackageBuildFailureMessage(failure_id=1)
    f_2 = failure_message_helper.GetPackageBuildFailureMessage(failure_id=2)
    f_3 = failure_message_helper.GetPackageBuildFailureMessage(failure_id=3)
    failures = (failure_message_lib.FailureMessageManager.ReconstructMessages(
        [f_1, f_2, f_3]))
    return self.ConstructBuildFailureMessage(
        failure_messages=failures)

  def testFindPackageBuildFailureSuspectsReturnsSuspects(self):
    """Test FindPackageBuildFailureSuspects which returns suspects."""
    changes = self._GetMockChanges()
    f_1 = failure_message_helper.GetPackageBuildFailureMessage(failure_id=1)
    build_failure = self.ConstructBuildFailureMessage(failure_messages=[f_1])

    self.PatchObject(portage_util, 'FindWorkonProjects',
                     return_value='chromiumos/overlays/chromiumos-overlay')
    suspects, no_assignee_packages = (
        build_failure.FindPackageBuildFailureSuspects(changes, f_1))
    self.assertItemsEqual([changes[0], changes[1]], suspects)
    self.assertFalse(no_assignee_packages)

    self.PatchObject(portage_util, 'FindWorkonProjects',
                     return_value='chromiumos/chromite')
    suspects, no_assignee_packages = (
        build_failure.FindPackageBuildFailureSuspects(changes, f_1))
    self.assertItemsEqual([changes[2]], suspects)
    self.assertFalse(no_assignee_packages)

  def testFindPackageBuildFailureSuspectsNoSuspects(self):
    """Test FindPackageBuildFailureSuspects which returns empty suspects."""
    changes = self._GetMockChanges()
    f_1 = failure_message_helper.GetPackageBuildFailureMessage(failure_id=1)
    build_failure = self.ConstructBuildFailureMessage(failure_messages=[f_1])
    self.PatchObject(portage_util, 'FindWorkonProjects',
                     return_value='chromiumos/third_party/kernel')
    suspects, no_assignee_packages = (
        build_failure.FindPackageBuildFailureSuspects(changes, f_1))

    self.assertEqual(suspects, set())
    self.assertTrue(no_assignee_packages)

  def testFindPackageBuildFailureSuspectsNoFailedPackages(self):
    """Test FindPackageBuildFailureSuspects without FailedPackages."""
    changes = self._GetMockChanges()
    f_1 = failure_message_helper.GetPackageBuildFailureMessage(
        failure_id=1, extra_info=None)
    build_failure = self.ConstructBuildFailureMessage(failure_messages=[f_1])

    self.PatchObject(portage_util, 'FindWorkonProjects',
                     return_value='chromiumos/overlays/chromiumos-overlay')
    suspects, no_assignee_packages = (
        build_failure.FindPackageBuildFailureSuspects(changes, f_1))

    self.assertEqual(suspects, set())
    self.assertFalse(no_assignee_packages)

  def testFindSuspectedChangesOnPackageBuildFailuresNotBlameEverything(self):
    """Test FindSuspectedChanges on PackageBuildFailures not BlameEverything."""
    changes = self._GetMockChanges()
    f_1 = failure_message_helper.GetPackageBuildFailureMessage(failure_id=1)
    build_failure = self.ConstructBuildFailureMessage(failure_messages=[f_1])
    mock_find = self.PatchObject(build_failure_message.BuildFailureMessage,
                                 'FindPackageBuildFailureSuspects',
                                 return_value=({changes[2]}, False))

    suspects = build_failure.FindSuspectedChanges(
        changes, mock.Mock(), mock.Mock(), True)
    expected = triage_lib.SuspectChanges({
        changes[0]: constants.SUSPECT_REASON_OVERLAY_CHANGE,
        changes[1]: constants.SUSPECT_REASON_OVERLAY_CHANGE,
        changes[2]: constants.SUSPECT_REASON_BUILD_FAIL})
    self.assertEqual(suspects, expected)
    mock_find.assert_called_once_with(changes, f_1)

    mock_find.reset_mock()
    suspects = build_failure.FindSuspectedChanges(
        changes, mock.Mock(), mock.Mock(), False)
    expected = expected = triage_lib.SuspectChanges({
        changes[2]: constants.SUSPECT_REASON_BUILD_FAIL})
    self.assertEqual(suspects, expected)
    mock_find.assert_called_once_with(changes, f_1)

  def testFindSuspectedChangesOnPackageBuildFailuresBlameEverything(self):
    """Test FindSuspectedChanges on PackageBuildFailures and BlameEverything."""
    changes = self._GetMockChanges()
    f_1 = failure_message_helper.GetPackageBuildFailureMessage(failure_id=1)
    build_failure = self.ConstructBuildFailureMessage(failure_messages=[f_1])
    mock_find = self.PatchObject(build_failure_message.BuildFailureMessage,
                                 'FindPackageBuildFailureSuspects',
                                 return_value=({changes[2]}, True))
    suspects = build_failure.FindSuspectedChanges(
        changes, mock.Mock(), mock.Mock(), True)
    expected = triage_lib.SuspectChanges({
        changes[0]: constants.SUSPECT_REASON_UNKNOWN,
        changes[1]: constants.SUSPECT_REASON_UNKNOWN,
        changes[2]: constants.SUSPECT_REASON_BUILD_FAIL,
        changes[3]: constants.SUSPECT_REASON_UNKNOWN})

    self.assertEqual(suspects, expected)
    mock_find.assert_called_once_with(changes, f_1)
    mock_find.reset_mock()

    suspects = build_failure.FindSuspectedChanges(
        changes, mock.Mock(), mock.Mock(), False)
    expected = triage_lib.SuspectChanges({
        changes[2]: constants.SUSPECT_REASON_BUILD_FAIL})
    self.assertEqual(suspects, expected)
    mock_find.assert_called_once_with(changes, f_1)

  def testFindSuspectedChangesOnHWTestFailuresNotBlameEverything(self):
    """Test FindSuspectedChanges on HWTestFailures do not blame everything."""
    changes = self._GetMockChanges()
    f_1 = failure_message_helper.GetTestFailureMessage(failure_id=1)
    build_failure = self.ConstructBuildFailureMessage(failure_messages=[f_1])
    mock_find = self.PatchObject(hwtest_results.HWTestResultManager,
                                 'FindHWTestFailureSuspects',
                                 return_value=({changes[2]}, False))

    build_root = mock.Mock()
    failed_hwtests = mock.Mock()

    suspects = build_failure.FindSuspectedChanges(
        changes, build_root, failed_hwtests, True)
    expected = triage_lib.SuspectChanges({
        changes[0]: constants.SUSPECT_REASON_OVERLAY_CHANGE,
        changes[1]: constants.SUSPECT_REASON_OVERLAY_CHANGE,
        changes[2]: constants.SUSPECT_REASON_TEST_FAIL})
    self.assertEqual(suspects, expected)
    mock_find.assert_called_once_with(changes, build_root, failed_hwtests)

    mock_find.reset_mock()
    suspects = build_failure.FindSuspectedChanges(
        changes, build_root, failed_hwtests, False)
    expected = expected = triage_lib.SuspectChanges({
        changes[2]: constants.SUSPECT_REASON_TEST_FAIL})
    self.assertEqual(suspects, expected)
    mock_find.assert_called_once_with(changes, build_root, failed_hwtests)

  def testFindSuspectedChangesOnHWTestFailuresBlameEverything(self):
    """Test FindSuspectedChanges on HWTestFailures and blame everything."""
    changes = self._GetMockChanges()
    f_1 = failure_message_helper.GetTestFailureMessage(failure_id=1)
    build_failure = self.ConstructBuildFailureMessage(failure_messages=[f_1])
    mock_find = self.PatchObject(hwtest_results.HWTestResultManager,
                                 'FindHWTestFailureSuspects',
                                 return_value=({changes[2]}, True))
    build_root = mock.Mock()
    failed_hwtests = mock.Mock()

    suspects = build_failure.FindSuspectedChanges(
        changes, build_root, failed_hwtests, True)
    expected = triage_lib.SuspectChanges({
        changes[0]: constants.SUSPECT_REASON_UNKNOWN,
        changes[1]: constants.SUSPECT_REASON_UNKNOWN,
        changes[2]: constants.SUSPECT_REASON_TEST_FAIL,
        changes[3]: constants.SUSPECT_REASON_UNKNOWN})
    self.assertEqual(suspects, expected)
    mock_find.assert_called_once_with(changes, build_root, failed_hwtests)
    mock_find.reset_mock()

    suspects = build_failure.FindSuspectedChanges(
        changes, build_root, failed_hwtests, False)
    expected = triage_lib.SuspectChanges({
        changes[2]: constants.SUSPECT_REASON_TEST_FAIL})
    self.assertEqual(suspects, expected)
    mock_find.assert_called_once_with(changes, build_root, failed_hwtests)

  def testFindSuspectedChangesOnUnknownFailures(self):
    """Test FindSuspectedChanges on unknown failures."""
    changes = self._GetMockChanges()
    f_1 = failure_message_helper.GetStageFailureMessage(failure_id=1)
    build_failure = self.ConstructBuildFailureMessage(failure_messages=[f_1])
    mock_find_build_failure = self.PatchObject(
        build_failure_message.BuildFailureMessage,
        'FindPackageBuildFailureSuspects',
        return_value=({changes[2]}, True))
    mock_find_hwtest_failure = self.PatchObject(
        hwtest_results.HWTestResultManager,
        'FindHWTestFailureSuspects',
        return_value=({changes[2]}, False))
    build_root = mock.Mock()
    failed_hwtests = mock.Mock()

    suspects = build_failure.FindSuspectedChanges(
        changes, build_root, failed_hwtests, True)
    expected = triage_lib.SuspectChanges({
        changes[0]: constants.SUSPECT_REASON_UNKNOWN,
        changes[1]: constants.SUSPECT_REASON_UNKNOWN,
        changes[2]: constants.SUSPECT_REASON_UNKNOWN,
        changes[3]: constants.SUSPECT_REASON_UNKNOWN})
    self.assertEqual(suspects, expected)
    mock_find_build_failure.assert_not_called()
    mock_find_hwtest_failure.assert_not_called()

    suspects = build_failure.FindSuspectedChanges(
        changes, build_root, failed_hwtests, False)
    expected = triage_lib.SuspectChanges()
    self.assertEqual(suspects, expected)
    mock_find_build_failure.assert_not_called()
    mock_find_hwtest_failure.assert_not_called()
