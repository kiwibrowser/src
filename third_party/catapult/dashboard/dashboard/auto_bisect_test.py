# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import mock

from dashboard import auto_bisect
from dashboard.common import namespaced_stored_object
from dashboard.common import request_handler
from dashboard.common import testing_common
from dashboard.common import utils
from dashboard.models import anomaly
from dashboard.models import try_job


class StartNewBisectForBugTest(testing_common.TestCase):

  def setUp(self):
    super(StartNewBisectForBugTest, self).setUp()
    self.SetCurrentUser('internal@chromium.org')
    namespaced_stored_object.Set('bot_configurations', {
        'linux-pinpoint': {},
    })

  @mock.patch.object(auto_bisect.start_try_job, 'PerformBisect')
  def testStartNewBisectForBug_StartsBisect(self, mock_perform_bisect):
    testing_common.AddTests(
        ['ChromiumPerf'], ['linux-release'], {'sunspider': {'score': {
            'page_1': {}, 'page_2': {}}}})
    test_key = utils.TestKey('ChromiumPerf/linux-release/sunspider/score')
    anomaly.Anomaly(
        bug_id=111, test=test_key,
        start_revision=300100, end_revision=300200,
        median_before_anomaly=100, median_after_anomaly=200).put()
    auto_bisect.StartNewBisectForBug(111)
    job = try_job.TryJob.query(try_job.TryJob.bug_id == 111).get()
    self.assertNotIn('--story-filter', job.config)
    mock_perform_bisect.assert_called_once_with(job)

  @mock.patch.object(auto_bisect.start_try_job, 'PerformBisect')
  def testStartNewBisectForBug_StartsBisectWithStoryFilter(
      self, mock_perform_bisect):
    testing_common.AddTests(
        ['ChromiumPerf'], ['linux-release'], {'sunspider': {'score': {
            'page_1': {}, 'page_2': {}}}})
    test_key = utils.TestKey(
        'ChromiumPerf/linux-release/sunspider/score/page_2')
    anomaly.Anomaly(
        bug_id=111, test=test_key,
        start_revision=300100, end_revision=300200,
        median_before_anomaly=100, median_after_anomaly=200).put()
    auto_bisect.StartNewBisectForBug(111)
    job = try_job.TryJob.query(try_job.TryJob.bug_id == 111).get()
    self.assertIn('--story-filter', job.config)
    mock_perform_bisect.assert_called_once_with(job)

  def testStartNewBisectForBug_RevisionTooLow_ReturnsError(self):
    testing_common.AddTests(
        ['ChromiumPerf'], ['linux-release'], {'sunspider': {'score': {}}})
    test_key = utils.TestKey('ChromiumPerf/linux-release/sunspider/score')
    anomaly.Anomaly(
        bug_id=222, test=test_key,
        start_revision=1200, end_revision=1250,
        median_before_anomaly=100, median_after_anomaly=200).put()
    result = auto_bisect.StartNewBisectForBug(222)
    self.assertEqual({'error': 'Invalid "good" revision: 1199.'}, result)

  def testStartNewBisectForBug_RevisionsEqual_ReturnsError(self):
    testing_common.AddTests(
        ['ChromiumPerf'], ['linux-release'], {'sunspider': {'score': {}}})
    test_key = utils.TestKey('ChromiumPerf/linux-release/sunspider/score')
    testing_common.AddRows(
        'ChromiumPerf/linux-release/sunspider/score',
        {
            11990: {
                'a_default_rev': 'r_foo',
                'r_foo': '9e29b5bcd08357155b2859f87227d50ed60cf857'
            },
            12500: {
                'a_default_rev': 'r_foo',
                'r_foo': 'fc34e5346446854637311ad7793a95d56e314042'
            }
        })
    anomaly.Anomaly(
        bug_id=222, test=test_key,
        start_revision=12500, end_revision=12500,
        median_before_anomaly=100, median_after_anomaly=200).put()
    result = auto_bisect.StartNewBisectForBug(222)
    self.assertEqual(
        {'error': 'Same "good"/"bad" revisions, bisect skipped'}, result)

  @mock.patch.object(
      auto_bisect.start_try_job, 'PerformBisect',
      mock.MagicMock(side_effect=request_handler.InvalidInputError(
          'Some reason')))
  def testStartNewBisectForBug_InvalidInputErrorRaised_ReturnsError(self):
    testing_common.AddTests(['Foo'], ['bar'], {'sunspider': {'score': {}}})
    test_key = utils.TestKey('Foo/bar/sunspider/score')
    anomaly.Anomaly(
        bug_id=345, test=test_key,
        start_revision=300100, end_revision=300200,
        median_before_anomaly=100, median_after_anomaly=200).put()
    result = auto_bisect.StartNewBisectForBug(345)
    self.assertEqual({'error': 'Some reason'}, result)

  @mock.patch.object(auto_bisect.start_try_job, 'PerformBisect')
  def testStartNewBisectForBug_WithDefaultRevs_StartsBisect(
      self, mock_perform_bisect):
    testing_common.AddTests(
        ['ChromiumPerf'], ['linux-release'], {'sunspider': {'score': {}}})
    test_key = utils.TestKey('ChromiumPerf/linux-release/sunspider/score')
    testing_common.AddRows(
        'ChromiumPerf/linux-release/sunspider/score',
        {
            11990: {
                'a_default_rev': 'r_foo',
                'r_foo': '9e29b5bcd08357155b2859f87227d50ed60cf857'
            },
            12500: {
                'a_default_rev': 'r_foo',
                'r_foo': 'fc34e5346446854637311ad7793a95d56e314042'
            }
        })
    anomaly.Anomaly(
        bug_id=333, test=test_key,
        start_revision=12000, end_revision=12500,
        median_before_anomaly=100, median_after_anomaly=200).put()
    auto_bisect.StartNewBisectForBug(333)
    job = try_job.TryJob.query(try_job.TryJob.bug_id == 333).get()
    mock_perform_bisect.assert_called_once_with(job)

  def testStartNewBisectForBug_UnbisectableTest_ReturnsError(self):
    testing_common.AddTests(['V8'], ['x86'], {'v8': {'sunspider': {}}})
    # The test suite "v8" is in the black-list of test suite names.
    test_key = utils.TestKey('V8/x86/v8/sunspider')
    anomaly.Anomaly(
        bug_id=444, test=test_key,
        start_revision=155000, end_revision=155100,
        median_before_anomaly=100, median_after_anomaly=200).put()
    result = auto_bisect.StartNewBisectForBug(444)
    self.assertEqual({'error': 'Could not select a test.'}, result)

  @mock.patch.object(
      utils, 'IsValidSheriffUser', mock.MagicMock(return_value=True))
  @mock.patch.object(
      auto_bisect.pinpoint_service, 'NewJob',
      mock.MagicMock(
          return_value={'jobId': 123, 'jobUrl': 'http://pinpoint/123'}))
  @mock.patch.object(
      auto_bisect.start_try_job, 'GuessStoryFilter')
  @mock.patch.object(auto_bisect.pinpoint_request, 'ResolveToGitHash',
                     mock.MagicMock(return_value='abc123'))
  def testStartNewBisectForBug_Pinpoint_Succeeds(self, mock_guess):
    namespaced_stored_object.Set('bot_configurations', {
        'linux-pinpoint': {
            'dimensions': [{'key': 'foo', 'value': 'bar'}]
        },
    })

    namespaced_stored_object.Set('repositories', {
        'chromium': {'some': 'params'},
    })

    testing_common.AddTests(
        ['ChromiumPerf'], ['linux-pinpoint'], {'sunspider': {'score': {}}})
    test_key = utils.TestKey('ChromiumPerf/linux-pinpoint/sunspider/score')
    testing_common.AddRows(
        'ChromiumPerf/linux-pinpoint/sunspider/score',
        {
            11999: {
                'a_default_rev': 'r_chromium',
                'r_chromium': '9e29b5bcd08357155b2859f87227d50ed60cf857'
            },
            12500: {
                'a_default_rev': 'r_chromium',
                'r_chromium': 'fc34e5346446854637311ad7793a95d56e314042'
            }
        })
    a = anomaly.Anomaly(
        bug_id=333, test=test_key,
        start_revision=12000, end_revision=12500,
        median_before_anomaly=100, median_after_anomaly=200).put()
    result = auto_bisect.StartNewBisectForBug(333)
    self.assertEqual(
        {'issue_id': 123, 'issue_url': 'http://pinpoint/123'}, result)
    mock_guess.assert_called_once_with(
        'ChromiumPerf/linux-pinpoint/sunspider/score')
    self.assertEqual('123', a.get().pinpoint_bisects[0])

  @mock.patch.object(
      auto_bisect.pinpoint_request, 'PinpointParamsFromBisectParams',
      mock.MagicMock(
          side_effect=auto_bisect.pinpoint_request.InvalidParamsError(
              'Some reason')))
  def testStartNewBisectForBug_Pinpoint_ParamsRaisesError(self):
    testing_common.AddTests(
        ['ChromiumPerf'], ['linux-pinpoint'], {'sunspider': {'score': {}}})
    test_key = utils.TestKey('ChromiumPerf/linux-pinpoint/sunspider/score')
    testing_common.AddRows(
        'ChromiumPerf/linux-pinpoint/sunspider/score',
        {
            11999: {
                'r_foo': '9e29b5bcd08357155b2859f87227d50ed60cf857'
            },
            12500: {
                'r_foo': 'fc34e5346446854637311ad7793a95d56e314042'
            }
        })
    anomaly.Anomaly(
        bug_id=333, test=test_key,
        start_revision=12000, end_revision=12501,
        median_before_anomaly=100, median_after_anomaly=200).put()
    result = auto_bisect.StartNewBisectForBug(333)
    self.assertEqual(
        {'error': 'Some reason'}, result)


if __name__ == '__main__':
  unittest.main()
