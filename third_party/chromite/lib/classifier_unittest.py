# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module that contains unittests for classifier module."""

from __future__ import print_function

import os

from chromite.lib import classifier
from chromite.lib import constants
from chromite.lib import cros_test_lib
from chromite.lib import osutils


class TestClassifyLabJobs(cros_test_lib.TestCase):
  """Tests ClassifyRepairFailure."""

  def fetchTestLogs(self, logname):
    """Helper method to fetch a log from testdata.

    Args:
      logname: string basename of the logfile.

    Returns:
      Iterable of log lines as strings.
    """
    testFileName = os.path.join(
        constants.CHROMITE_DIR, 'lib', 'testdata', 'classifier', logname)
    return osutils.ReadFile(testFileName).splitlines()

  def testRegEx(self):
    """Validate our regular expression against a number of known strings."""

    # cases is a list of tuples (<logline>, <expected>).
    TEST_CASES = [
        ('START  ----  repair  timestamp=1488283838  localtime=Feb 28 04:10:38',
         ('START', None, 'repair', '')),

        (' GOOD  ----  verify.ssh  timestamp=1488283840  '
         'localtime=Feb 28 04:10:40 ',
         (None, 'GOOD', 'verify.ssh', '')),

        ('  FAIL  ----  verify.ssh  timestamp=1488284562  '
         'localtime=Feb 28 04:22:42 No answer to ping from '
         'chromeos4-row9-rack7-host1',
         (None, 'FAIL', 'verify.ssh',
          'No answer to ping from chromeos4-row9-rack7-host1')),

        ('  END FAIL  ----  repair.rpm  timestamp=1488284808  '
         'localtime=Feb 28 04:26:48',
         ('END', 'FAIL', 'repair.rpm', '')),

        ('    INFO  ----  Orphaned Crash Dump timestamp=1488285072  '
         'localtime=Feb 28 04:31:12 '
         '/var/spool/crash/kernel_warning.20170228.033751.0.meta',
         None),
    ]

    for line, expected in TEST_CASES:
      result = classifier.ExtractLabStatusParts(line)
      self.assertEqual(result, expected,
                       'LINE: %s\n%s != %s' % (line, result, expected))

  def testNoopRepair(self):
    """Sometimes repairs don't have to do anything at all."""
    repair_status_log = self.fetchTestLogs('noop-repair-status.log')

    result = classifier.ClassifyLabJobStatusResult(repair_status_log)

    self.assertEqual(result, 'repair: GOOD: NoRepairNeeded')

  def testNoopRepairNoServo(self):
    """Sometimes repairs don't have to do anything at all."""
    repair_status_log = self.fetchTestLogs('noop-repair-status-no-servo.log')

    result = classifier.ClassifyLabJobStatusResult(repair_status_log)

    self.assertEqual(result, 'repair: GOOD: NoRepairNeeded')

  def testUsbRepair(self):
    """A successful repair with the network down, and recovery used."""
    repair_status_log = self.fetchTestLogs('usb-repair-status.log')

    result = classifier.ClassifyLabJobStatusResult(repair_status_log)

    self.assertEqual(result, 'repair: GOOD: NetworkWasDown, USB')

  def testNetDownRepair(self):
    """A successful repair with the network down."""
    repair_status_log = self.fetchTestLogs('net-down-repair-status.log')

    result = classifier.ClassifyLabJobStatusResult(repair_status_log)

    self.assertEqual(result, 'repair: GOOD: NetworkWasDown')

  def testFailRepair(self):
    """Test a repair that failed."""
    repair_status_log = self.fetchTestLogs('fail-repair-status.log')

    result = classifier.ClassifyLabJobStatusResult(repair_status_log)

    self.assertEqual(result, 'repair: FAIL: NetworkWasDown, USB')

  def testGoodProvision(self):
    """Test a provision that pass."""
    repair_status_log = self.fetchTestLogs('good-provision-status.log')

    result = classifier.ClassifyLabJobStatusResult(repair_status_log)

    self.assertEqual(result, 'provision: GOOD: No details extracted.')

  def testBadProvision(self):
    """Test a provision that pass."""
    repair_status_log = self.fetchTestLogs('bad-provision-status.log')

    result = classifier.ClassifyLabJobStatusResult(repair_status_log)

    self.assertEqual(result, 'provision: FAIL: No details extracted.')
