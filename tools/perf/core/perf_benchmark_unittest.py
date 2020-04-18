# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import shutil
import tempfile
import unittest

from telemetry import decorators
from telemetry.testing import options_for_unittests

from core import perf_benchmark


class PerfBenchmarkTest(unittest.TestCase):
  def setUp(self):
    self._output_dir = tempfile.mkdtemp()

  def tearDown(self):
    shutil.rmtree(self._output_dir, ignore_errors=True)

  def _ExpectAdTaggingProfileFiles(self, browser_options, expect_present):
    files_to_copy = browser_options.profile_files_to_copy

    local_state_to_copy = [
        (s, d) for (s, d) in files_to_copy if d == 'Local State']
    ruleset_data_to_copy = [
        (s, d) for (s, d) in files_to_copy if d.endswith('Ruleset Data')]

    num_expected_matches = 1 if expect_present else 0
    self.assertEqual(num_expected_matches, len(local_state_to_copy))
    self.assertEqual(num_expected_matches, len(ruleset_data_to_copy))


  @decorators.Disabled('chromeos')  # http://crbug.com/844863
  def testVariationArgs(self):
    benchmark = perf_benchmark.PerfBenchmark()
    options = options_for_unittests.GetCopy()
    benchmark.CustomizeBrowserOptions(options.browser_options)
    extra_args = options.browser_options.extra_browser_args
    feature_args = [a for a in extra_args if a.startswith('--enable-features')]
    self.assertEqual(1, len(feature_args))

  def testVariationArgsReference(self):
    benchmark = perf_benchmark.PerfBenchmark()
    options = options_for_unittests.GetCopy()
    options.browser_options.browser_type = 'reference'
    benchmark.CustomizeBrowserOptions(options.browser_options)

    extra_args = options.browser_options.extra_browser_args
    feature_args = [a for a in extra_args if a.startswith('--enable-features')]
    self.assertEqual(0, len(feature_args))

  def testNoAdTaggingRuleset(self):
    benchmark = perf_benchmark.PerfBenchmark()
    options = options_for_unittests.GetCopy()
    benchmark.CustomizeBrowserOptions(options.browser_options)
    self._ExpectAdTaggingProfileFiles(options.browser_options, False)

  def testAdTaggingRulesetReference(self):
    os.makedirs(os.path.join(
        self._output_dir, 'gen', 'components', 'subresource_filter',
        'tools','GeneratedRulesetData'))

    benchmark = perf_benchmark.PerfBenchmark()
    options = options_for_unittests.GetCopy()
    options.browser_options.browser_type = 'reference'

    # Careful, do not parse the command line flag for 'chromium-output-dir', as
    # that sets the global os environment variable CHROMIUM_OUTPUT_DIR,
    # affecting other tests. See http://crbug.com/843994.
    options.chromium_output_dir = self._output_dir

    benchmark.CustomizeBrowserOptions(options.browser_options)
    self._ExpectAdTaggingProfileFiles(options.browser_options, False)

  def testAdTaggingRuleset(self):
    os.makedirs(os.path.join(
        self._output_dir, 'gen', 'components', 'subresource_filter',
        'tools','GeneratedRulesetData'))

    benchmark = perf_benchmark.PerfBenchmark()
    options = options_for_unittests.GetCopy()

    # Careful, do not parse the command line flag for 'chromium-output-dir', as
    # that sets the global os environment variable CHROMIUM_OUTPUT_DIR,
    # affecting other tests. See http://crbug.com/843994.
    options.chromium_output_dir = self._output_dir

    benchmark.CustomizeBrowserOptions(options.browser_options)
    self._ExpectAdTaggingProfileFiles(options.browser_options, True)
