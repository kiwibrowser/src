# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys
import unittest
import urllib

import mock

from google.appengine.ext import ndb

from dashboard import email_sheriff
from dashboard.common import testing_common
from dashboard.common import utils
from dashboard.models import anomaly
from dashboard.models import bug_label_patterns
from dashboard.models import sheriff

_SHERIFF_URL = 'http://chromium-build.appspot.com/p/chromium/sheriff_perf.js'
_SHERIFF_EMAIL = 'perf-sheriff-group@google.com'


class EmailSheriffTest(testing_common.TestCase):

  def _AddTestToStubDataStore(self):
    """Adds a test which will be used in the methods below."""
    bug_label_patterns.AddBugLabelPattern('label1', '*/*/dromaeo/dom')
    bug_label_patterns.AddBugLabelPattern('label2', '*/*/other/test')
    testing_common.AddTests(
        ['ChromiumPerf'], ['Win7'], {'dromaeo': {'dom': {}}})
    test = utils.TestKey('ChromiumPerf/Win7/dromaeo/dom').get()
    test.improvement_direction = anomaly.DOWN
    sheriff.Sheriff(
        id='Chromium Perf Sheriff',
        url=_SHERIFF_URL,
        email=_SHERIFF_EMAIL,
        labels=['Performance-Sheriff']).put()
    return test

  def _GetDefaultMailArgs(self):
    """Adds an Anomaly and returns arguments for email_sheriff.EmailSheriff."""
    test_entity = self._AddTestToStubDataStore()
    sheriff_entity = ndb.Key('Sheriff', 'Chromium Perf Sheriff').get()
    anomaly_entity = anomaly.Anomaly(
        median_before_anomaly=5.0,
        median_after_anomaly=10.0,
        start_revision=10002,
        end_revision=10004,
        sheriff=sheriff_entity.key,
        test=utils.TestKey('ChromiumPerf/Win7/dromaeo/dom'))
    return {
        'sheriff': sheriff_entity,
        'test': test_entity,
        'anomaly': anomaly_entity
    }

  @mock.patch(
      'google.appengine.api.urlfetch.fetch',
      mock.MagicMock(return_value=testing_common.FakeResponseObject(
          200, 'document.write(\'sullivan\')')))
  def testEmailSheriff_ContentAndRecipientAreCorrect(self):
    email_sheriff.EmailSheriff(**self._GetDefaultMailArgs())

    messages = self.mail_stub.get_sent_messages()
    self.assertEqual(1, len(messages))
    self.assertEqual('gasper-alerts@google.com', messages[0].sender)
    self.assertEqual(
        set(['perf-sheriff-group@google.com', 'sullivan@google.com']),
        set([s.strip() for s in messages[0].to.split(',')]))

    name = 'dromaeo/dom on Win7'
    expected_subject = '100.0%% regression in %s at 10002:10004' % name
    self.assertEqual(expected_subject, messages[0].subject)
    body = str(messages[0].body)
    self.assertIn('10002 - 10004', body)
    self.assertIn('100.0%', body)
    self.assertIn('ChromiumPerf', body)
    self.assertIn('Win7', body)
    self.assertIn('dromaeo/dom', body)

    html = str(messages[0].html)
    self.assertIn('<b>10002 - 10004</b>', html)
    self.assertIn('<b>100.0%</b>', html)
    self.assertIn('<b>ChromiumPerf</b>', html)
    self.assertIn('<b>Win7</b>', html)
    self.assertIn('<b>dromaeo/dom</b>', html)

    # Sheriff link
    self.assertIn(
        '/alerts?sheriff=%s' % urllib.quote('Chromium Perf Sheriff'), html)

  @mock.patch(
      'google.appengine.api.urlfetch.fetch',
      mock.MagicMock(return_value=testing_common.FakeResponseObject(
          200, 'document.write(\'sonnyrao, digit\')')))
  def testEmailSheriff_MultipleSheriffs_AllGetEmailed(self):
    email_sheriff.EmailSheriff(**self._GetDefaultMailArgs())
    messages = self.mail_stub.get_sent_messages()
    self.assertEqual(1, len(messages))
    self.assertEqual('gasper-alerts@google.com', messages[0].sender)
    self.assertEqual(
        set(['perf-sheriff-group@google.com',
             'sonnyrao@google.com', 'digit@google.com']),
        set([s.strip() for s in messages[0].to.split(',')]))

  def testEmail_NoSheriffUrl_EmailSentToSheriffRotationEmailAddress(self):
    args = self._GetDefaultMailArgs()
    args['sheriff'].url = None
    email_sheriff.EmailSheriff(**args)
    messages = self.mail_stub.get_sent_messages()
    self.assertEqual(1, len(messages))
    # An email is only sent to the general sheriff rotation email;
    # There is no other specific sheriff to send it to.
    self.assertEqual('perf-sheriff-group@google.com', messages[0].to)

  @mock.patch(
      'google.appengine.api.urlfetch.fetch',
      mock.MagicMock(
          return_value=testing_common.FakeResponseObject(200, 'garbage')))
  def testEmailSheriff_RotationUrlHasInvalidContent_EmailStillSent(self):
    """Tests the email to list when the rotation URL returns garbage."""
    args = self._GetDefaultMailArgs()
    email_sheriff.EmailSheriff(**args)
    messages = self.mail_stub.get_sent_messages()
    self.assertEqual(1, len(messages))
    # An email is only sent to the general sheriff rotation email.
    self.assertEqual('perf-sheriff-group@google.com', messages[0].to)

  def testEmailSheriff_PercentChangeMaxFloat_ContentSaysAlertSize(self):
    """Tests the email content for "freakin huge" alert."""
    args = self._GetDefaultMailArgs()
    args['sheriff'].url = None
    args['anomaly'].median_before_anomaly = 0.0
    email_sheriff.EmailSheriff(**args)
    messages = self.mail_stub.get_sent_messages()
    self.assertEqual(1, len(messages))
    self.assertIn(anomaly.FREAKIN_HUGE, str(messages[0].subject))
    self.assertNotIn(str(sys.float_info.max), str(messages[0].body))
    self.assertIn(anomaly.FREAKIN_HUGE, str(messages[0].body))
    self.assertNotIn(str(sys.float_info.max), str(messages[0].html))
    self.assertIn(anomaly.FREAKIN_HUGE, str(messages[0].html))


if __name__ == '__main__':
  unittest.main()
