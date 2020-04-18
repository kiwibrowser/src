#!/usr/bin/env python
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys
import unittest

import PRESUBMIT

sys.path.append(os.path.abspath(os.path.dirname(os.path.abspath(__file__))
    + '/../../../..'))
from PRESUBMIT_test_mocks import MockFile, MockInputApi, MockOutputApi

class CheckNotificationConstructors(unittest.TestCase):
  """Test the _CheckNotificationConstructors presubmit check."""

  def testTruePositives(self):
    """Examples of when Notification.Builder use is correctly flagged."""
    mock_input = MockInputApi()
    mock_input.files = [
      MockFile('path/One.java', ['new Notification.Builder()']),
      MockFile('path/Two.java', ['new NotificationCompat.Builder()']),
    ]
    errors = PRESUBMIT._CheckNotificationConstructors(
        mock_input, MockOutputApi())
    self.assertEqual(1, len(errors))
    self.assertEqual(2, len(errors[0].items))
    self.assertTrue('One.java' in errors[0].items[0])
    self.assertTrue('Two.java' in errors[0].items[1])

  def testFalsePositives(self):
    """Examples of when Notification.Builder should not be flagged."""
    mock_input = MockInputApi()
    mock_input.files = [
      MockFile(
          'chrome/android/java/src/org/chromium/chrome/browser/notifications/'
              'NotificationBuilder.java',
          ['new Notification.Builder()']),
      MockFile(
          'chrome/android/java/src/org/chromium/chrome/browser/notifications/'
              'NotificationCompatBuilder.java',
          ['new NotificationCompat.Builder()']),
      MockFile('path/One.java', ['Notification.Builder']),
      MockFile('path/Two.java', ['// do not: new Notification.Builder()']),
      MockFile('path/Three.java', [
          '/** ChromeNotificationBuilder',
          ' * replaces: new Notification.Builder()']),
      MockFile('path/PRESUBMIT.py', ['new Notification.Builder()']),
      MockFile('path/Four.java', ['new NotificationCompat.Builder()'],
          action='D'),
    ]
    errors = PRESUBMIT._CheckNotificationConstructors(
        mock_input, MockOutputApi())
    self.assertEqual(0, len(errors))


if __name__ == '__main__':
  unittest.main()
