# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import mock
import unittest

from google.appengine.api import taskqueue

from dashboard.common import testing_common
from dashboard.pinpoint.models import results2


_ATTEMPT_DATA = {
    "executions": [{"result_arguments": {
        "isolate_server": "https://isolateserver.appspot.com",
        "isolate_hash": "e26a40a0d4",
    }}]
}


_JOB_NO_DIFFERENCES = {
    "state": [
        {
            "attempts": [_ATTEMPT_DATA],
            "change": {},
            "comparisons": {'next': 'same'},
        },
        {
            "attempts": [_ATTEMPT_DATA],
            "change": {},
            "comparisons": {'next': 'same', 'prev': 'same'},
        },
        {
            "attempts": [_ATTEMPT_DATA],
            "change": {},
            "comparisons": {'next': 'same', 'prev': 'same'},
        },
        {
            "attempts": [_ATTEMPT_DATA],
            "change": {},
            "comparisons": {'prev': 'same'},
        },
    ],
    "quests": ["Test"],
}


_JOB_WITH_DIFFERENCES = {
    "state": [
        {
            "attempts": [_ATTEMPT_DATA],
            "change": {},
            "comparisons": {'next': 'same'},
        },
        {
            "attempts": [_ATTEMPT_DATA],
            "change": {},
            "comparisons": {'prev': 'same', 'next': 'different'},
        },
        {
            "attempts": [_ATTEMPT_DATA],
            "change": {},
            "comparisons": {'prev': 'different', 'next': 'different'},
        },
        {
            "attempts": [_ATTEMPT_DATA],
            "change": {},
            "comparisons": {'prev': 'different'},
        },
    ],
    "quests": ["Test"],
}


_JOB_MISSING_EXECUTIONS = {
    "state": [
        {
            "attempts": [_ATTEMPT_DATA, {"executions": []}],
            "change": {},
            "comparisons": {'next': 'same'},
        },
        {
            "attempts": [{"executions": []}, _ATTEMPT_DATA],
            "change": {},
            "comparisons": {'prev': 'same'},
        },
    ],
    "quests": ["Test"],
}


@mock.patch.object(results2.cloudstorage, 'listbucket')
class GetCachedResults2Test(unittest.TestCase):

  def testGetCachedResults2_Cached_ReturnsResult(self, mock_cloudstorage):
    mock_cloudstorage.return_value = ['foo']

    job = _JobStub(_JOB_WITH_DIFFERENCES, '123')
    url = results2.GetCachedResults2(job)

    self.assertEqual(
        'https://storage.cloud.google.com/results2-public/'
        '%s.html' % job.job_id,
        url)

  @mock.patch.object(results2, 'ScheduleResults2Generation', mock.MagicMock())
  def testGetCachedResults2_Uncached_Fails(
      self, mock_cloudstorage):
    mock_cloudstorage.return_value = []

    job = _JobStub(_JOB_WITH_DIFFERENCES, '123')
    url = results2.GetCachedResults2(job)

    self.assertIsNone(url)


class ScheduleResults2Generation2Test(unittest.TestCase):

  @mock.patch.object(results2.taskqueue, 'add',
                     mock.MagicMock(side_effect=taskqueue.TombstonedTaskError))
  def testScheduleResults2Generation2_FailedPreviously(self):
    job = _JobStub(_JOB_WITH_DIFFERENCES, '123')
    result = results2.ScheduleResults2Generation(job)

    self.assertFalse(result)

  @mock.patch.object(
      results2.taskqueue, 'add',
      mock.MagicMock(side_effect=taskqueue.TaskAlreadyExistsError))
  def testScheduleResults2Generation2_AlreadyRunning(self):
    job = _JobStub(_JOB_WITH_DIFFERENCES, '123')
    result = results2.ScheduleResults2Generation(job)

    self.assertTrue(result)


@mock.patch.object(results2, 'open', mock.mock_open(read_data='fake_viewer'),
                   create=True)
class GenerateResults2Test(testing_common.TestCase):

  @mock.patch.object(results2, '_FetchHistogramsDataFromJobData',
                     mock.MagicMock(return_value=['a', 'b']))
  @mock.patch.object(results2, '_GcsFileStream', mock.MagicMock())
  @mock.patch.object(results2.render_histograms_viewer,
                     'RenderHistogramsViewer')
  def testPost_Renders(self, mock_render):
    job = _JobStub(_JOB_NO_DIFFERENCES, '123')
    results2.GenerateResults2(job)

    mock_render.assert_called_with(
        ['a', 'b'], mock.ANY, reset_results=True, vulcanized_html='fake_viewer')

    results = results2.CachedResults2.query().fetch()
    self.assertEqual(1, len(results))


@mock.patch.object(results2.read_value, '_RetrieveOutputJson',
                   mock.MagicMock(return_value=['a']))
class FetchHistogramsTest(unittest.TestCase):
  def testGet_WithNoDifferences(self):
    job = _JobStub(_JOB_NO_DIFFERENCES, '123')
    fetch = results2._FetchHistogramsDataFromJobData(job)
    self.assertEqual(['a', 'a', 'a', 'a'], [f for f in fetch])

  def testGet_WithDifferences(self):
    job = _JobStub(_JOB_WITH_DIFFERENCES, '123')
    fetch = results2._FetchHistogramsDataFromJobData(job)
    self.assertEqual(['a', 'a', 'a'], [f for f in fetch])

  def testGet_MissingExecutions(self):
    job = _JobStub(_JOB_MISSING_EXECUTIONS, '123')
    fetch = results2._FetchHistogramsDataFromJobData(job)
    self.assertEqual(['a', 'a'], [f for f in fetch])


class _JobStub(object):

  def __init__(self, job_dict, job_id):
    self._job_dict = job_dict
    self.job_id = job_id

  def AsDict(self, options=None):
    del options
    return self._job_dict
