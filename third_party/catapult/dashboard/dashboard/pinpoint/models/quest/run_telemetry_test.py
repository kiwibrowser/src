# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Quest for running a Telemetry benchmark in Swarming."""

import copy

from dashboard.pinpoint.models.quest import run_test


_DEFAULT_EXTRA_ARGS = [
    '-v', '--upload-results', '--output-format', 'histograms']


class RunTelemetryTest(run_test.RunTest):

  def Start(self, change, isolate_server, isolate_hash):
    # For results2 to differentiate between runs, we need to add the
    # Telemetry parameter `--results-label <change>` to the runs.
    extra_args = copy.copy(self._extra_args)
    extra_args += ('--results-label', str(change))

    return self._Start(change, isolate_server, isolate_hash, extra_args)

  @classmethod
  def FromDict(cls, arguments):
    swarming_extra_args = []

    benchmark = arguments.get('benchmark')
    if not benchmark:
      raise TypeError('Missing "benchmark" argument.')
    if arguments.get('target') == 'performance_test_suite':
      swarming_extra_args += ('--benchmarks', benchmark)
    else:
      # TODO: Remove this hack when all builders build performance_test_suite.
      swarming_extra_args.append(benchmark)

    story = arguments.get('story')
    if story:
      swarming_extra_args += ('--story-filter', story)

    # TODO: Workaround for crbug.com/677843.
    if (benchmark.startswith('startup.warm') or
        benchmark.startswith('start_with_url.warm')):
      swarming_extra_args += ('--pageset-repeat', '2')
    else:
      swarming_extra_args += ('--pageset-repeat', '1')

    browser = arguments.get('browser')
    if not browser:
      raise TypeError('Missing "browser" argument.')
    swarming_extra_args += ('--browser', browser)

    if browser == 'android-webview':
      # TODO: Share code with the perf waterfall configs. crbug.com/771680
      swarming_extra_args += ('--webview-embedder-apk',
                              '../../out/Release/apks/SystemWebViewShell.apk')

    return cls._FromDict(arguments, swarming_extra_args + _DEFAULT_EXTRA_ARGS)
