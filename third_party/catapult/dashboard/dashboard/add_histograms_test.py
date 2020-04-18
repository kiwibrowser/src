# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import base64
import json
import mock
import sys
import webapp2
import webtest
import zlib

from google.appengine.api import users

from dashboard import add_point_queue
from dashboard import add_histograms
from dashboard import add_histograms_queue
from dashboard.api import api_auth
from dashboard.api import api_request_handler
from dashboard.common import stored_object
from dashboard.common import testing_common
from dashboard.common import utils
from dashboard.models import graph_data
from dashboard.models import histogram
from dashboard.models import sheriff
from tracing.value import histogram as histogram_module
from tracing.value import histogram_set
from tracing.value.diagnostics import breakdown
from tracing.value.diagnostics import generic_set
from tracing.value.diagnostics import reserved_infos

# pylint: disable=too-many-lines


GOOGLER_USER = users.User(email='authorized@chromium.org',
                          _auth_domain='google.com')


def SetGooglerOAuth(mock_oauth):
  mock_oauth.get_current_user.return_value = GOOGLER_USER
  mock_oauth.get_client_id.return_value = api_auth.OAUTH_CLIENT_ID_WHITELIST[0]


def _CreateHistogram(
    name='hist', master=None, bot=None, benchmark=None,
    device=None, owner=None, stories=None, story_tags=None,
    benchmark_description=None, commit_position=None,
    samples=None, max_samples=None, is_ref=False, is_summary=None):
  hists = [histogram_module.Histogram(name, 'count')]
  if max_samples:
    hists[0].max_num_sample_values = max_samples
  if samples:
    for s in samples:
      hists[0].AddSample(s)

  histograms = histogram_set.HistogramSet(hists)
  if master:
    histograms.AddSharedDiagnostic(
        reserved_infos.MASTERS.name,
        generic_set.GenericSet([master]))
  if bot:
    histograms.AddSharedDiagnostic(
        reserved_infos.BOTS.name,
        generic_set.GenericSet([bot]))
  if commit_position:
    histograms.AddSharedDiagnostic(
        reserved_infos.CHROMIUM_COMMIT_POSITIONS.name,
        generic_set.GenericSet([commit_position]))
  if benchmark:
    histograms.AddSharedDiagnostic(
        reserved_infos.BENCHMARKS.name,
        generic_set.GenericSet([benchmark]))
  if benchmark_description:
    histograms.AddSharedDiagnostic(
        reserved_infos.BENCHMARK_DESCRIPTIONS.name,
        generic_set.GenericSet([benchmark_description]))
  if owner:
    histograms.AddSharedDiagnostic(
        reserved_infos.OWNERS.name,
        generic_set.GenericSet([owner]))
  if device:
    histograms.AddSharedDiagnostic(
        reserved_infos.DEVICE_IDS.name,
        generic_set.GenericSet([device]))
  if stories:
    histograms.AddSharedDiagnostic(
        reserved_infos.STORIES.name,
        generic_set.GenericSet(stories))
  if story_tags:
    histograms.AddSharedDiagnostic(
        reserved_infos.STORY_TAGS.name,
        generic_set.GenericSet(story_tags))
  if is_ref:
    histograms.AddSharedDiagnostic(
        reserved_infos.IS_REFERENCE_BUILD.name,
        generic_set.GenericSet([True]))
  if is_summary is not None:
    histograms.AddSharedDiagnostic(
        reserved_infos.SUMMARY_KEYS.name,
        generic_set.GenericSet(is_summary))
  return histograms


class AddHistogramsEndToEndTest(testing_common.TestCase):

  def setUp(self):
    super(AddHistogramsEndToEndTest, self).setUp()
    app = webapp2.WSGIApplication([
        ('/add_histograms', add_histograms.AddHistogramsHandler),
        ('/add_histograms_queue',
         add_histograms_queue.AddHistogramsQueueHandler)])
    self.testapp = webtest.TestApp(app)
    testing_common.SetIsInternalUser('foo@bar.com', True)
    self.SetCurrentUser('foo@bar.com', is_admin=True)
    oauth_patcher = mock.patch.object(api_auth, 'oauth')
    self.addCleanup(oauth_patcher.stop)
    mock_oauth = oauth_patcher.start()
    SetGooglerOAuth(mock_oauth)

  @mock.patch.object(
      add_histograms_queue.graph_revisions, 'AddRowsToCacheAsync')
  @mock.patch.object(add_histograms_queue.find_anomalies, 'ProcessTestsAsync')
  def testPost_Succeeds(self, mock_process_test, mock_graph_revisions):
    hs = _CreateHistogram(
        master='master', bot='bot', benchmark='benchmark', commit_position=123,
        benchmark_description='Benchmark description.', samples=[1, 2, 3])
    data = json.dumps(hs.AsDicts())
    sheriff.Sheriff(
        id='my_sheriff1', email='a@chromium.org', patterns=[
            '*/*/*/hist', '*/*/*/hist_avg']).put()

    self.testapp.post('/add_histograms', {'data': data})
    self.ExecuteTaskQueueTasks('/add_histograms_queue',
                               add_histograms.TASK_QUEUE_NAME)
    diagnostics = histogram.SparseDiagnostic.query().fetch()
    self.assertEqual(4, len(diagnostics))
    histograms = histogram.Histogram.query().fetch()
    self.assertEqual(1, len(histograms))

    tests = graph_data.TestMetadata.query().fetch()

    self.assertEqual('Benchmark description.', tests[0].description)

    # Verify that an anomaly processing was called.
    mock_process_test.assert_called_once_with([tests[1].key, tests[2].key])

    rows = graph_data.Row.query().fetch()
    # We want to verify that the method was called with all rows that have
    # been added, but the ordering will be different because we produce
    # the rows by iterating over a dict.
    mock_graph_revisions.assert_called_once()
    self.assertEqual(len(mock_graph_revisions.mock_calls[0][1][0]), len(rows))

  @mock.patch.object(
      add_histograms_queue.graph_revisions, 'AddRowsToCacheAsync')
  @mock.patch.object(add_histograms_queue.find_anomalies, 'ProcessTestsAsync')
  def testPost_ZlibSucceeds(self, mock_process_test, mock_graph_revisions):
    hs = _CreateHistogram(
        master='master', bot='bot', benchmark='benchmark', commit_position=123,
        benchmark_description='Benchmark description.', samples=[1, 2, 3])
    data = zlib.compress(json.dumps(hs.AsDicts()))
    sheriff.Sheriff(
        id='my_sheriff1', email='a@chromium.org', patterns=[
            '*/*/*/hist', '*/*/*/hist_avg']).put()

    self.testapp.post('/add_histograms', data)
    self.ExecuteTaskQueueTasks('/add_histograms_queue',
                               add_histograms.TASK_QUEUE_NAME)
    diagnostics = histogram.SparseDiagnostic.query().fetch()
    self.assertEqual(4, len(diagnostics))
    histograms = histogram.Histogram.query().fetch()
    self.assertEqual(1, len(histograms))

    tests = graph_data.TestMetadata.query().fetch()

    self.assertEqual('Benchmark description.', tests[0].description)

    # Verify that an anomaly processing was called.
    mock_process_test.assert_called_once_with([tests[1].key, tests[2].key])

    rows = graph_data.Row.query().fetch()
    # We want to verify that the method was called with all rows that have
    # been added, but the ordering will be different because we produce
    # the rows by iterating over a dict.
    mock_graph_revisions.assert_called_once()
    self.assertEqual(len(mock_graph_revisions.mock_calls[0][1][0]), len(rows))

  @mock.patch.object(
      add_histograms_queue.graph_revisions, 'AddRowsToCacheAsync',
      mock.MagicMock())
  @mock.patch.object(
      add_histograms_queue.find_anomalies, 'ProcessTestsAsync',
      mock.MagicMock())
  def testPost_BenchmarkTotalDuration_Filtered(self):
    hs = _CreateHistogram(
        name='benchmark_total_duration', master='master', bot='bot',
        benchmark='v8.browsing', commit_position=123, samples=[1], is_ref=True)
    data = json.dumps(hs.AsDicts())

    self.testapp.post('/add_histograms', {'data': data})
    self.ExecuteTaskQueueTasks('/add_histograms_queue',
                               add_histograms.TASK_QUEUE_NAME)
    tests = graph_data.TestMetadata.query().fetch()
    for t in tests:
      parts = t.key.id().split('/')
      if len(parts) <= 3:
        continue
      self.assertEqual('benchmark_total_duration', parts[3])

  def testPost_PurgesBinData(self):
    hs = _CreateHistogram(
        master='m', bot='b', benchmark='s', commit_position=1)
    b = breakdown.Breakdown()
    dm = histogram_module.DiagnosticMap()
    dm['breakdown'] = b
    hs.GetFirstHistogram().AddSample(0, dm)
    data = json.dumps(hs.AsDicts())

    self.testapp.post('/add_histograms', {'data': data})
    self.ExecuteTaskQueueTasks('/add_histograms_queue',
                               add_histograms.TASK_QUEUE_NAME)

    histograms = histogram.Histogram.query().fetch()
    hist = histogram_module.Histogram.FromDict(histograms[0].data)
    for b in hist.bins:
      for dm in b.diagnostic_maps:
        self.assertEqual(0, len(dm))

  def testPost_IllegalMasterName_Fails(self):
    hs = _CreateHistogram(
        master='m/m', bot='b', benchmark='s', commit_position=1, samples=[1])
    data = json.dumps(hs.AsDicts())

    response = self.testapp.post('/add_histograms', {'data': data}, status=400)
    self.assertIn('Illegal slash', response.body)

  def testPost_IllegalBotName_Fails(self):
    hs = _CreateHistogram(
        master='m', bot='b/b', benchmark='s', commit_position=1, samples=[1])
    data = json.dumps(hs.AsDicts())

    response = self.testapp.post('/add_histograms', {'data': data}, status=400)
    self.assertIn('Illegal slash', response.body)

  def testPost_IllegalSuiteName_Fails(self):
    hs = _CreateHistogram(
        master='m', bot='b', benchmark='s/s', commit_position=1, samples=[1])
    data = json.dumps(hs.AsDicts())

    response = self.testapp.post('/add_histograms', {'data': data}, status=400)
    self.assertIn('Illegal slash', response.body)

  @mock.patch.object(
      add_histograms_queue.graph_revisions, 'AddRowsToCacheAsync')
  @mock.patch.object(add_histograms_queue.find_anomalies, 'ProcessTestsAsync')
  @mock.patch.object(
      add_histograms_queue, 'CreateRowEntities',
      mock.MagicMock(return_value=None))
  def testPost_EmptyHistogram_NotAdded(
      self, mock_process_test, mock_graph_revisions):
    hs = _CreateHistogram(
        master='m', bot='b', benchmark='s', name='foo', commit_position=1,
        samples=[])
    data = json.dumps(hs.AsDicts())

    sheriff.Sheriff(
        id='my_sheriff1', email='a@chromium.org', patterns=['*/*/*/foo2']).put()

    self.testapp.post('/add_histograms', {'data': data})
    self.ExecuteTaskQueueTasks('/add_histograms_queue',
                               add_histograms.TASK_QUEUE_NAME)

    rows = graph_data.Row.query().fetch()
    self.assertEqual(0, len(rows))

    histograms = histogram.Histogram.query().fetch()
    self.assertEqual(0, len(histograms))

    self.assertFalse(mock_process_test.called)
    self.assertFalse(mock_graph_revisions.called)

  @mock.patch.object(add_histograms_queue.find_anomalies, 'ProcessTestsAsync')
  def testPost_TestNameEndsWithUnderscoreRef_ProcessTestIsNotCalled(
      self, mock_process_test):
    """Tests that Tests ending with "_ref" aren't analyzed for Anomalies."""
    sheriff.Sheriff(
        id='ref_sheriff', email='a@chromium.org', patterns=['*/*/*/*']).put()
    hs = _CreateHistogram(
        master='master', bot='bot', benchmark='benchmark',
        commit_position=424242, stories=['abcd'], samples=[1, 2, 3],
        is_ref=True)
    data = json.dumps(hs.AsDicts())
    self.testapp.post('/add_histograms', {'data': data})
    self.ExecuteTaskQueueTasks('/add_histograms_queue',
                               add_histograms.TASK_QUEUE_NAME)
    mock_process_test.assert_called_once_with([])

  @mock.patch.object(add_histograms_queue.find_anomalies, 'ProcessTestsAsync')
  def testPost_TestNameEndsWithSlashRef_ProcessTestIsNotCalled(
      self, mock_process_test):
    """Tests that leaf tests named ref aren't added to the task queue."""
    sheriff.Sheriff(
        id='ref_sheriff', email='a@chromium.org', patterns=['*/*/*/*']).put()
    hs = _CreateHistogram(
        master='master', bot='bot', benchmark='benchmark',
        commit_position=424242, stories=['ref'], samples=[1, 2, 3])
    data = json.dumps(hs.AsDicts())
    self.testapp.post('/add_histograms', {'data': data})
    self.ExecuteTaskQueueTasks('/add_histograms_queue',
                               add_histograms.TASK_QUEUE_NAME)
    mock_process_test.assert_called_once_with([])

  @mock.patch.object(add_histograms_queue.find_anomalies, 'ProcessTestsAsync')
  def testPost_TestNameEndsContainsButDoesntEndWithRef_ProcessTestIsCalled(
      self, mock_process_test):
    sheriff.Sheriff(
        id='ref_sheriff', email='a@chromium.org', patterns=['*/*/*/*']).put()
    hs = _CreateHistogram(
        master='master', bot='bot', benchmark='benchmark',
        commit_position=424242, stories=['_ref_abcd'], samples=[1, 2, 3])
    data = json.dumps(hs.AsDicts())
    self.testapp.post('/add_histograms', {'data': data})
    self.ExecuteTaskQueueTasks('/add_histograms_queue',
                               add_histograms.TASK_QUEUE_NAME)
    self.assertTrue(mock_process_test.called)

  @mock.patch.object(
      add_histograms_queue.graph_revisions, 'AddRowsToCacheAsync',
      mock.MagicMock())
  @mock.patch.object(
      add_histograms_queue.find_anomalies, 'ProcessTestsAsync',
      mock.MagicMock())
  def testPost_DeduplicateByName(self):
    hs = _CreateHistogram(
        master='m', bot='b', benchmark='s', stories=['s1', 's2'],
        commit_position=1111, device='device1', owner='owner1', samples=[42])
    self.testapp.post(
        '/add_histograms', {'data': json.dumps(hs.AsDicts())})
    self.ExecuteTaskQueueTasks('/add_histograms_queue',
                               add_histograms.TASK_QUEUE_NAME)

    # There should be the 4 suite level and 2 histogram level diagnostics
    diagnostics = histogram.SparseDiagnostic.query().fetch()
    self.assertEqual(6, len(diagnostics))

    hs = _CreateHistogram(
        master='m', bot='b', benchmark='s', stories=['s1', 's2'],
        commit_position=1112, device='device1', owner='owner1', samples=[42])
    self.testapp.post(
        '/add_histograms', {'data': json.dumps(hs.AsDicts())})
    self.ExecuteTaskQueueTasks('/add_histograms_queue',
                               add_histograms.TASK_QUEUE_NAME)

    # There should STILL be the 4 suite level and 2 histogram level diagnostics
    diagnostics = histogram.SparseDiagnostic.query().fetch()
    self.assertEqual(6, len(diagnostics))

    hs = _CreateHistogram(
        master='m', bot='b', benchmark='s', stories=['s1', 's2'],
        commit_position=1113, device='device2', owner='owner1', samples=[42])
    self.testapp.post(
        '/add_histograms', {'data': json.dumps(hs.AsDicts())})
    self.ExecuteTaskQueueTasks('/add_histograms_queue',
                               add_histograms.TASK_QUEUE_NAME)

    # There should one additional device diagnostic since that changed
    diagnostics = histogram.SparseDiagnostic.query().fetch()
    self.assertEqual(7, len(diagnostics))

    hs = _CreateHistogram(
        master='m', bot='b', benchmark='s', stories=['s1', 's2'],
        commit_position=1114, device='device2', owner='owner2')
    self.testapp.post(
        '/add_histograms', {'data': json.dumps(hs.AsDicts())})
    self.ExecuteTaskQueueTasks('/add_histograms_queue',
                               add_histograms.TASK_QUEUE_NAME)

    # Now there's an additional owner diagnostic
    diagnostics = histogram.SparseDiagnostic.query().fetch()
    self.assertEqual(8, len(diagnostics))

    hs = _CreateHistogram(
        master='m', bot='b', benchmark='s', stories=['s1', 's2'],
        commit_position=1115, device='device2', owner='owner2', samples=[42])
    self.testapp.post(
        '/add_histograms', {'data': json.dumps(hs.AsDicts())})
    self.ExecuteTaskQueueTasks('/add_histograms_queue',
                               add_histograms.TASK_QUEUE_NAME)

    # No more new diagnostics
    diagnostics = histogram.SparseDiagnostic.query().fetch()
    self.assertEqual(8, len(diagnostics))

  @mock.patch.object(
      add_histograms_queue.graph_revisions, 'AddRowsToCacheAsync',
      mock.MagicMock())
  @mock.patch.object(
      add_histograms_queue.find_anomalies, 'ProcessTestsAsync',
      mock.MagicMock())
  def testPost_NamesAreSet(self):
    hs = _CreateHistogram(
        master='master', bot='bot', benchmark='benchmark',
        commit_position=12345, device='foo', samples=[42])

    self.testapp.post(
        '/add_histograms', {'data': json.dumps(hs.AsDicts())})
    self.ExecuteTaskQueueTasks('/add_histograms_queue',
                               add_histograms.TASK_QUEUE_NAME)

    diagnostics = histogram.SparseDiagnostic.query().fetch()
    self.assertEqual(4, len(diagnostics))

    # The first 3 are all suite level diagnostics, the last is a histogram
    # level diagnostic.
    names = [
        reserved_infos.MASTERS.name,
        reserved_infos.BOTS.name,
        reserved_infos.BENCHMARKS.name,
        reserved_infos.DEVICE_IDS.name]
    for d in diagnostics:
      self.assertIn(d.name, names)
      names.remove(d.name)

  def _TestDiagnosticsInternalOnly(self):
    hs = _CreateHistogram(
        master='master', bot='bot', benchmark='benchmark',
        commit_position=12345)

    self.testapp.post(
        '/add_histograms', {'data': json.dumps(hs.AsDicts())})
    self.ExecuteTaskQueueTasks('/add_histograms_queue',
                               add_histograms.TASK_QUEUE_NAME)

  @mock.patch.object(
      add_histograms_queue.graph_revisions, 'AddRowsToCacheAsync',
      mock.MagicMock())
  @mock.patch.object(
      add_histograms_queue.find_anomalies, 'ProcessTestsAsync',
      mock.MagicMock())
  def testPost_DiagnosticsInternalOnly_False(self):
    stored_object.Set(
        add_point_queue.BOT_WHITELIST_KEY, ['bot'])

    self._TestDiagnosticsInternalOnly()

    diagnostics = histogram.SparseDiagnostic.query().fetch()
    for d in diagnostics:
      self.assertFalse(d.internal_only)

  @mock.patch.object(
      add_histograms_queue.graph_revisions, 'AddRowsToCacheAsync',
      mock.MagicMock())
  @mock.patch.object(
      add_histograms_queue.find_anomalies, 'ProcessTestsAsync',
      mock.MagicMock())
  def testPost_DiagnosticsInternalOnly_True(self):
    stored_object.Set(
        add_point_queue.BOT_WHITELIST_KEY, ['not_in_list'])

    self._TestDiagnosticsInternalOnly()

    diagnostics = histogram.SparseDiagnostic.query().fetch()
    for d in diagnostics:
      self.assertTrue(d.internal_only)

  def testPost_SetsCorrectTestPathForSummary(self):
    histograms = _CreateHistogram(
        master='master', bot='bot', benchmark='benchmark',
        commit_position=12345, device='device_foo', stories=['story'],
        story_tags=['group:media', 'case:browse'], is_summary=['name'],
        samples=[42])

    self.testapp.post(
        '/add_histograms', {'data': json.dumps(histograms.AsDicts())})
    self.ExecuteTaskQueueTasks('/add_histograms_queue',
                               add_histograms.TASK_QUEUE_NAME)

    tests = graph_data.TestMetadata.query().fetch()

    expected = [
        'master/bot/benchmark',
        'master/bot/benchmark/hist',
        'master/bot/benchmark/hist_avg',
        'master/bot/benchmark/hist_count',
        'master/bot/benchmark/hist_max',
        'master/bot/benchmark/hist_min',
        'master/bot/benchmark/hist_std',
        'master/bot/benchmark/hist_sum',
    ]

    for test in tests:
      self.assertIn(test.key.id(), expected)
      expected.remove(test.key.id())
    self.assertEqual(0, len(expected))

  def testPost_SetsCorrectTestPathForTIRLabelSummary(self):
    histograms = _CreateHistogram(
        master='master', bot='bot', benchmark='benchmark',
        commit_position=12345, device='device_foo', stories=['story'],
        story_tags=['group:media', 'case:browse'],
        is_summary=['name', 'storyTags'], samples=[42])

    self.testapp.post(
        '/add_histograms', {'data': json.dumps(histograms.AsDicts())})
    self.ExecuteTaskQueueTasks('/add_histograms_queue',
                               add_histograms.TASK_QUEUE_NAME)

    tests = graph_data.TestMetadata.query().fetch()

    expected = [
        'master/bot/benchmark',
        'master/bot/benchmark/hist',
        'master/bot/benchmark/hist/browse_media',
        'master/bot/benchmark/hist_avg',
        'master/bot/benchmark/hist_avg/browse_media',
        'master/bot/benchmark/hist_count',
        'master/bot/benchmark/hist_count/browse_media',
        'master/bot/benchmark/hist_max',
        'master/bot/benchmark/hist_max/browse_media',
        'master/bot/benchmark/hist_min',
        'master/bot/benchmark/hist_min/browse_media',
        'master/bot/benchmark/hist_std',
        'master/bot/benchmark/hist_std/browse_media',
        'master/bot/benchmark/hist_sum',
        'master/bot/benchmark/hist_sum/browse_media',
    ]

    for test in tests:
      self.assertIn(test.key.id(), expected)
      expected.remove(test.key.id())
    self.assertEqual(0, len(expected))

  def testPost_SetsCorrectTestPathForNonSummary(self):
    histograms = _CreateHistogram(
        master='master', bot='bot', benchmark='benchmark',
        commit_position=12345, device='device_foo', stories=['story'],
        is_summary=None, samples=[42])

    self.testapp.post(
        '/add_histograms', {'data': json.dumps(histograms.AsDicts())})
    self.ExecuteTaskQueueTasks('/add_histograms_queue',
                               add_histograms.TASK_QUEUE_NAME)

    tests = [t.key.id() for t in graph_data.TestMetadata.query().fetch()]
    self.assertEqual(15, len(tests))  # suite + hist + stats + story per stat
    self.assertIn('master/bot/benchmark/hist/story', tests)

  def testPost_SetsCorrectTestPathForSummaryAbsent(self):
    histograms = _CreateHistogram(
        master='master', bot='bot', benchmark='benchmark',
        commit_position=12345, device='device_foo', stories=['story'],
        samples=[42])

    self.testapp.post(
        '/add_histograms', {'data': json.dumps(histograms.AsDicts())})
    self.ExecuteTaskQueueTasks('/add_histograms_queue',
                               add_histograms.TASK_QUEUE_NAME)

    tests = [t.key.id() for t in graph_data.TestMetadata.query().fetch()]
    self.assertEqual(15, len(tests))
    self.assertIn('master/bot/benchmark/hist/story', tests)


class AddHistogramsTest(testing_common.TestCase):

  def setUp(self):
    super(AddHistogramsTest, self).setUp()
    app = webapp2.WSGIApplication([
        ('/add_histograms', add_histograms.AddHistogramsHandler)])
    self.testapp = webtest.TestApp(app)
    testing_common.SetIsInternalUser('foo@bar.com', True)
    self.SetCurrentUser('foo@bar.com', is_admin=True)
    oauth_patcher = mock.patch.object(api_auth, 'oauth')
    self.addCleanup(oauth_patcher.stop)
    mock_oauth = oauth_patcher.start()
    SetGooglerOAuth(mock_oauth)

  def TaskParamsByGuid(self):
    tasks = self.GetTaskQueueTasks(add_histograms.TASK_QUEUE_NAME)
    params_by_guid = {}
    for task in tasks:
      params = base64.b64decode(task['body'])
      histogram_dicts = json.loads(params)
      for d in histogram_dicts:
        params_by_guid[d['data']['guid']] = d
    return params_by_guid

  @mock.patch.object(
      add_histograms, '_QueueHistogramTasks')
  def testPostHistogram_TooManyHistograms_Splits(self, mock_queue):
    def _MakeHistogram(name):
      h = histogram_module.Histogram(name, 'count')
      for i in xrange(100):
        h.AddSample(i)
      return h

    hists = [_MakeHistogram('hist_%d' % i) for i in xrange(100)]
    histograms = histogram_set.HistogramSet(hists)
    histograms.AddSharedDiagnostic(
        reserved_infos.MASTERS.name,
        generic_set.GenericSet(['master']))
    histograms.AddSharedDiagnostic(
        reserved_infos.BOTS.name,
        generic_set.GenericSet(['bot']))
    histograms.AddSharedDiagnostic(
        reserved_infos.CHROMIUM_COMMIT_POSITIONS.name,
        generic_set.GenericSet([12345]))
    histograms.AddSharedDiagnostic(
        reserved_infos.BENCHMARKS.name,
        generic_set.GenericSet(['benchmark']))
    histograms.AddSharedDiagnostic(
        reserved_infos.DEVICE_IDS.name,
        generic_set.GenericSet(['devie_foo']))

    self.testapp.post(
        '/add_histograms', {'data': json.dumps(histograms.AsDicts())})

    self.assertTrue(len(mock_queue.call_args[0][0]) > 1)

  @mock.patch.object(
      add_histograms, '_QueueHistogramTasks')
  def testPostHistogram_FewHistograms_SingleTask(self, mock_queue):
    def _MakeHistogram(name):
      h = histogram_module.Histogram(name, 'count')
      for i in xrange(100):
        h.AddSample(i)
      return h

    hists = [_MakeHistogram('hist_%d' % i) for i in xrange(50)]
    histograms = histogram_set.HistogramSet(hists)
    histograms.AddSharedDiagnostic(
        reserved_infos.MASTERS.name,
        generic_set.GenericSet(['master']))
    histograms.AddSharedDiagnostic(
        reserved_infos.BOTS.name,
        generic_set.GenericSet(['bot']))
    histograms.AddSharedDiagnostic(
        reserved_infos.CHROMIUM_COMMIT_POSITIONS.name,
        generic_set.GenericSet([12345]))
    histograms.AddSharedDiagnostic(
        reserved_infos.BENCHMARKS.name,
        generic_set.GenericSet(['benchmark']))
    histograms.AddSharedDiagnostic(
        reserved_infos.DEVICE_IDS.name,
        generic_set.GenericSet(['devie_foo']))

    self.testapp.post(
        '/add_histograms', {'data': json.dumps(histograms.AsDicts())})

    self.assertEqual(len(mock_queue.call_args[0][0]), 1)

  def testPostHistogramSetsTestPathAndRevision(self):
    data = json.dumps([
        {
            'values': ['benchmark'],
            'guid': '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae',
            'type': 'GenericSet',
        }, {
            'values': [424242],
            'guid': '25f0a111-9bb4-4cea-b0c1-af2609623160',
            'type': 'GenericSet',
        }, {
            'values': ['master'],
            'guid': 'e9c2891d-2b04-413f-8cf4-099827e67626',
            'type': 'GenericSet'
        }, {
            'values': ['bot'],
            'guid': '53fb5448-9f8d-407a-8891-e7233fe1740f',
            'type': 'GenericSet'
        }, {
            'values': ['story'],
            'guid': 'dc894bd9-0b73-4400-9d95-b21ee371031d',
            'type': 'GenericSet',
        }, {
            'binBoundaries': [1, [1, 1000, 20]],
            'diagnostics': {
                reserved_infos.STORIES.name:
                    'dc894bd9-0b73-4400-9d95-b21ee371031d',
                reserved_infos.MASTERS.name:
                    'e9c2891d-2b04-413f-8cf4-099827e67626',
                reserved_infos.BOTS.name:
                    '53fb5448-9f8d-407a-8891-e7233fe1740f',
                reserved_infos.CHROMIUM_COMMIT_POSITIONS.name:
                    '25f0a111-9bb4-4cea-b0c1-af2609623160',
                reserved_infos.BENCHMARKS.name:
                    '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae',
            },
            'guid': '4989617a-14d6-4f80-8f75-dafda2ff13b0',
            'name': 'foo',
            'unit': 'count'
        }, {
            'binBoundaries': [1, [1, 1000, 20]],
            'diagnostics': {
                reserved_infos.MASTERS.name:
                    'e9c2891d-2b04-413f-8cf4-099827e67626',
                reserved_infos.BOTS.name:
                    '53fb5448-9f8d-407a-8891-e7233fe1740f',
                reserved_infos.CHROMIUM_COMMIT_POSITIONS.name:
                    '25f0a111-9bb4-4cea-b0c1-af2609623160',
                reserved_infos.BENCHMARKS.name:
                    '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae',
                reserved_infos.STORIES.name:
                    'dc894bd9-0b73-4400-9d95-b21ee371031d',
            },
            'guid': '2a714c36-f4ef-488d-8bee-93c7e3149388',
            'name': 'foo2',
            'unit': 'count'
        }
    ])
    self.testapp.post('/add_histograms', {'data': data})
    params_by_guid = self.TaskParamsByGuid()

    self.assertEqual(2, len(params_by_guid))
    self.assertEqual(
        'master/bot/benchmark/foo/story',
        params_by_guid['4989617a-14d6-4f80-8f75-dafda2ff13b0']['test_path'])
    self.assertEqual(
        424242,
        params_by_guid['4989617a-14d6-4f80-8f75-dafda2ff13b0']['revision'])
    self.assertEqual(
        'master/bot/benchmark/foo2/story',
        params_by_guid['2a714c36-f4ef-488d-8bee-93c7e3149388']['test_path'])
    self.assertEqual(
        424242,
        params_by_guid['2a714c36-f4ef-488d-8bee-93c7e3149388']['revision'])

  def testPostHistogramPassesHistogramLevelSparseDiagnostics(self):
    data = json.dumps([
        {
            'values': ['benchmark'],
            'guid': '876d0fba-1d12-4c00-a7e9-5fed467e19e3',
            'type': 'GenericSet',
        }, {
            'values': ['test'],
            'guid': '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae',
            'type': 'GenericSet',
        }, {
            'values': [424242],
            'guid': '25f0a111-9bb4-4cea-b0c1-af2609623160',
            'type': 'GenericSet',
        }, {
            'values': ['master'],
            'guid': 'e9c2891d-2b04-413f-8cf4-099827e67626',
            'type': 'GenericSet'
        }, {
            'values': ['bot'],
            'guid': '53fb5448-9f8d-407a-8891-e7233fe1740f',
            'type': 'GenericSet'
        }, {
            'binBoundaries': [1, [1, 1000, 20]],
            'diagnostics': {
                reserved_infos.MASTERS.name:
                    'e9c2891d-2b04-413f-8cf4-099827e67626',
                reserved_infos.BOTS.name:
                    '53fb5448-9f8d-407a-8891-e7233fe1740f',
                reserved_infos.CHROMIUM_COMMIT_POSITIONS.name:
                    '25f0a111-9bb4-4cea-b0c1-af2609623160',
                reserved_infos.BENCHMARKS.name:
                    '876d0fba-1d12-4c00-a7e9-5fed467e19e3',
            },
            'guid': '4989617a-14d6-4f80-8f75-dafda2ff13b0',
            'name': 'foo',
            'unit': 'count'
        }, {
            'binBoundaries': [1, [1, 1000, 20]],
            'diagnostics': {
                reserved_infos.MASTERS.name:
                    'e9c2891d-2b04-413f-8cf4-099827e67626',
                reserved_infos.BOTS.name:
                    '53fb5448-9f8d-407a-8891-e7233fe1740f',
                reserved_infos.DEVICE_IDS.name:
                    '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae',
                reserved_infos.CHROMIUM_COMMIT_POSITIONS.name:
                    '25f0a111-9bb4-4cea-b0c1-af2609623160',
                reserved_infos.BENCHMARKS.name:
                    '876d0fba-1d12-4c00-a7e9-5fed467e19e3',
            },
            'guid': '2a714c36-f4ef-488d-8bee-93c7e3149388',
            'name': 'foo2',
            'unit': 'count'
        }
    ])
    self.testapp.post('/add_histograms', {'data': data})

    params_by_guid = self.TaskParamsByGuid()
    params = params_by_guid['2a714c36-f4ef-488d-8bee-93c7e3149388']
    diagnostics = params['diagnostics']

    self.assertEqual(1, len(diagnostics))
    self.assertEqual(
        ['test'], diagnostics[reserved_infos.DEVICE_IDS.name]['values'])
    self.assertNotEqual(
        '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae',
        diagnostics[reserved_infos.DEVICE_IDS.name]['guid'])

  def testPostHistogram_AddsNewSparseDiagnostic(self):
    diag_dict = {
        'values': ['master'],
        'guid': 'e9c2891d-2b04-413f-8cf4-099827e67626',
        'type': 'GenericSet'
    }
    diag = histogram.SparseDiagnostic(
        data=diag_dict, start_revision=1, end_revision=sys.maxint,
        test=utils.TestKey('master/bot/benchmark'))
    diag.put()
    data = json.dumps([
        {
            'values': ['benchmark'],
            'guid': '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae',
            'type': 'GenericSet',
        }, {
            'values': [424242],
            'guid': '25f0a111-9bb4-4cea-b0c1-af2609623160',
            'type': 'GenericSet',
        }, {
            'values': ['master'],
            'guid': 'e9c2891d-2b04-413f-8cf4-099827e67626',
            'type': 'GenericSet'
        }, {
            'values': ['bot'],
            'guid': '53fb5448-9f8d-407a-8891-e7233fe1740f',
            'type': 'GenericSet'
        }, {
            'binBoundaries': [1, [1, 1000, 20]],
            'diagnostics': {
                reserved_infos.MASTERS.name:
                    'e9c2891d-2b04-413f-8cf4-099827e67626',
                reserved_infos.BOTS.name:
                    '53fb5448-9f8d-407a-8891-e7233fe1740f',
                reserved_infos.CHROMIUM_COMMIT_POSITIONS.name:
                    '25f0a111-9bb4-4cea-b0c1-af2609623160',
                reserved_infos.BENCHMARKS.name:
                    '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae',
            },
            'guid': '4989617a-14d6-4f80-8f75-dafda2ff13b0',
            'name': 'foo',
            'unit': 'count'}
    ])
    self.testapp.post('/add_histograms', {'data': data})

    diagnostics = histogram.SparseDiagnostic.query().fetch()
    params_by_guid = self.TaskParamsByGuid()
    params = params_by_guid['4989617a-14d6-4f80-8f75-dafda2ff13b0']
    hist = params['data']

    self.assertEqual(4, len(diagnostics))
    self.assertEqual(
        'e9c2891d-2b04-413f-8cf4-099827e67626',
        hist['diagnostics'][reserved_infos.MASTERS.name])

  def testPostHistogram_DeduplicatesSameSparseDiagnostic(self):
    diag_dict = {
        'values': ['master'],
        'guid': 'e9c2891d-2b04-413f-8cf4-099827e67626',
        'type': 'GenericSet'
    }
    diag = histogram.SparseDiagnostic(
        id='e9c2891d-2b04-413f-8cf4-099827e67626', data=diag_dict,
        start_revision=1, end_revision=sys.maxint,
        test=utils.TestKey('master/bot/benchmark'))
    diag.put()
    data = json.dumps([
        {
            'values': ['benchmark'],
            'guid': '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae',
            'type': 'GenericSet',
        }, {
            'values': [424242],
            'guid': '25f0a111-9bb4-4cea-b0c1-af2609623160',
            'type': 'GenericSet',
        }, {
            'values': ['master'],
            'guid': 'e9c2891d-2b04-413f-8cf4-099827e67626',
            'type': 'GenericSet'
        }, {
            'values': ['bot'],
            'guid': '53fb5448-9f8d-407a-8891-e7233fe1740f',
            'type': 'GenericSet'
        }, {
            'binBoundaries': [1, [1, 1000, 20]],
            'diagnostics': {
                reserved_infos.MASTERS.name:
                    'e9c2891d-2b04-413f-8cf4-099827e67626',
                reserved_infos.BOTS.name:
                    '53fb5448-9f8d-407a-8891-e7233fe1740f',
                reserved_infos.CHROMIUM_COMMIT_POSITIONS.name:
                    '25f0a111-9bb4-4cea-b0c1-af2609623160',
                reserved_infos.BENCHMARKS.name:
                    '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae',
            },
            'guid': '4989617a-14d6-4f80-8f75-dafda2ff13b0',
            'name': 'foo',
            'unit': 'count'
        }
    ])
    self.testapp.post('/add_histograms', {'data': data})

    diagnostics = histogram.SparseDiagnostic.query().fetch()
    params_by_guid = self.TaskParamsByGuid()
    params = params_by_guid['4989617a-14d6-4f80-8f75-dafda2ff13b0']
    hist = params['data']

    self.assertEqual(3, len(diagnostics))
    self.assertEqual(
        'e9c2891d-2b04-413f-8cf4-099827e67626',
        hist['diagnostics'][reserved_infos.MASTERS.name])

  def testPostHistogramFailsWithoutHistograms(self):
    data = json.dumps([
        {
            'values': ['benchmark'],
            'guid': '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae',
            'type': 'GenericSet',
        }, {
            'values': ['master'],
            'guid': 'e9c2891d-2b04-413f-8cf4-099827e67626',
            'type': 'GenericSet'
        }, {
            'values': ['bot'],
            'guid': '53fb5448-9f8d-407a-8891-e7233fe1740f',
            'type': 'GenericSet'
        }
    ])

    self.testapp.post('/add_histograms', {'data': data}, status=400)

  def testPostHistogramFailsWithoutBuildbotInfo(self):
    data = json.dumps([
        {
            'values': ['benchmark'],
            'guid': '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae',
            'type': 'GenericSet'
        }, {
            'values': [424242],
            'guid': '25f0a111-9bb4-4cea-b0c1-af2609623160',
            'type': 'GenericSet',
        }, {
            'binBoundaries': [1, [1, 1000, 20]],
            'diagnostics': {
                reserved_infos.CHROMIUM_COMMIT_POSITIONS.name:
                    '25f0a111-9bb4-4cea-b0c1-af2609623160',
                reserved_infos.BENCHMARKS.name:
                    '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae',
            },
            'guid': '4989617a-14d6-4f80-8f75-dafda2ff13b0',
            'name': 'foo',
            'unit': 'count'
        }
    ])

    self.testapp.post('/add_histograms', {'data': data}, status=400)

  def testPostHistogramFailsWithoutChromiumCommit(self):
    data = json.dumps([
        {
            'values': ['benchmark'],
            'guid': '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae',
            'type': 'GenericSet',
        }, {
            'values': ['master'],
            'guid': 'e9c2891d-2b04-413f-8cf4-099827e67626',
            'type': 'GenericSet'
        }, {
            'values': ['bot'],
            'guid': '53fb5448-9f8d-407a-8891-e7233fe1740f',
            'type': 'GenericSet'
        }, {
            'binBoundaries': [1, [1, 1000, 20]],
            'diagnostics': {
                reserved_infos.MASTERS.name:
                    'e9c2891d-2b04-413f-8cf4-099827e67626',
                reserved_infos.BOTS.name:
                    '53fb5448-9f8d-407a-8891-e7233fe1740f',
                reserved_infos.BENCHMARKS.name:
                    '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae',
            },
            'guid': '4989617a-14d6-4f80-8f75-dafda2ff13b0',
            'name': 'foo',
            'unit': 'count'}
    ])

    self.testapp.post('/add_histograms', {'data': data}, status=400)

  def testPostHistogramFailsWithoutBenchmark(self):
    data = json.dumps([
        {
            'values': [424242],
            'guid': '25f0a111-9bb4-4cea-b0c1-af2609623160',
            'type': 'GenericSet',
        }, {
            'values': ['master'],
            'guid': 'e9c2891d-2b04-413f-8cf4-099827e67626',
            'type': 'GenericSet'
        }, {
            'values': ['bot'],
            'guid': '53fb5448-9f8d-407a-8891-e7233fe1740f',
            'type': 'GenericSet'
        }, {
            'binBoundaries': [1, [1, 1000, 20]],
            'diagnostics': {
                reserved_infos.MASTERS.name:
                    'e9c2891d-2b04-413f-8cf4-099827e67626',
                reserved_infos.BOTS.name:
                    '53fb5448-9f8d-407a-8891-e7233fe1740f',
                reserved_infos.CHROMIUM_COMMIT_POSITIONS.name:
                    '25f0a111-9bb4-4cea-b0c1-af2609623160'
            },
            'guid': '4989617a-14d6-4f80-8f75-dafda2ff13b0',
            'name': 'foo',
            'unit': 'count'
        }
    ])

    self.testapp.post('/add_histograms', {'data': data}, status=400)

  def testPostHistogram_AddsSparseDiagnosticByName(self):
    data = json.dumps([
        {
            'type': 'GenericSet',
            'guid': 'cabb59fe-4bcf-4512-881c-d038c7a80635',
            'values': ['alice@chromium.org']
        },
        {
            'values': ['benchmark'],
            'guid': '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae',
            'type': 'GenericSet',
        }, {
            'values': [424242],
            'guid': '25f0a111-9bb4-4cea-b0c1-af2609623160',
            'type': 'GenericSet',
        }, {
            'values': ['master'],
            'guid': 'e9c2891d-2b04-413f-8cf4-099827e67626',
            'type': 'GenericSet'
        }, {
            'values': ['bot'],
            'guid': '53fb5448-9f8d-407a-8891-e7233fe1740f',
            'type': 'GenericSet'
        }, {
            'binBoundaries': [1, [1, 1000, 20]],
            'diagnostics': {
                reserved_infos.OWNERS.name:
                    'cabb59fe-4bcf-4512-881c-d038c7a80635',
                reserved_infos.MASTERS.name:
                    'e9c2891d-2b04-413f-8cf4-099827e67626',
                reserved_infos.BOTS.name:
                    '53fb5448-9f8d-407a-8891-e7233fe1740f',
                reserved_infos.CHROMIUM_COMMIT_POSITIONS.name:
                    '25f0a111-9bb4-4cea-b0c1-af2609623160',
                reserved_infos.BENCHMARKS.name:
                    '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae',
            },
            'guid': '4989617a-14d6-4f80-8f75-dafda2ff13b0',
            'name': 'foo',
            'unit': 'count'}
        ])

    self.testapp.post('/add_histograms', {'data': data})

    diagnostics = histogram.SparseDiagnostic.query().fetch()
    params_by_guid = self.TaskParamsByGuid()

    params = params_by_guid['4989617a-14d6-4f80-8f75-dafda2ff13b0']
    hist = params['data']
    owners_info = hist['diagnostics'][reserved_infos.OWNERS.name]
    self.assertEqual(4, len(diagnostics))

    names = [
        reserved_infos.BENCHMARKS.name,
        reserved_infos.BOTS.name,
        reserved_infos.OWNERS.name,
        reserved_infos.MASTERS.name]
    diagnostics_by_name = {}
    for d in diagnostics:
      self.assertIn(d.name, names)
      names.remove(d.name)
      diagnostics_by_name[d.name] = d
    self.assertEqual(
        ['benchmark'],
        diagnostics_by_name[reserved_infos.BENCHMARKS.name].data['values'])
    self.assertEqual(
        ['bot'],
        diagnostics_by_name[reserved_infos.BOTS.name].data['values'])
    self.assertEqual(
        ['alice@chromium.org'],
        diagnostics_by_name[reserved_infos.OWNERS.name].data['values'])
    self.assertEqual(
        ['master'],
        diagnostics_by_name[reserved_infos.MASTERS.name].data['values'])
    self.assertEqual('cabb59fe-4bcf-4512-881c-d038c7a80635', owners_info)

  def testPostHistogram_AddsSparseDiagnosticByName_OnlyOnce(self):
    data = json.dumps([
        {
            'type': 'GenericSet',
            'guid': 'cabb59fe-4bcf-4512-881c-d038c7a80635',
            'values': ['alice@chromium.org']
        },
        {
            'values': ['benchmark'],
            'guid': '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae',
            'type': 'GenericSet',
        }, {
            'values': [424242],
            'guid': '25f0a111-9bb4-4cea-b0c1-af2609623160',
            'type': 'GenericSet'
        }, {
            'values': ['master'],
            'guid': 'e9c2891d-2b04-413f-8cf4-099827e67626',
            'type': 'GenericSet'
        }, {
            'values': ['bot'],
            'guid': '53fb5448-9f8d-407a-8891-e7233fe1740f',
            'type': 'GenericSet'
        }, {
            'binBoundaries': [1, [1, 1000, 20]],
            'diagnostics': {
                reserved_infos.MASTERS.name:
                    'e9c2891d-2b04-413f-8cf4-099827e67626',
                reserved_infos.BOTS.name:
                    '53fb5448-9f8d-407a-8891-e7233fe1740f',
                reserved_infos.CHROMIUM_COMMIT_POSITIONS.name:
                    '25f0a111-9bb4-4cea-b0c1-af2609623160',
                reserved_infos.BENCHMARKS.name:
                    '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae',
                reserved_infos.OWNERS.name:
                    'cabb59fe-4bcf-4512-881c-d038c7a80635'
            },
            'guid': '4989617a-14d6-4f80-8f75-dafda2ff13b0',
            'name': 'foo',
            'unit': 'count'
        }, {
            'binBoundaries': [1, [1, 1000, 20]],
            'diagnostics': {
                reserved_infos.MASTERS.name:
                    'e9c2891d-2b04-413f-8cf4-099827e67626',
                reserved_infos.BOTS.name:
                    '53fb5448-9f8d-407a-8891-e7233fe1740f',
                reserved_infos.CHROMIUM_COMMIT_POSITIONS.name:
                    '25f0a111-9bb4-4cea-b0c1-af2609623160',
                reserved_infos.OWNERS.name:
                    'cabb59fe-4bcf-4512-881c-d038c7a80635',
                reserved_infos.BENCHMARKS.name:
                    '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae',
            },
            'guid': '5239617a-14d6-4f80-8f75-dafda2ff13b1',
            'name': 'bar',
            'unit': 'count'
        }])

    self.testapp.post('/add_histograms', {'data': data})

    diagnostics = histogram.SparseDiagnostic.query().fetch()

    self.assertEqual(4, len(diagnostics))
    self.assertEqual(reserved_infos.BOTS.name, diagnostics[1].name)
    self.assertNotEqual(reserved_infos.BOTS.name, diagnostics[0].name)

  def testPostHistogram_AddsSparseDiagnosticByName_ErrorsIfDiverging(self):
    data = json.dumps([
        {
            'type': 'GenericSet',
            'guid': 'cabb59fe-4bcf-4512-881c-d038c7a80635',
            'values': ['alice@chromium.org']
        }, {
            'type': 'GenericSet',
            'guid': '7c5bd92f-4146-411b-9192-248ffc1be92c',
            'values': ['bob@chromium.org']
        }, {
            'values': ['benchmark'],
            'guid': '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae',
            'type': 'GenericSet'
        }, {
            'values': [424242],
            'guid': '25f0a111-9bb4-4cea-b0c1-af2609623160',
            'type': 'GenericSet'
        }, {
            'values': ['master'],
            'guid': 'e9c2891d-2b04-413f-8cf4-099827e67626',
            'type': 'GenericSet'
        }, {
            'values': ['bot'],
            'guid': '53fb5448-9f8d-407a-8891-e7233fe1740f',
            'type': 'GenericSet'
        }, {
            'binBoundaries': [1, [1, 1000, 20]],
            'diagnostics': {
                reserved_infos.MASTERS.name:
                    'e9c2891d-2b04-413f-8cf4-099827e67626',
                reserved_infos.BOTS.name:
                    '53fb5448-9f8d-407a-8891-e7233fe1740f',
                reserved_infos.CHROMIUM_COMMIT_POSITIONS.name:
                    '25f0a111-9bb4-4cea-b0c1-af2609623160',
                reserved_infos.BENCHMARKS.name:
                    '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae',
                reserved_infos.OWNERS.name:
                    'cabb59fe-4bcf-4512-881c-d038c7a80635'
            },
            'guid': '4989617a-14d6-4f80-8f75-dafda2ff13b0',
            'name': 'foo',
            'unit': 'count'
        }, {
            'binBoundaries': [1, [1, 1000, 20]],
            'diagnostics': {
                reserved_infos.MASTERS.name:
                    'e9c2891d-2b04-413f-8cf4-099827e67626',
                reserved_infos.BOTS.name:
                    '53fb5448-9f8d-407a-8891-e7233fe1740f',
                reserved_infos.CHROMIUM_COMMIT_POSITIONS.name:
                    '25f0a111-9bb4-4cea-b0c1-af2609623160',
                reserved_infos.BENCHMARKS.name:
                    '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae',
                reserved_infos.OWNERS.name:
                    '7c5bd92f-4146-411b-9192-248ffc1be92c'
            },
            'guid': 'bda61ae3-0178-43f8-8aec-3ab78b9a2e18',
            'name': 'foo',
            'unit': 'count'
        }])

    self.testapp.post('/add_histograms', {'data': data}, status=500)

  def testFindHistogramLevelSparseDiagnostics(self):
    hist = histogram_module.Histogram('hist', 'count')
    histograms = histogram_set.HistogramSet([hist])
    histograms.AddSharedDiagnostic('foo', generic_set.GenericSet(['bar']))
    histograms.AddSharedDiagnostic(
        reserved_infos.DEVICE_IDS.name,
        generic_set.GenericSet([]))
    diagnostics = add_histograms.FindHistogramLevelSparseDiagnostics(
        hist.guid, histograms)

    self.assertEqual(1, len(diagnostics))
    self.assertIsInstance(
        diagnostics[reserved_infos.DEVICE_IDS.name],
        generic_set.GenericSet)

  def testFindSuiteLevelSparseDiagnostics(self):
    def _CreateSingleHistogram(hist, master):
      h = histogram_module.Histogram(hist, 'count')
      h.diagnostics[reserved_infos.MASTERS.name] = (
          generic_set.GenericSet([master]))
      return h

    histograms = histogram_set.HistogramSet([
        _CreateSingleHistogram('hist1', 'master1'),
        _CreateSingleHistogram('hist2', 'master2')])

    with self.assertRaises(ValueError):
      add_histograms.FindSuiteLevelSparseDiagnostics(
          histograms, utils.TestKey('M/B/Foo'), 12345, False)

  def testComputeTestPathWithStory(self):
    hist = histogram_module.Histogram('hist', 'count')
    histograms = histogram_set.HistogramSet([hist])
    histograms.AddSharedDiagnostic(
        reserved_infos.MASTERS.name,
        generic_set.GenericSet(['master']))
    histograms.AddSharedDiagnostic(
        reserved_infos.BOTS.name,
        generic_set.GenericSet(['bot']))
    histograms.AddSharedDiagnostic(
        reserved_infos.BENCHMARKS.name,
        generic_set.GenericSet(['benchmark']))
    histograms.AddSharedDiagnostic(
        reserved_infos.STORIES.name,
        generic_set.GenericSet(['http://story']))
    hist = histograms.GetFirstHistogram()
    test_path = add_histograms.ComputeTestPath(
        'master/bot/benchmark', hist.guid, histograms)
    self.assertEqual('master/bot/benchmark/hist/http___story', test_path)

  def testComputeTestPathWithTIRLabel(self):
    hist = histogram_module.Histogram('hist', 'count')
    histograms = histogram_set.HistogramSet([hist])
    histograms.AddSharedDiagnostic(
        reserved_infos.MASTERS.name,
        generic_set.GenericSet(['master']))
    histograms.AddSharedDiagnostic(
        reserved_infos.BOTS.name,
        generic_set.GenericSet(['bot']))
    histograms.AddSharedDiagnostic(
        reserved_infos.BENCHMARKS.name,
        generic_set.GenericSet(['benchmark']))
    histograms.AddSharedDiagnostic(
        reserved_infos.STORIES.name,
        generic_set.GenericSet(['http://story']))
    histograms.AddSharedDiagnostic(
        reserved_infos.STORY_TAGS.name,
        generic_set.GenericSet(
            ['group:media', 'ignored_tag', 'case:browse']))
    hist = histograms.GetFirstHistogram()
    test_path = add_histograms.ComputeTestPath(
        'master/bot/benchmark', hist.guid, histograms)
    self.assertEqual(
        'master/bot/benchmark/hist/browse_media/http___story', test_path)

  def testComputeTestPathWithoutStory(self):
    hist = histogram_module.Histogram('hist', 'count')
    histograms = histogram_set.HistogramSet([hist])
    histograms.AddSharedDiagnostic(
        reserved_infos.MASTERS.name,
        generic_set.GenericSet(['master']))
    histograms.AddSharedDiagnostic(
        reserved_infos.BOTS.name,
        generic_set.GenericSet(['bot']))
    histograms.AddSharedDiagnostic(
        reserved_infos.BENCHMARKS.name,
        generic_set.GenericSet(['benchmark']))
    hist = histograms.GetFirstHistogram()
    test_path = add_histograms.ComputeTestPath(
        'master/bot/benchmark', hist.guid, histograms)
    self.assertEqual('master/bot/benchmark/hist', test_path)

  def testComputeTestPathWithIsRefWithoutStory(self):
    hist = histogram_module.Histogram('hist', 'count')
    histograms = histogram_set.HistogramSet([hist])
    histograms.AddSharedDiagnostic(
        reserved_infos.MASTERS.name,
        generic_set.GenericSet(['master']))
    histograms.AddSharedDiagnostic(
        reserved_infos.BOTS.name,
        generic_set.GenericSet(['bot']))
    histograms.AddSharedDiagnostic(
        reserved_infos.BENCHMARKS.name,
        generic_set.GenericSet(['benchmark']))
    histograms.AddSharedDiagnostic(
        reserved_infos.IS_REFERENCE_BUILD.name,
        generic_set.GenericSet([True]))
    hist = histograms.GetFirstHistogram()
    test_path = add_histograms.ComputeTestPath(
        'master/bot/benchmark', hist.guid, histograms)
    self.assertEqual('master/bot/benchmark/hist/ref', test_path)

  def testComputeTestPathWithIsRefAndStory(self):
    hist = histogram_module.Histogram('hist', 'count')
    histograms = histogram_set.HistogramSet([hist])
    histograms.AddSharedDiagnostic(
        reserved_infos.MASTERS.name,
        generic_set.GenericSet(['master']))
    histograms.AddSharedDiagnostic(
        reserved_infos.BOTS.name,
        generic_set.GenericSet(['bot']))
    histograms.AddSharedDiagnostic(
        reserved_infos.BENCHMARKS.name,
        generic_set.GenericSet(['benchmark']))
    histograms.AddSharedDiagnostic(
        reserved_infos.STORIES.name,
        generic_set.GenericSet(['http://story']))
    histograms.AddSharedDiagnostic(
        reserved_infos.IS_REFERENCE_BUILD.name,
        generic_set.GenericSet([True]))
    hist = histograms.GetFirstHistogram()
    test_path = add_histograms.ComputeTestPath(
        'master/bot/benchmark', hist.guid, histograms)
    self.assertEqual('master/bot/benchmark/hist/http___story_ref', test_path)

  def testComputeRevision(self):
    hist = histogram_module.Histogram('hist', 'count')
    histograms = histogram_set.HistogramSet([hist])
    chromium_commit = generic_set.GenericSet([424242])
    histograms.AddSharedDiagnostic(
        reserved_infos.CHROMIUM_COMMIT_POSITIONS.name, chromium_commit)
    self.assertEqual(424242, add_histograms.ComputeRevision(histograms))

  def testComputeRevision_NotInteger_Raises(self):
    hist = histogram_module.Histogram('hist', 'count')
    histograms = histogram_set.HistogramSet([hist])
    chromium_commit = generic_set.GenericSet(['123'])
    histograms.AddSharedDiagnostic(
        reserved_infos.CHROMIUM_COMMIT_POSITIONS.name, chromium_commit)
    with self.assertRaises(api_request_handler.BadRequestError):
      add_histograms.ComputeRevision(histograms)

  def testComputeRevision_RaisesOnError(self):
    hist = histogram_module.Histogram('hist', 'count')
    histograms = histogram_set.HistogramSet([hist])
    chromium_commit = generic_set.GenericSet([424242, 0])
    histograms.AddSharedDiagnostic(
        reserved_infos.CHROMIUM_COMMIT_POSITIONS.name, chromium_commit)
    with self.assertRaises(api_request_handler.BadRequestError):
      add_histograms.ComputeRevision(histograms)

  def testSparseDiagnosticsAreNotInlined(self):
    hist = histogram_module.Histogram('hist', 'count')
    histograms = histogram_set.HistogramSet([hist])
    histograms.AddSharedDiagnostic(
        reserved_infos.BENCHMARKS.name,
        generic_set.GenericSet(['benchmark']))
    add_histograms.InlineDenseSharedDiagnostics(histograms)
    self.assertTrue(hist.diagnostics[reserved_infos.BENCHMARKS.name].has_guid)

  @mock.patch('logging.info')
  def testLogDebugInfo_Succeeds(self, mock_log):
    hist = histogram_module.Histogram('hist', 'count')
    histograms = histogram_set.HistogramSet([hist])
    histograms.AddSharedDiagnostic(
        reserved_infos.LOG_URLS.name,
        generic_set.GenericSet(['http://foo']))
    add_histograms._LogDebugInfo(histograms)
    mock_log.assert_called_once_with('Buildbot URL: %s', "['http://foo']")

  @mock.patch('logging.info')
  def testLogDebugInfo_NoHistograms(self, mock_log):
    histograms = histogram_set.HistogramSet()
    add_histograms._LogDebugInfo(histograms)
    mock_log.assert_called_once_with('No histograms in data.')

  @mock.patch('logging.info')
  def testLogDebugInfo_NoLogUrls(self, mock_log):
    hist = histogram_module.Histogram('hist', 'count')
    histograms = histogram_set.HistogramSet([hist])
    add_histograms._LogDebugInfo(histograms)
    mock_log.assert_called_once_with('No LOG_URLS in data.')

  def testDeduplicateAndPut_Same(self):
    d = {
        'values': ['master'],
        'guid': 'e9c2891d-2b04-413f-8cf4-099827e67626',
        'type': 'GenericSet'
    }
    test_key = utils.TestKey('Chromium/win7/foo')
    entity = histogram.SparseDiagnostic(
        data=d, name='masters', test=test_key, start_revision=1,
        end_revision=sys.maxint, id='abc')
    entity.put()
    d2 = d.copy()
    d2['guid'] = 'def'
    entity2 = histogram.SparseDiagnostic(
        data=d2, test=test_key,
        start_revision=2, end_revision=sys.maxint, id='def')
    add_histograms.DeduplicateAndPut([entity2], test_key, 2)
    sparse = histogram.SparseDiagnostic.query().fetch()
    self.assertEqual(2, len(sparse))

  def testDeduplicateAndPut_Different(self):
    d = {
        'values': ['master'],
        'guid': 'e9c2891d-2b04-413f-8cf4-099827e67626',
        'type': 'GenericSet'
    }
    test_key = utils.TestKey('Chromium/win7/foo')
    entity = histogram.SparseDiagnostic(
        data=d, name='masters', test=test_key, start_revision=1,
        end_revision=sys.maxint, id='abc')
    entity.put()
    d2 = d.copy()
    d2['guid'] = 'def'
    d2['displayBotName'] = 'mac'
    entity2 = histogram.SparseDiagnostic(
        data=d2, test=test_key,
        start_revision=1, end_revision=sys.maxint, id='def')
    add_histograms.DeduplicateAndPut([entity2], test_key, 2)
    sparse = histogram.SparseDiagnostic.query().fetch()
    self.assertEqual(2, len(sparse))

  def testDeduplicateAndPut_New(self):
    d = {
        'values': ['master'],
        'guid': 'e9c2891d-2b04-413f-8cf4-099827e67626',
        'type': 'GenericSet'
    }
    test_key = utils.TestKey('Chromium/win7/foo')
    entity = histogram.SparseDiagnostic(
        data=d, test=test_key, start_revision=1,
        end_revision=sys.maxint, id='abc')
    entity.put()
    add_histograms.DeduplicateAndPut([entity], test_key, 1)
    sparse = histogram.SparseDiagnostic.query().fetch()
    self.assertEqual(1, len(sparse))
