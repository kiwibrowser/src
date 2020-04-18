# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from dashboard.pinpoint.models.quest import run_telemetry_test
from dashboard.pinpoint.models.quest import run_test


_BASE_ARGUMENTS = {
    'swarming_server': 'server',
    'dimensions': {'key': 'value'},
    'benchmark': 'speedometer',
    'browser': 'release',
}


_BASE_EXTRA_ARGS = [
    'speedometer', '--pageset-repeat', '1', '--browser', 'release',
] + run_telemetry_test._DEFAULT_EXTRA_ARGS + run_test._DEFAULT_EXTRA_ARGS


class StartTest(unittest.TestCase):

  def testStart(self):
    quest = run_telemetry_test.RunTelemetryTest(
        'server', {'key': 'value'}, ['arg'])
    execution = quest.Start('change', 'https://isolate.server', 'isolate hash')
    self.assertEqual(execution._extra_args,
                     ['arg', '--results-label', 'change'])


class FromDictTest(unittest.TestCase):

  def testMinimumArguments(self):
    quest = run_telemetry_test.RunTelemetryTest.FromDict(_BASE_ARGUMENTS)
    expected = run_telemetry_test.RunTelemetryTest(
        'server', {'key': 'value'}, _BASE_EXTRA_ARGS)
    self.assertEqual(quest, expected)

  def testAllArguments(self):
    arguments = dict(_BASE_ARGUMENTS)
    arguments['story'] = 'http://www.fifa.com/'
    quest = run_telemetry_test.RunTelemetryTest.FromDict(arguments)

    extra_args = [
        'speedometer', '--story-filter', 'http://www.fifa.com/',
        '--pageset-repeat', '1', '--browser', 'release',
    ] + run_telemetry_test._DEFAULT_EXTRA_ARGS + run_test._DEFAULT_EXTRA_ARGS
    expected = run_telemetry_test.RunTelemetryTest(
        'server', {'key': 'value'}, extra_args)
    self.assertEqual(quest, expected)

  def testMissingBenchmark(self):
    arguments = dict(_BASE_ARGUMENTS)
    del arguments['benchmark']
    with self.assertRaises(TypeError):
      run_telemetry_test.RunTelemetryTest.FromDict(arguments)

  def testBenchmarkWithPerformanceTestSuite(self):
    arguments = dict(_BASE_ARGUMENTS)
    arguments['target'] = 'performance_test_suite'
    quest = run_telemetry_test.RunTelemetryTest.FromDict(arguments)

    expected = run_telemetry_test.RunTelemetryTest(
        'server', {'key': 'value'}, ['--benchmarks'] + _BASE_EXTRA_ARGS)
    self.assertEqual(quest, expected)

  def testMissingBrowser(self):
    arguments = dict(_BASE_ARGUMENTS)
    del arguments['browser']
    with self.assertRaises(TypeError):
      run_telemetry_test.RunTelemetryTest.FromDict(arguments)

  def testStartupBenchmarkRepeatCount(self):
    arguments = dict(_BASE_ARGUMENTS)
    arguments['benchmark'] = 'start_with_url.warm.startup_pages'
    quest = run_telemetry_test.RunTelemetryTest.FromDict(arguments)

    extra_args = [
        'start_with_url.warm.startup_pages',
        '--pageset-repeat', '2', '--browser', 'release',
    ] + run_telemetry_test._DEFAULT_EXTRA_ARGS + run_test._DEFAULT_EXTRA_ARGS
    expected = run_telemetry_test.RunTelemetryTest(
        'server', {'key': 'value'}, extra_args)
    self.assertEqual(quest, expected)

  def testWebviewFlag(self):
    arguments = dict(_BASE_ARGUMENTS)
    arguments['browser'] = 'android-webview'
    quest = run_telemetry_test.RunTelemetryTest.FromDict(arguments)

    extra_args = [
        'speedometer', '--pageset-repeat', '1',
        '--browser', 'android-webview', '--webview-embedder-apk',
        '../../out/Release/apks/SystemWebViewShell.apk',
    ] + run_telemetry_test._DEFAULT_EXTRA_ARGS + run_test._DEFAULT_EXTRA_ARGS
    expected = run_telemetry_test.RunTelemetryTest(
        'server', {'key': 'value'}, extra_args)
    self.assertEqual(quest, expected)
