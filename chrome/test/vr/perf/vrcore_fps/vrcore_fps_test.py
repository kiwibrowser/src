# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Visits various WebVR app URLs and records performance metrics from VrCore.
"""

import android_vr_perf_test
import vr_perf_summary

import json
import logging
import numpy
import os
import time

DEFAULT_URLS = [
  # Standard WebVR sample app with no changes.
  'https://webvr.info/samples/test-slow-render.html?'
  'canvasClickPresents=1\&renderScale=1',
  # Increased render scale.
  'https://webvr.info/samples/test-slow-render.html?'
  'canvasClickPresents=1\&renderScale=1.5',
  # Default render scale, but increased load.
  'https://webvr.info/samples/test-slow-render.html?'
  'canvasClickPresents=1\&'
  'renderScale=1\&heavyGpu=1\&cubeScale=0.2\&workTime=5',
  # Further increased load.
  'https://webvr.info/samples/test-slow-render.html?'
  'canvasClickPresents=1\&'
  'renderScale=1\&heavyGpu=1\&cubeScale=0.3\&workTime=10',
]


class VrCoreFpsTest(android_vr_perf_test.AndroidVrPerfTest):
  def __init__(self, args):
    super(VrCoreFpsTest, self).__init__(args)
    self._duration = args.duration
    assert (self._duration > 0),'Duration must be positive'
    self._test_urls = args.urls or DEFAULT_URLS
    self._test_results = {}

  def _Run(self, url):
    # We're already in VR and logcat is cleared during setup, so wait for
    # the test duration.
    time.sleep(self._duration)

    # Exit VR so that VrCore stops logging performance data.
    self._Adb(['shell', 'input', 'keyevent', 'KEYCODE_BACK'])
    time.sleep(1)

    output = self._Adb(['logcat', '-d'])
    logging_sessions = vr_perf_summary.ParseLinesIntoSessions(output)
    if len(logging_sessions) != 1:
      raise RuntimeError('Expected 1 VR session, found %d' %
                         len(logging_sessions))
    session = logging_sessions[0]
    if len(session) == 0:
      raise RuntimeError('No data actually collected in logging session')
    self._StoreResults(url, vr_perf_summary.ComputeSessionStatistics(session))

  def _StoreResults(self, url, results):
    """Temporarily stores the results of a test.

    Stores the given results in memory to be later retrieved and written to
    a file in _SaveResultsToFile once all tests are done. Also logs the raw
    data and calculated statistics.
    """
    logging.info('\nURL: %s\n'
                 'Raw app FPS: %s\n'
                 'Raw asynchronous reprojection thread FPS: %s\n'
                 'Raw asynchronous reprojection thread missed frames: %s\n'
                 'Raw frame submissions blocked on gpu: %s\n'
                 '%s\n' %
                 (url, str(results[vr_perf_summary.APP_FPS_KEY]),
                  str(results[vr_perf_summary.ASYNC_FPS_KEY]),
                  str(results[vr_perf_summary.ASYNC_MISSED_KEY]),
                  str(results[vr_perf_summary.BLOCKED_SUBMISSION_KEY]),
                  vr_perf_summary.StringifySessionStatistics(results)))
    self._test_results[url] = results

  def _SaveResultsToFile(self):
    outpath = None
    if (hasattr(self._args, 'isolated_script_test_perf_output') and
        self._args.isolated_script_test_perf_output):
      outpath = self._args.isolated_script_test_perf_output
    elif (hasattr(self._args, 'isolated_script_test_chartjson_output') and
        self._args.isolated_script_test_chartjson_output):
      outpath = self._args.isolated_script_test_chartjson_output
    else:
      logging.warning('No output file set, not saving results to file')
      return

    # TODO(bsheedy): Move this to a common place so other tests can use it.
    def _GenerateTrace(name, improvement_direction, std=None,
                       value_type=None, units=None, values=None):
      return {
          'improvement_direction': improvement_direction,
          'name': name,
          'std': std or 0.0,
          'type': value_type or 'list_of_scalar_values',
          'units': units or '',
          'values': values or [],
      }

    def _GenerateBaseChart(name, improvement_direction, std=None,
                           value_type=None, units=None, values=None):
      return {'summary': _GenerateTrace(name, improvement_direction, std=std,
                                        value_type=value_type, units=units,
                                        values=values)}

    registered_charts = {}
    def _RegisterChart(name, improvement_direction, value_func, std_func,
                       results_key, units=None):
      registered_charts[self._device_name + name] = {
          'improvement_direction': improvement_direction,
          'value_func': value_func,
          'std_func': std_func,
          'results_key': results_key,
          'units': units or ''
      }

    # value/summary functions.
    def f_identity(x): return x
    def f_min(x): return [min(x)]
    def f_max(x): return [max(x)]
    # std functions.
    def f_zero(x): return 0.0
    def f_numpy(x): return numpy.std(x)

    _RegisterChart('_avg_app_fps', 'up', f_identity, f_numpy,
                   vr_perf_summary.APP_FPS_KEY, units='FPS')
    _RegisterChart('_min_app_fps', 'up', f_min, f_zero,
                   vr_perf_summary.APP_FPS_KEY, units='FPS')
    _RegisterChart('_max_app_fps', 'up', f_max, f_zero,
                   vr_perf_summary.APP_FPS_KEY, units='FPS')
    _RegisterChart('_avg_async_thread_fps', 'up', f_identity, f_numpy,
                   vr_perf_summary.ASYNC_FPS_KEY, units='FPS')
    _RegisterChart('_min_async_thread_fps', 'up', f_min, f_zero,
                   vr_perf_summary.ASYNC_FPS_KEY, units='FPS')
    _RegisterChart('_max_async_thread_fps', 'up', f_max, f_zero,
                   vr_perf_summary.ASYNC_FPS_KEY, units='FPS')
    # Unlike the rest of the data, missed async reprojection thread vsyncs are
    # only logged when they happen so it's normal to get an empty list, which
    # makes numpy and the perf dashboard unhappy.
    _RegisterChart('_avg_async_thread_vsync_miss_time', 'down',
                   lambda x: x or [0.0],
                   lambda x: 0.0 if len(x) == 0 else numpy.std(x),
                   vr_perf_summary.ASYNC_MISSED_KEY, units='ms')
    _RegisterChart('_num_async_thread_vsync_misses', 'down', lambda x: [len(x)],
                   f_zero, vr_perf_summary.ASYNC_MISSED_KEY)
    _RegisterChart('_num_blocked_frame_submissions', 'down', lambda x: [sum(x)],
                   f_zero, vr_perf_summary.BLOCKED_SUBMISSION_KEY)

    charts = {}
    # Set up empty summary charts.
    for chart, config in registered_charts.iteritems():
      charts[chart] = _GenerateBaseChart(chart, config['improvement_direction'],
                                         units=config['units'])

    # Populate a trace for each URL in each chart.
    for url, results in self._test_results.iteritems():
      for chart, config in registered_charts.iteritems():
        result = results[config['results_key']]
        charts[chart][url] = (
            _GenerateTrace(chart, config['improvement_direction'],
                           units=config['units'],
                           std=config['std_func'](result),
                           values=config['value_func'](result)))
        charts[chart]['summary']['values'].extend(config['value_func'](result))

    results = {
        'format_version': '1.0',
        'benchmarck_name': 'vrcore_fps',
        'benchmark_description': 'Measures VR FPS using VrCore perf logging',
        'charts': charts,
    }

    with file(outpath, 'w') as outfile:
      json.dump(results, outfile)
