# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import os
import subprocess
import sys
import unittest

from telemetry import decorators
from telemetry.testing import options_for_unittests


class ScriptsSmokeTest(unittest.TestCase):

  perf_dir = os.path.dirname(__file__)

  def RunPerfScript(self, command):
    main_command = [sys.executable]
    args = main_command + command.split(' ')
    proc = subprocess.Popen(args, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, cwd=self.perf_dir)
    stdout = proc.communicate()[0]
    return_code = proc.returncode
    return return_code, stdout

  def testRunBenchmarkHelp(self):
    return_code, stdout = self.RunPerfScript('run_benchmark help')
    self.assertEquals(return_code, 0, stdout)
    self.assertIn('Available commands are', stdout)

  def testRunBenchmarkRunListsOutBenchmarks(self):
    return_code, stdout = self.RunPerfScript('run_benchmark run')
    self.assertIn('Pass --browser to list benchmarks', stdout)
    self.assertNotEquals(return_code, 0)

  def testRunBenchmarkRunNonExistingBenchmark(self):
    return_code, stdout = self.RunPerfScript('run_benchmark foo')
    self.assertIn('No benchmark named "foo"', stdout)
    self.assertNotEquals(return_code, 0)

  def testRunRecordWprHelp(self):
    return_code, stdout = self.RunPerfScript('record_wpr')
    self.assertEquals(return_code, 0, stdout)
    self.assertIn('optional arguments:', stdout)

  @decorators.Disabled('chromeos')  # crbug.com/814068
  def testRunRecordWprList(self):
    return_code, stdout = self.RunPerfScript('record_wpr --list-benchmarks')
    # TODO(nednguyen): Remove this once we figure out why importing
    # small_profile_extender fails on Android dbg.
    # crbug.com/561668
    if 'ImportError: cannot import name small_profile_extender' in stdout:
      self.skipTest('small_profile_extender is missing')
    self.assertEquals(return_code, 0, stdout)
    self.assertIn('kraken', stdout)

  @decorators.Disabled('chromeos')  # crbug.com/754913
  def testRunTelemetryBenchmarkAsGoogletest(self):
    options = options_for_unittests.GetCopy()
    browser_type = options.browser_type
    return_code, stdout = self.RunPerfScript(
        '../../testing/scripts/run_telemetry_benchmark_as_googletest.py '
        'run_benchmark dummy_benchmark.stable_benchmark_1 --browser=%s '
        '--isolated-script-test-output=output.json '
        '--isolated-script-test-chartjson-output=chartjson_output.json '
        '--output-format=chartjson' % browser_type)
    self.assertEquals(return_code, 0, stdout)
    try:
      with open('../../tools/perf/output.json') as f:
        self.assertIsNotNone(
            json.load(f), 'json_test_results should be populated: ' + stdout)
      os.remove('../../tools/perf/output.json')
    except IOError as e:
      self.fail('json_test_results should be populated: ' + stdout + str(e))
    try:
      with open('../../tools/perf/chartjson_output.json') as f:
        self.assertIsNotNone(
            json.load(f), 'chartjson should be populated: ' + stdout)
      os.remove('../../tools/perf/chartjson_output.json')
    except IOError as e:
      self.fail('chartjson should be populated: ' + stdout + str(e))
