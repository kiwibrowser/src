# -*- coding: utf-8 -*-
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for the retry_stats.py module."""

from __future__ import print_function

import StringIO

from chromite.lib import cros_test_lib
from chromite.lib import retry_stats


# We access internal members to help with testing.
# pylint: disable=W0212


class TestRetryException(Exception):
  """Used when testing failure cases."""

class TestRetryStats(cros_test_lib.TestCase):
  """This contains test cases for the retry_stats module."""

  CAT = 'Test Service A'
  CAT_B = 'Test Service B'

  SUCCESS_RESULT = 'success result'

  def setUp(self):
    retry_stats._STATS_COLLECTION = None

  def handlerNoRetry(self, _e):
    return False

  def handlerRetry(self, _e):
    return True

  def callSuccess(self):
    return self.SUCCESS_RESULT

  def callFailure(self):
    raise TestRetryException()


  def _verifyStats(self, category, success=0, failure=0, retry=0):
    """Verify that the given category has the specified values collected."""
    stats_success, stats_failure, stats_retry = retry_stats.CategoryStats(
        category)

    self.assertEqual(stats_success, success)
    self.assertEqual(stats_failure, failure)
    self.assertEqual(stats_retry, retry)

  def testSetupStats(self):
    """Verify that we do something when we setup a new stats category."""
    # Show that setup does something.
    self.assertEqual(retry_stats._STATS_COLLECTION, None)
    retry_stats.SetupStats()
    self.assertNotEqual(retry_stats._STATS_COLLECTION, None)

  def testReportCategoryStatsEmpty(self):
    retry_stats.SetupStats()

    out = StringIO.StringIO()

    retry_stats.ReportCategoryStats(out, self.CAT)

    expected = """************************************************************
** Performance Statistics for Test Service A
**
** Success: 0
** Failure: 0
** Retries: 0
** Total: 0
************************************************************
"""

    self.assertEqual(out.getvalue(), expected)

  def testReportStatsEmpty(self):
    retry_stats.SetupStats()

    out = StringIO.StringIO()
    retry_stats.ReportStats(out)

    # No data collected means no categories are known, nothing to report.
    self.assertEqual(out.getvalue(), '')

  def testReportStats(self):
    retry_stats.SetupStats()

    # Insert some stats to report.
    retry_stats.RetryWithStats(
        self.CAT, self.handlerNoRetry, 3, self.callSuccess)
    retry_stats.RetryWithStats(
        self.CAT_B, self.handlerNoRetry, 3, self.callSuccess)
    self.assertRaises(TestRetryException,
                      retry_stats.RetryWithStats,
                      self.CAT, self.handlerRetry, 3, self.callFailure)

    out = StringIO.StringIO()
    retry_stats.ReportStats(out)

    # Expecting reports for both CAT and CAT_B used above.
    expected = """************************************************************
** Performance Statistics for Test Service A
**
** Success: 1
** Failure: 1
** Retries: 3
** Total: 2
************************************************************
************************************************************
** Performance Statistics for Test Service B
**
** Success: 1
** Failure: 0
** Retries: 0
** Total: 1
************************************************************
"""

    self.assertEqual(out.getvalue(), expected)

  def testSuccessNoSetup(self):
    """Verify that we can handle a successful call if we're never setup."""
    self.assertEqual(retry_stats._STATS_COLLECTION, None)

    result = retry_stats.RetryWithStats(
        self.CAT, self.handlerNoRetry, 3, self.callSuccess)
    self.assertEqual(result, self.SUCCESS_RESULT)

    result = retry_stats.RetryWithStats(
        self.CAT, self.handlerNoRetry, 3, self.callSuccess)
    self.assertEqual(result, self.SUCCESS_RESULT)

    self.assertEqual(retry_stats._STATS_COLLECTION, None)

  def testFailureNoRetryNoSetup(self):
    """Verify that we can handle a failure call if we're never setup."""
    self.assertEqual(retry_stats._STATS_COLLECTION, None)

    self.assertRaises(TestRetryException,
                      retry_stats.RetryWithStats,
                      self.CAT, self.handlerNoRetry, 3, self.callFailure)

    self.assertRaises(TestRetryException,
                      retry_stats.RetryWithStats,
                      self.CAT, self.handlerNoRetry, 3, self.callFailure)

    self.assertEqual(retry_stats._STATS_COLLECTION, None)

  def testSuccess(self):
    """Verify that we can handle a successful call."""
    retry_stats.SetupStats()
    self._verifyStats(self.CAT)

    # Succeed once.
    result = retry_stats.RetryWithStats(
        self.CAT, self.handlerNoRetry, 3, self.callSuccess)
    self.assertEqual(result, self.SUCCESS_RESULT)
    self._verifyStats(self.CAT, success=1)

    # Succeed twice.
    result = retry_stats.RetryWithStats(
        self.CAT, self.handlerNoRetry, 3, self.callSuccess)
    self.assertEqual(result, self.SUCCESS_RESULT)
    self._verifyStats(self.CAT, success=2)

  def testSuccessRetry(self):
    """Verify that we can handle a successful call after tries."""
    retry_stats.SetupStats()
    self._verifyStats(self.CAT)

    # Use this scoped list as a persistent counter.
    call_counter = ['fail 1', 'fail 2']

    def callRetrySuccess():
      if call_counter:
        raise TestRetryException(call_counter.pop())
      else:
        return self.SUCCESS_RESULT

    # Retry twice, then succeed.
    result = retry_stats.RetryWithStats(
        self.CAT, self.handlerRetry, 3, callRetrySuccess)
    self.assertEqual(result, self.SUCCESS_RESULT)
    self._verifyStats(self.CAT, success=1, retry=2)

  def testFailureNoRetry(self):
    """Verify that we can handle a failure if the handler doesn't retry."""
    retry_stats.SetupStats()
    self._verifyStats(self.CAT)

    # Fail once without retries.
    self.assertRaises(TestRetryException,
                      retry_stats.RetryWithStats,
                      self.CAT, self.handlerNoRetry, 3, self.callFailure)
    self._verifyStats(self.CAT, failure=1)

    # Fail twice without retries.
    self.assertRaises(TestRetryException,
                      retry_stats.RetryWithStats,
                      self.CAT, self.handlerNoRetry, 3, self.callFailure)
    self._verifyStats(self.CAT, failure=2)

  def testFailureRetry(self):
    """Verify that we can handle a failure if we use all retries."""
    retry_stats.SetupStats()
    self._verifyStats(self.CAT)

    # Fail once with exhausted retries.
    self.assertRaises(TestRetryException,
                      retry_stats.RetryWithStats,
                      self.CAT, self.handlerRetry, 3, self.callFailure)
    self._verifyStats(self.CAT, failure=1, retry=3) # 3 retries = 4 attempts.

    # Fail twice with exhausted retries.
    self.assertRaises(TestRetryException,
                      retry_stats.RetryWithStats,
                      self.CAT, self.handlerRetry, 3, self.callFailure)
    self._verifyStats(self.CAT, failure=2, retry=6)
