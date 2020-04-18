# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing unit tests for failure_message_lib."""

from __future__ import print_function

from chromite.lib import constants
from chromite.lib import cros_test_lib
from chromite.lib import failure_message_lib


DEFAULT_BUILD_SCRIPT_FAILURE_EXTRA_INFO = (
    '{\"shortname\": \"./security_test_image\"}')
DEFAULT_PACKAGE_BUILD_FAILURE_EXTRA_INFO = (
    '{\"shortname\": \"./build_image\", \"failed_packages\":'
    '[\"chromeos-base/chromeos-chrome\", \"chromeos-base/chromeos-init\"]}')


class StageFailureHelper(object):
  """Helper method to create StageFailure instances for test."""

  @classmethod
  def CreateStageFailure(
      cls, failure_id=1, build_stage_id=1, outer_failure_id=None,
      exception_type='StepFailure', exception_message='exception message',
      exception_category=constants.EXCEPTION_CATEGORY_UNKNOWN,
      extra_info=None, timestamp=None, stage_name='stage_1', board='board_1',
      stage_status=constants.BUILDER_STATUS_PASSED, build_id=1,
      master_build_id=None, builder_name='builder_name_1',
      waterfall='waterfall', build_number='build_number_1',
      build_config='config_1', build_status=constants.BUILDER_STATUS_PASSED,
      important=True, buildbucket_id='bb_id'):
    return failure_message_lib.StageFailure(
        failure_id, build_stage_id, outer_failure_id, exception_type,
        exception_message, exception_category, extra_info, timestamp,
        stage_name, board, stage_status, build_id, master_build_id,
        builder_name, waterfall, build_number, build_config, build_status,
        important, buildbucket_id)

  @classmethod
  def GetStageFailure(
      cls,
      failure_id=1,
      build_stage_id=1,
      outer_failure_id=None,
      exception_type='StepFailure',
      exception_message='exception msg',
      exception_category=constants.EXCEPTION_CATEGORY_UNKNOWN,
      extra_info=None,
      stage_name='Paygen',
      build_config=None):
    return cls.CreateStageFailure(
        failure_id, build_stage_id, outer_failure_id, exception_type,
        exception_message, exception_category, extra_info, None, stage_name,
        None, None, None, None, None, None, None, build_config, None, None,
        None)


class StageFailureTests(cros_test_lib.TestCase):
  """Tests for StageFailure."""

  def testGetStageFailureFromMessage(self):
    """Test GetStageFailureFromMessage."""
    failure_message = FailureMessageHelper.GetStageFailureMessage()
    stage_failure = failure_message_lib.StageFailure.GetStageFailureFromMessage(
        failure_message)

    self.assertEqual(stage_failure.id, 1)
    self.assertEqual(stage_failure.exception_type, 'StepFailure')
    self.assertEqual(stage_failure.exception_category,
                     constants.EXCEPTION_CATEGORY_UNKNOWN)


class FailureMessageHelper(object):
  """Helper class to help create stage failure message instances for test."""

  @classmethod
  def GetStageFailureMessage(
      cls,
      failure_id=1,
      build_stage_id=1,
      outer_failure_id=None,
      exception_type='StepFailure',
      exception_message='exception msg',
      exception_category=constants.EXCEPTION_CATEGORY_UNKNOWN,
      extra_info=None,
      stage_name='Paygen'):
    stage_failure = StageFailureHelper.GetStageFailure(
        failure_id, build_stage_id, outer_failure_id, exception_type,
        exception_message, exception_category, extra_info, stage_name)

    return failure_message_lib.StageFailureMessage(stage_failure)

  @classmethod
  def GetBuildScriptFailureMessage(
      cls,
      failure_id=2,
      build_stage_id=0,
      outer_failure_id=None,
      exception_type='BuildScriptFailure',
      exception_message='exception msg',
      exception_category=constants.EXCEPTION_CATEGORY_BUILD,
      extra_info=DEFAULT_BUILD_SCRIPT_FAILURE_EXTRA_INFO,
      stage_name='InitSDK',):
    stage_failure = StageFailureHelper.GetStageFailure(
        failure_id, build_stage_id, outer_failure_id, exception_type,
        exception_message, exception_category, extra_info, stage_name)

    return failure_message_lib.BuildScriptFailureMessage(stage_failure)

  @classmethod
  def GetPackageBuildFailureMessage(
      cls,
      failure_id=3,
      build_stage_id=0,
      outer_failure_id=None,
      exception_type='PackageBuildFailure',
      exception_message='exception msg',
      exception_category=constants.EXCEPTION_CATEGORY_BUILD,
      extra_info=DEFAULT_PACKAGE_BUILD_FAILURE_EXTRA_INFO,
      stage_name='BuildImage',):
    stage_failure = StageFailureHelper.GetStageFailure(
        failure_id, build_stage_id, outer_failure_id, exception_type,
        exception_message, exception_category, extra_info, stage_name)

    return failure_message_lib.PackageBuildFailureMessage(stage_failure)

  @classmethod
  def GetCompoundFailureMessage(
      cls,
      failure_id=4,
      build_stage_id=0,
      outer_failure_id=None,
      exception_type='BackgroundFailure',
      exception_message='exception msg',
      exception_category=constants.EXCEPTION_CATEGORY_UNKNOWN,
      extra_info=None,
      stage_name='Archive',
      stage_prefix_name='Archive'):
    stage_failure = StageFailureHelper.GetStageFailure(
        failure_id, build_stage_id, outer_failure_id, exception_type,
        exception_message, exception_category, extra_info, stage_name)

    return failure_message_lib.CompoundFailureMessage(
        stage_failure, extra_info=extra_info,
        stage_prefix_name=stage_prefix_name)

  @classmethod
  def GetTestFailureMessage(
      cls,
      failure_id=5,
      build_stage_id=0,
      outer_failure_id=None,
      exception_type='TestFailure',
      exception_message='** HWTest failed (code 1) **',
      exception_category=constants.EXCEPTION_CATEGORY_TEST,
      extra_info=None,
      stage_name='HWTest'):
    stage_failure = StageFailureHelper.GetStageFailure(
        failure_id, build_stage_id, outer_failure_id, exception_type,
        exception_message, exception_category, extra_info, stage_name)

    return failure_message_lib.StageFailureMessage(stage_failure)

  @classmethod
  def GetBuildFailureMessageWithMixedMsgs(cls):
    f_1 = cls.GetStageFailureMessage(failure_id=1, outer_failure_id=4)
    f_2 = cls.GetBuildScriptFailureMessage(failure_id=2, outer_failure_id=4)
    f_3 = cls.GetPackageBuildFailureMessage(failure_id=3, outer_failure_id=4)
    f_4 = cls.GetStageFailureMessage(failure_id=4)
    f_5 = cls.GetStageFailureMessage(failure_id=5)
    f_6 = cls.GetBuildScriptFailureMessage(failure_id=6)
    f_7 = cls.GetPackageBuildFailureMessage(failure_id=7)

    return (failure_message_lib.FailureMessageManager.ReconstructMessages(
        [f_1, f_2, f_3, f_4, f_5, f_6, f_7]))


class StageFailureMessageTests(cros_test_lib.TestCase):
  """Tests for StageFailureMessage."""

  def testDecodeExtraInfoWithNoneExtraInfo(self):
    """Test _DecodeExtraInfo with None extra_info."""
    failure_message = FailureMessageHelper.GetStageFailureMessage()

    self.assertEqual(failure_message.extra_info, {})

  def testDecodeExtraInfoWithNoneValidExtraInfo(self):
    """Test _DecodeExtraInfo with valid extra_info."""
    failure_message = FailureMessageHelper.GetStageFailureMessage(
        extra_info=DEFAULT_BUILD_SCRIPT_FAILURE_EXTRA_INFO)

    self.assertEqual(failure_message.extra_info['shortname'],
                     "./security_test_image")

  def testDecodeExtraInfoWithNoneInvalidExtraInfo(self):
    """Test _DecodeExtraInfo with invalid extra_info."""
    failure_message = FailureMessageHelper.GetStageFailureMessage(
        extra_info='{\"shortname\": \"./security_test_image\"')

    self.assertEqual(failure_message.extra_info, {})

  def testExtractStagePrefixName(self):
    """Test _ExtractStagePrefixName."""
    prefix_name_map = {
        'HWTest [bvt-arc]': 'HWTest',
        'HWTest': 'HWTest',
        'ImageTest': 'ImageTest',
        'ImageTest [amd64-generic]': 'ImageTest',
        'VMTest (attempt 1)': 'VMTest',
        'VMTest [amd64-generic] (attempt 1)': 'VMTest'
    }

    for k, v in prefix_name_map.iteritems():
      failure_message = FailureMessageHelper.GetStageFailureMessage(
          stage_name=k)
      self.assertEqual(failure_message.stage_prefix_name, v)


class BuildScriptFailureMessageTests(cros_test_lib.TestCase):
  """Tests for BuildScriptFailureMessage."""

  def testGetShortname(self):
    """Test GetShortname."""
    failure_message = FailureMessageHelper.GetBuildScriptFailureMessage()

    self.assertEqual(failure_message.GetShortname(), "./security_test_image")


class PackageBuildFailureMessageTests(cros_test_lib.TestCase):
  """Tests for PackageBuildFailureMessage."""

  def testGetShortname(self):
    """Test GetShortname."""
    failure_message = FailureMessageHelper.GetPackageBuildFailureMessage()

    self.assertEqual(failure_message.GetShortname(),
                     "./build_image")

  def testGetFailedPackages(self):
    """Test GetFailedPackages."""
    failure_message = FailureMessageHelper.GetPackageBuildFailureMessage()

    self.assertItemsEqual(
        ['chromeos-base/chromeos-chrome', 'chromeos-base/chromeos-init'],
        failure_message.GetFailedPackages())


class CompoundFailureMessageTests(cros_test_lib.TestCase):
  """Tests for CompoundFailureMessage."""

  def testGetCompoundFailureMessage(self):
    """Test GetCompoundFailureMessage."""
    failure_message = FailureMessageHelper.GetPackageBuildFailureMessage()
    new_failure_message = (
        failure_message_lib.CompoundFailureMessage.GetFailureMessage(
            failure_message))
    self.assertEqual(new_failure_message.inner_failures, [])

  def testHasEmptyList(self):
    """Test HasEmptyList."""
    failure_message = FailureMessageHelper.GetCompoundFailureMessage()
    self.assertTrue(failure_message.HasEmptyList())

    failure_message.inner_failures.append(
        FailureMessageHelper.GetPackageBuildFailureMessage())
    self.assertFalse(failure_message.HasEmptyList())

  def testHasExceptionCategories(self):
    """Test HasExceptionCategories."""
    failure_message = FailureMessageHelper.GetCompoundFailureMessage()
    self.assertFalse(failure_message.HasExceptionCategories(
        {constants.EXCEPTION_CATEGORY_BUILD}))

    failure_message.inner_failures.append(
        FailureMessageHelper.GetPackageBuildFailureMessage())
    self.assertTrue(failure_message.HasExceptionCategories(
        {constants.EXCEPTION_CATEGORY_BUILD}))
    self.assertFalse(failure_message.HasExceptionCategories(
        {constants.EXCEPTION_CATEGORY_LAB}))
    self.assertTrue(failure_message.HasExceptionCategories(
        {constants.EXCEPTION_CATEGORY_BUILD, constants.EXCEPTION_CATEGORY_LAB}))

  def testMatchesExceptionCategories(self):
    """Test MatchesExceptionCategories."""
    failure_message = FailureMessageHelper.GetCompoundFailureMessage()
    self.assertFalse(failure_message.MatchesExceptionCategories(
        {constants.EXCEPTION_CATEGORY_BUILD}))

    failure_message.inner_failures.append(
        FailureMessageHelper.GetPackageBuildFailureMessage())
    self.assertTrue(failure_message.MatchesExceptionCategories(
        {constants.EXCEPTION_CATEGORY_BUILD}))
    self.assertFalse(failure_message.MatchesExceptionCategories(
        {constants.EXCEPTION_CATEGORY_LAB}))
    self.assertTrue(failure_message.MatchesExceptionCategories(
        {constants.EXCEPTION_CATEGORY_BUILD, constants.EXCEPTION_CATEGORY_LAB}))

    failure_message.inner_failures.append(
        FailureMessageHelper.GetStageFailureMessage())
    self.assertFalse(failure_message.MatchesExceptionCategories(
        {constants.EXCEPTION_CATEGORY_BUILD}))
    self.assertFalse(failure_message.MatchesExceptionCategories(
        {constants.EXCEPTION_CATEGORY_LAB}))


class FailureMessageManagerTests(cros_test_lib.TestCase):
  """Tests for FailureMessageManager."""

  def testCreateMessageForBuildScriptFailureMessage(self):
    """Test CreateMessage for BuildScriptFailureMessage."""
    stage_failure = StageFailureHelper.GetStageFailure(
        1, 1, 0, 'BuildScriptFailure', 'exception msg', 'build',
        '{\"shortname\": \"./security_test_image\"', 'InitSDK')
    failure_message = failure_message_lib.FailureMessageManager.CreateMessage(
        stage_failure)

    self.assertTrue(isinstance(failure_message,
                               failure_message_lib.BuildScriptFailureMessage))

  def testCreateMessageForPackageBuildFailureMessage(self):
    """Test CreateMessage for PackageBuildFailureMessage."""
    stage_failure = StageFailureHelper.GetStageFailure(
        1, 1, 0, 'PackageBuildFailure', 'exception msg', 'build',
        DEFAULT_PACKAGE_BUILD_FAILURE_EXTRA_INFO, 'BuildImage')
    failure_message = failure_message_lib.FailureMessageManager.CreateMessage(
        stage_failure)

    self.assertTrue(isinstance(failure_message,
                               failure_message_lib.PackageBuildFailureMessage))

  def testCreateMessageForStageFailureMessage(self):
    """Test CreateMessage for StageFailureMessage."""
    stage_failure = StageFailureHelper.GetStageFailure(
        1, 1, None, 'TestFailure', 'exception msg', 'build', None, 'VMTest')
    failure_message = failure_message_lib.FailureMessageManager.CreateMessage(
        stage_failure)

    self.assertTrue(isinstance(failure_message,
                               failure_message_lib.StageFailureMessage))

  def testReconstructMessagesOnMixedMsgs(self):
    """Test ReconstructMessages on mixed messages."""
    failures = FailureMessageHelper.GetBuildFailureMessageWithMixedMsgs()

    self.assertEqual(len(failures), 4)
    failure_ids = [f.failure_id for f in failures]
    self.assertItemsEqual(failure_ids, [4, 5, 6, 7])

    for f in failures:
      if f.failure_id == 4:
        self.assertTrue(isinstance(
            f, failure_message_lib.CompoundFailureMessage))
        inner_failures = f.inner_failures
        inner_failure_ids = [n_f.failure_id for n_f in inner_failures]
        self.assertItemsEqual([1, 2, 3], inner_failure_ids)

  def testReconstructMessagesOnEmptyMsgs(self):
    """Test ReconstructMessages on empty messages."""
    failures = failure_message_lib.FailureMessageManager.ReconstructMessages([])

    self.assertEqual(failures, [])

  def testConstructStageFailureMessagesOnNonCompoundFailures(self):
    """Test ConstructStageFailureMessages on non-compound failures."""
    entry_1 = StageFailureHelper.GetStageFailure(failure_id=1)
    entry_2 = StageFailureHelper.GetStageFailure(failure_id=2)
    entry_3 = StageFailureHelper.GetStageFailure(failure_id=3)
    failure_entries = [entry_1, entry_2, entry_3]

    failures = (
        failure_message_lib.FailureMessageManager.ConstructStageFailureMessages(
            failure_entries))

    self.assertEqual(len(failures), 3)
    failure_ids = [f.failure_id for f in failures]
    self.assertItemsEqual(failure_ids, [1, 2, 3])

  def testConstructStageFailureMessagesOnCompoundFailures(self):
    """Test ConstructStageFailureMessages on compound failures."""
    entry_1 = StageFailureHelper.GetStageFailure(failure_id=1)
    entry_2 = StageFailureHelper.GetStageFailure(
        failure_id=2, outer_failure_id=1)
    entry_3 = StageFailureHelper.GetStageFailure(
        failure_id=3, outer_failure_id=1)
    failure_entries = [entry_1, entry_2, entry_3]

    failures = (
        failure_message_lib.FailureMessageManager.ConstructStageFailureMessages(
            failure_entries))

    self.assertEqual(len(failures), 1)
    f = failures[0]
    self.assertEqual(f.failure_id, 1)
    self.assertTrue(isinstance(f, failure_message_lib.CompoundFailureMessage))
    inner_failure_ids = [n_f.failure_id for n_f in f.inner_failures]
    self.assertItemsEqual([2, 3], inner_failure_ids)
