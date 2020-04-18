# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for perf_uploader module."""

from __future__ import print_function

import json
import os
import tempfile
import urllib2
import urlparse

from chromite.lib import cros_test_lib
from chromite.lib import perf_uploader
from chromite.lib import osutils


class PerfUploadTestCase(cros_test_lib.MockTestCase):
  """Base utility class to setup mock objects and temp file for tests."""

  def setUp(self):
    presentation_info = perf_uploader.PresentationInfo(
        master_name='ChromeOSPerf',
        test_name='TestName',
    )
    self.PatchObject(perf_uploader, '_GetPresentationInfo',
                     return_value=presentation_info)
    self.file_name = tempfile.NamedTemporaryFile().name

  def tearDown(self):
    osutils.SafeUnlink(self.file_name)


class OutputPerfValueTest(PerfUploadTestCase):
  """Test function OutputPerfValue."""

  def testInvalidDescription(self):
    perf_uploader.OutputPerfValue(self.file_name, 'a' * 256, 0, 'ignored')
    self.assertRaises(ValueError, perf_uploader.OutputPerfValue,
                      'ignored', 'a' * 257, 0, 'ignored')

    perf_uploader.OutputPerfValue(self.file_name, 'abcXYZ09-_.', 0, 'ignored')
    self.assertRaises(ValueError, perf_uploader.OutputPerfValue,
                      'ignored', 'a\x00c', 0, 'ignored')

  def testInvalidUnits(self):
    self.assertRaises(ValueError, perf_uploader.OutputPerfValue,
                      'ignored', 'ignored', 0, 'a' * 257)
    self.assertRaises(ValueError, perf_uploader.OutputPerfValue,
                      'ignored', 'ignored', 0, 'a\x00c')

  def testValidJson(self):
    perf_uploader.OutputPerfValue(self.file_name, 'desc', 42, 'units')
    data = osutils.ReadFile(self.file_name)
    entry = json.loads(data)
    self.assertTrue(isinstance(entry, dict))


class LoadPerfValuesTest(PerfUploadTestCase):
  """Test function LoadPerfValues."""

  def testEmptyFile(self):
    osutils.WriteFile(self.file_name, '')
    entries = perf_uploader.LoadPerfValues(self.file_name)
    self.assertEqual(0, len(entries))

  def testLoadOneValue(self):
    perf_uploader.OutputPerfValue(self.file_name, 'desc', 41, 'units')
    entries = perf_uploader.LoadPerfValues(self.file_name)
    self.assertEqual(1, len(entries))
    self.assertEqual(41, entries[0].value)
    self.assertEqual('desc', entries[0].description)
    self.assertEqual(True, entries[0].higher_is_better)

  def testLoadTwoValues(self):
    perf_uploader.OutputPerfValue(self.file_name, 'desc', 41, 'units')
    perf_uploader.OutputPerfValue(self.file_name, 'desc2', 42, 'units2')
    entries = perf_uploader.LoadPerfValues(self.file_name)
    self.assertEqual(2, len(entries))
    self.assertEqual(41, entries[0].value)
    self.assertEqual(42, entries[1].value)
    self.assertEqual('desc2', entries[1].description)
    self.assertEqual(None, entries[1].graph)


class SendToDashboardTest(PerfUploadTestCase):
  """Ensure perf values are sent to chromeperf via HTTP."""

  def setUp(self):
    self.urlopen = self.PatchObject(urllib2, 'urlopen')

  def testOneEntry(self):
    perf_uploader.OutputPerfValue(self.file_name, 'desc1', 42, 'unit')
    perf_values = perf_uploader.LoadPerfValues(self.file_name)
    perf_uploader.UploadPerfValues(perf_values, 'platform', 'TestName',
                                   cros_version='cros', chrome_version='chrome')
    request = self.urlopen.call_args[0][0]
    self.assertEqual(os.path.join(perf_uploader.DASHBOARD_URL, 'add_point'),
                     request.get_full_url())
    data = request.get_data()
    data = urlparse.parse_qs(data)['data']
    entries = [json.loads(x) for x in data]
    entry = entries[0][0]
    self.assertEqual('cros', entry['supplemental_columns']['r_cros_version'])
    self.assertEqual(42, entry['value'])
    self.assertEqual('cbuildbot.TestName/desc1', entry['test'])
    self.assertEqual('unit', entry['units'])

  def testCustomDashboard(self):
    """Verify we can set data to different dashboards."""
    perf_uploader.OutputPerfValue(self.file_name, 'desc1', 42, 'unit')
    perf_values = perf_uploader.LoadPerfValues(self.file_name)
    perf_uploader.UploadPerfValues(perf_values, 'platform', 'TestName',
                                   cros_version='cros', chrome_version='chrome',
                                   dashboard='http://localhost')
    request = self.urlopen.call_args[0][0]
    self.assertEqual('http://localhost/add_point', request.get_full_url())


class UploadPerfValuesTest(PerfUploadTestCase):
  """Test UploadPerfValues function."""

  def setUp(self):
    self.send_func = self.PatchObject(perf_uploader, '_SendToDashboard')

  def testOneEntry(self):
    """Upload one perf value."""
    perf_uploader.OutputPerfValue(self.file_name, 'desc1', 42, 'unit')
    perf_values = perf_uploader.LoadPerfValues(self.file_name)
    perf_uploader.UploadPerfValues(perf_values, 'platform', 'TestName',
                                   cros_version='cros', chrome_version='chrome')
    positional_args, _ = self.send_func.call_args
    first_param = positional_args[0]
    data = json.loads(first_param['data'])
    self.assertEqual(1, len(data))
    entry = data[0]
    self.assertEqual('unit', entry['units'])
    self.assertEqual('cros',
                     entry['supplemental_columns']['r_cros_version'])
    self.assertEqual('chrome',
                     entry['supplemental_columns']['r_chrome_version'])
    self.assertEqual('cros-platform', entry['bot'])
    self.assertEqual(42, entry['value'])
    self.assertEqual(0, entry['error'])

  def testRevision(self):
    """Verify revision is accepted over cros/chrome version."""
    perf_uploader.OutputPerfValue(self.file_name, 'desc1', 42, 'unit')
    perf_values = perf_uploader.LoadPerfValues(self.file_name)
    perf_uploader.UploadPerfValues(perf_values, 'platform', 'TestName',
                                   revision=12345)
    positional_args, _ = self.send_func.call_args
    first_param = positional_args[0]
    data = json.loads(first_param['data'])
    entry = data[0]
    self.assertEqual(12345, entry['revision'])

  def testTwoEntriesOfSameTest(self):
    """Upload one test, two perf values."""
    perf_uploader.OutputPerfValue(self.file_name, 'desc1', 40, 'unit')
    perf_uploader.OutputPerfValue(self.file_name, 'desc1', 42, 'unit')
    perf_values = perf_uploader.LoadPerfValues(self.file_name)
    perf_uploader.UploadPerfValues(perf_values, 'platform', 'TestName',
                                   cros_version='cros', chrome_version='chrome')
    positional_args, _ = self.send_func.call_args
    first_param = positional_args[0]
    data = json.loads(first_param['data'])
    self.assertEqual(1, len(data))
    entry = data[0]
    self.assertEqual('unit', entry['units'])
    # Average of 40 and 42
    self.assertEqual(41, entry['value'])
    # Standard deviation sqrt(2)
    self.assertEqual(1.4142, entry['error'])

  def testTwoTests(self):
    """Upload two tests, one perf value each."""
    perf_uploader.OutputPerfValue(self.file_name, 'desc1', 40, 'unit')
    perf_uploader.OutputPerfValue(self.file_name, 'desc2', 42, 'unit')
    perf_values = perf_uploader.LoadPerfValues(self.file_name)
    perf_uploader.UploadPerfValues(perf_values, 'platform', 'TestName',
                                   cros_version='cros', chrome_version='chrome')
    positional_args, _ = self.send_func.call_args
    first_param = positional_args[0]
    data = json.loads(first_param['data'])
    self.assertEqual(2, len(data))
    data = sorted(data, key=lambda x: x['test'])
    entry = data[0]
    self.assertEqual(40, entry['value'])
    self.assertEqual(0, entry['error'])
    entry = data[1]
    self.assertEqual(42, entry['value'])
    self.assertEqual(0, entry['error'])

  def testTwoTestsThreeEntries(self):
    """Upload two tests, one perf value each."""
    perf_uploader.OutputPerfValue(self.file_name, 'desc1', 40, 'unit')
    perf_uploader.OutputPerfValue(self.file_name, 'desc1', 42, 'unit')
    perf_uploader.OutputPerfValue(self.file_name, 'desc2', 42, 'unit')
    perf_values = perf_uploader.LoadPerfValues(self.file_name)
    perf_uploader.UploadPerfValues(perf_values, 'platform', 'TestName',
                                   cros_version='cros', chrome_version='chrome')
    positional_args, _ = self.send_func.call_args
    first_param = positional_args[0]
    data = json.loads(first_param['data'])
    self.assertEqual(2, len(data))
    data = sorted(data, key=lambda x: x['test'])
    entry = data[0]
    self.assertEqual(41, entry['value'])
    self.assertEqual(1.4142, entry['error'])
    entry = data[1]
    self.assertEqual(42, entry['value'])
    self.assertEqual(0, entry['error'])

  def testDryRun(self):
    """Make sure dryrun mode doesn't upload."""
    self.send_func.side_effect = AssertionError('dryrun should not upload')
    perf_uploader.OutputPerfValue(self.file_name, 'desc1', 40, 'unit')
    perf_values = perf_uploader.LoadPerfValues(self.file_name)
    perf_uploader.UploadPerfValues(perf_values, 'platform', 'TestName',
                                   cros_version='cros', chrome_version='chrome',
                                   dry_run=True)
