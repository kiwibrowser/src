# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import unittest

import webapp2
import webtest

from google.appengine.ext import ndb

from dashboard import email_summary
from dashboard.common import testing_common
from dashboard.models import anomaly
from dashboard.models import bug_label_patterns
from dashboard.models import sheriff


class EmailSummaryTest(testing_common.TestCase):

  def setUp(self):
    super(EmailSummaryTest, self).setUp()
    app = webapp2.WSGIApplication(
        [('/email_summary', email_summary.EmailSummaryHandler)])
    self.testapp = webtest.TestApp(app)

  def _AddAnomalies(
      self, rev_range_start, rev_range_end, median_before_anomaly,
      median_after_anomaly, sheriff_key, anomaly_time=None):
    first_paint = ndb.Key(
        'TestMetadata',
        'ChromiumGPU/linux-release/scrolling-benchmark/first_paint')
    mean_frame_time = ndb.Key(
        'TestMetadata',
        'ChromiumGPU/linux-release/scrolling-benchmark/mean_frame_time')
    if not anomaly_time:
      anomaly_time = datetime.datetime.now()
    for end_rev in range(rev_range_start, rev_range_end, 10):
      test = first_paint if end_rev % 20 == 0 else mean_frame_time
      anomaly.Anomaly(
          start_revision=end_rev - 5, end_revision=end_rev, test=test,
          median_before_anomaly=median_before_anomaly,
          median_after_anomaly=median_after_anomaly,
          timestamp=anomaly_time, sheriff=sheriff_key).put()

  def _AddNewAlertsForSheriffsWithNoSummary(self):
    """Adds a sheriff with summarize set to False, and some alerts."""
    sheriff_key = sheriff.Sheriff(
        id='Chromium Perf Sheriff', email='anandc@google.com',
        summarize=False, labels=['Performance-Sheriff']).put()
    testing_common.AddTests(['ChromiumGPU'], ['linux-release'], {
        'scrolling-benchmark': {
            'first_paint': {},
            'mean_frame_time': {},
        }
    })
    anomaly_time = datetime.datetime.now() - datetime.timedelta(hours=18)
    self._AddAnomalies(10000, 10020, 100, 200, sheriff_key, anomaly_time)
    anomaly_time = datetime.datetime.now() - datetime.timedelta(hours=36)
    self._AddAnomalies(10120, 10140, 100, 150, sheriff_key, anomaly_time)

  def _AddFourNewAlertsWithSummaryForOnlyTwo(self, internal_only=False):
    sheriff_key = sheriff.Sheriff(
        id='Chromium Perf Sheriff', email='anandc@google.com',
        summarize=True, labels=['Performance-Sheriff'],
        internal_only=internal_only).put()
    testing_common.AddTests(['ChromiumGPU'], ['linux-release'], {
        'scrolling-benchmark': {
            'first_paint': {},
            'mean_frame_time': {},
        }
    })
    anomaly_time = datetime.datetime.now() - datetime.timedelta(hours=18)
    self._AddAnomalies(10000, 10020, 100, 200, sheriff_key, anomaly_time)
    anomaly_time = datetime.datetime.now() - datetime.timedelta(hours=36)
    self._AddAnomalies(10120, 10140, 100, 150, sheriff_key, anomaly_time)

  def _AddAlertsForFreakinHugeRegressions(self):
    sheriff_key = sheriff.Sheriff(
        id='Chromium Perf Sheriff', email='anandc@google.com',
        summarize=True, internal_only=False).put()
    testing_common.AddTests(['ChromiumGPU'], ['linux-release'], {
        'scrolling-benchmark': {
            'first_paint': {},
            'mean_frame_time': {},
        }
    })
    anomaly_time = datetime.datetime.now() - datetime.timedelta(hours=18)
    self._AddAnomalies(10000, 10020, 0, 100, sheriff_key, anomaly_time)

  def _AddOldAlertsForSheriffWithSummary(self):
    """Adds alerts for two separate sheriffs which both have summarize=True."""
    sheriff_key = sheriff.Sheriff(
        id='Chromium Perf Sheriff', email='anandc@google.com',
        labels=['Performance-Sheriff'],
        summarize=True, internal_only=False).put()
    testing_common.AddTests(['ChromiumGPU'], ['linux-release'], {
        'scrolling-benchmark': {
            'first_paint': {},
            'mean_frame_time': {},
        }
    })
    anomaly_time = datetime.datetime.now() - datetime.timedelta(hours=36)
    self._AddAnomalies(10000, 10020, 100, 200, sheriff_key, anomaly_time)

  def _AddAlertsForTwoSheriffsWithSummary(self):
    """Adds alerts for two separate sheriffs which both have summarize=True."""
    sheriff1 = sheriff.Sheriff(
        id='Sheriff1', email='anandc@google.com', summarize=True,
        internal_only=False).put()
    sheriff2 = sheriff.Sheriff(
        id='Sheriff2', email='anandc@chromium.org', summarize=True,
        internal_only=False).put()
    testing_common.AddTests(['ChromiumGPU'], ['linux-release'], {
        'scrolling-benchmark': {
            'first_paint': {},
            'mean_frame_time': {},
        }
    })
    anomaly_time = datetime.datetime.now() - datetime.timedelta(hours=18)
    self._AddAnomalies(10000, 10020, 100, 200, sheriff1, anomaly_time)
    self._AddAnomalies(10000, 10020, 100, 200, sheriff2, anomaly_time)

  def testGet_SheriffWithSummary(self):
    self._AddFourNewAlertsWithSummaryForOnlyTwo()
    bug_label_patterns.AddBugLabelPattern('label1', '*/*/*/first_paint')
    self.testapp.get('/email_summary')
    messages = self.mail_stub.get_sent_messages()
    self.assertEqual(1, len(messages))
    self.assertEqual(
        'Chromium Perf Sheriff: 2 anomalies found at 9995:10010.',
        messages[0].subject)
    self.assertEqual('gasper-alerts@google.com', messages[0].sender)
    self.assertEqual('anandc@google.com', messages[0].to)
    html = str(messages[0].html)
    self.assertIn('2 total performance regressions were found', html)

  def testGet_SheriffWithSummary_DoesntEmailInternalOnly(self):
    self._AddFourNewAlertsWithSummaryForOnlyTwo(internal_only=True)
    bug_label_patterns.AddBugLabelPattern('label1', '*/*/*/first_paint')
    self.testapp.get('/email_summary')
    messages = self.mail_stub.get_sent_messages()
    self.assertEqual(0, len(messages))

  def testGet_SheriffWithSummary_EmailsInternalOnly(self):
    self._AddFourNewAlertsWithSummaryForOnlyTwo(internal_only=True)
    bug_label_patterns.AddBugLabelPattern('label1', '*/*/*/first_paint')
    self.testapp.get('/email_summary?internal_only=1')
    messages = self.mail_stub.get_sent_messages()
    self.assertEqual(1, len(messages))
    self.assertEqual(
        'Chromium Perf Sheriff: 2 anomalies found at 9995:10010.',
        messages[0].subject)
    self.assertEqual('gasper-alerts@google.com', messages[0].sender)
    self.assertEqual('anandc@google.com', messages[0].to)
    html = str(messages[0].html)
    self.assertIn('2 total performance regressions were found', html)

  def testGet_SheriffWithSummary_SkipsImprovements(self):
    self._AddFourNewAlertsWithSummaryForOnlyTwo()
    for a in anomaly.Anomaly.query().fetch():
      a.is_improvement = True
      a.put()
    self.testapp.get('/email_summary')
    messages = self.mail_stub.get_sent_messages()
    self.assertEqual(0, len(messages))

  def testGet_SheriffWithSummaryButNoNewAlerts_DoesntSendEmail(self):
    self._AddOldAlertsForSheriffWithSummary()
    self.testapp.get('/email_summary')
    messages = self.mail_stub.get_sent_messages()
    self.assertEqual(0, len(messages))

  def testGet_SheriffWithSummaryAndZeroToNonZeroRegression(self):
    self._AddAlertsForFreakinHugeRegressions()
    self.testapp.get('/email_summary')
    messages = self.mail_stub.get_sent_messages()
    self.assertEqual(1, len(messages))
    html = str(messages[0].html)
    self.assertIn('2 total performance regressions were found', html)

  def testGet_SendsEmailsToMultipleSheriffsInOneRequest(self):
    self._AddAlertsForTwoSheriffsWithSummary()
    self.testapp.get('/email_summary')
    messages = self.mail_stub.get_sent_messages()
    self.assertEquals(2, len(messages))
    expected_subject1 = 'Sheriff1: 2 anomalies found at 9995:10010.'
    expected_subject2 = 'Sheriff2: 2 anomalies found at 9995:10010.'
    self.assertEquals(messages[0].to, 'anandc@google.com')
    self.assertEquals(messages[1].to, 'anandc@chromium.org')
    self.assertEquals(expected_subject1, messages[0].subject)
    self.assertEquals(expected_subject2, messages[1].subject)
    for message in messages:
      self.assertEquals(message.sender, 'gasper-alerts@google.com')
      html = str(message.html)
      self.assertIn('2 total performance regressions were found', html)

  def testGet_SheriffSummarizeSetToFalse_NoEmailSent(self):
    self._AddNewAlertsForSheriffsWithNoSummary()
    self.testapp.get('/email_summary')
    messages = self.mail_stub.get_sent_messages()
    self.assertEqual(0, len(messages))


if __name__ == '__main__':
  unittest.main()
