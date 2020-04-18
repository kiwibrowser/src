# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Pull a sandwich run's output directory's metrics from traces into a CSV.

python pull_sandwich_metrics.py -h
"""

import collections
import json
import logging
import os
import shutil
import subprocess
import sys
import tempfile

_SRC_DIR = os.path.abspath(os.path.join(
    os.path.dirname(__file__), '..', '..', '..'))

sys.path.append(os.path.join(_SRC_DIR, 'tools', 'perf'))
from core import path_util
sys.path.append(path_util.GetTelemetryDir())

from telemetry.internal.image_processing import video
from telemetry.util import image_util
from telemetry.util import rgba_color

import common_util
import loading_trace as loading_trace_module
import sandwich_runner
import tracing_track


COMMON_CSV_COLUMN_NAMES = [
    'chromium_commit',
    'platform',
    'first_layout',
    'first_contentful_paint',
    'first_meaningful_paint',
    'total_load',
    'js_onload_event',
    'browser_malloc_avg',
    'browser_malloc_max',
    'speed_index',
    'net_emul.name', # Should be in emulation.NETWORK_CONDITIONS.keys()
    'net_emul.download',
    'net_emul.upload',
    'net_emul.latency']

_UNAVAILABLE_CSV_VALUE = 'unavailable'

_FAILED_CSV_VALUE = 'failed'

_TRACKED_EVENT_NAMES = set([
    'requestStart',
    'loadEventStart',
    'loadEventEnd',
    'firstContentfulPaint',
    'firstLayout'])

# Points of a completeness record.
#
# Members:
#   |time| is in milliseconds,
#   |frame_completeness| value representing how complete the frame is at a given
#     |time|. Caution: this completeness might be negative.
CompletenessPoint = collections.namedtuple('CompletenessPoint',
    ('time', 'frame_completeness'))


def _GetBrowserPID(track):
  """Get the browser PID from a trace.

  Args:
    track: The tracing_track.TracingTrack.

  Returns:
    The browser's PID as an integer.
  """
  for event in track.GetEvents():
    if event.category != '__metadata' or event.name != 'process_name':
      continue
    if event.args['name'] == 'Browser':
      return event.pid
  raise ValueError('couldn\'t find browser\'s PID')


def _GetBrowserDumpEvents(track):
  """Get the browser memory dump events from a tracing track.

  Args:
    track: The tracing_track.TracingTrack.

  Returns:
    List of memory dump events.
  """
  assert sandwich_runner.MEMORY_DUMP_CATEGORY in track.Categories()
  browser_pid = _GetBrowserPID(track)
  browser_dumps_events = []
  for event in track.GetEvents():
    if event.category != 'disabled-by-default-memory-infra':
      continue
    if event.type != 'v' or event.name != 'periodic_interval':
      continue
    # Ignore dump events for processes other than the browser process
    if event.pid != browser_pid:
      continue
    browser_dumps_events.append(event)
  if len(browser_dumps_events) == 0:
    raise ValueError('No browser dump events found.')
  return browser_dumps_events


def _GetWebPageTrackedEvents(track):
  """Get the web page's tracked events from a tracing track.

  Args:
    track: The tracing_track.TracingTrack.

  Returns:
    A dict mapping event.name -> tracing_track.Event for each first occurrence
        of a tracked event.
  """
  main_frame_id = None
  tracked_events = {}
  sorted_events = sorted(track.GetEvents(),
                         key=lambda event: event.start_msec)
  for event in sorted_events:
    if event.category != 'blink.user_timing':
      continue
    event_name = event.name

    # Find the id of the main frame. Skip all events until it is found.
    if not main_frame_id:
      # Tracing (in Sandwich) is started after about:blank is fully loaded,
      # hence the first navigationStart in the trace registers the correct frame
      # id.
      if event_name == 'navigationStart':
        logging.info('  Found navigationStart at: %f', event.start_msec)
        main_frame_id = event.args['frame']
      continue

    # Ignore events with frame id attached, but not being the main frame.
    if 'frame' in event.args and event.args['frame'] != main_frame_id:
      continue

    # Capture trace events by the first time of their appearance. Note: some
    # important events (like requestStart) do not have a frame id attached.
    if event_name in _TRACKED_EVENT_NAMES and event_name not in tracked_events:
      tracked_events[event_name] = event
      logging.info('  Event %s first appears at: %f', event_name,
          event.start_msec)
  return tracked_events


def _ExtractDefaultMetrics(loading_trace):
  """Extracts all the default metrics from a given trace.

  Args:
    loading_trace: loading_trace.LoadingTrace.

  Returns:
    Dictionary with all trace extracted fields set.
  """
  END_REQUEST_EVENTS = [
      ('first_layout', 'requestStart', 'firstLayout'),
      ('first_contentful_paint', 'requestStart', 'firstContentfulPaint'),
      ('total_load', 'requestStart', 'loadEventEnd'),
      ('js_onload_event', 'loadEventStart', 'loadEventEnd')]
  web_page_tracked_events = _GetWebPageTrackedEvents(
      loading_trace.tracing_track)
  metrics = {}
  for metric_name, start_event_name, end_event_name in END_REQUEST_EVENTS:
    try:
      metrics[metric_name] = (
          web_page_tracked_events[end_event_name].start_msec -
          web_page_tracked_events[start_event_name].start_msec)
    except KeyError as error:
      logging.error('could not extract metric %s: missing trace event: %s' % (
          metric_name, str(error)))
      metrics[metric_name] = _FAILED_CSV_VALUE
  return metrics


def _ExtractTimeToFirstMeaningfulPaint(loading_trace):
  """Extracts the time to first meaningful paint from a given trace.

  Args:
    loading_trace: loading_trace_module.LoadingTrace.

  Returns:
    Time to first meaningful paint in milliseconds.
  """
  required_categories = set(sandwich_runner.TTFMP_ADDITIONAL_CATEGORIES)
  if not required_categories.issubset(loading_trace.tracing_track.Categories()):
    return _UNAVAILABLE_CSV_VALUE
  logging.info('  Extracting first_meaningful_paint')
  events = [e.ToJsonDict() for e in loading_trace.tracing_track.GetEvents()]
  with common_util.TemporaryDirectory(prefix='sandwich_tmp_') as tmp_dir:
    chrome_trace_path = os.path.join(tmp_dir, 'chrome_trace.json')
    with open(chrome_trace_path, 'w') as output_file:
      json.dump({'traceEvents': events, 'metadata': {}}, output_file)
    catapult_run_metric_bin_path = os.path.join(
        _SRC_DIR, 'third_party', 'catapult', 'tracing', 'bin', 'run_metric')
    output = subprocess.check_output(
        [catapult_run_metric_bin_path, 'firstPaintMetric', chrome_trace_path])
  json_output = json.loads(output)
  for metric in json_output[chrome_trace_path]['pairs']['values']:
    if metric['name'] == 'firstMeaningfulPaint_avg':
      return metric['numeric']['value']
  logging.info('  Extracting first_meaningful_paint: failed')
  return _FAILED_CSV_VALUE


def _ExtractMemoryMetrics(loading_trace):
  """Extracts all the memory metrics from a given trace.

  Args:
    loading_trace: loading_trace_module.LoadingTrace.

  Returns:
    Dictionary with all trace extracted fields set.
  """
  if (sandwich_runner.MEMORY_DUMP_CATEGORY not in
          loading_trace.tracing_track.Categories()):
    return {
      'browser_malloc_avg': _UNAVAILABLE_CSV_VALUE,
      'browser_malloc_max': _UNAVAILABLE_CSV_VALUE
    }
  browser_dump_events = _GetBrowserDumpEvents(loading_trace.tracing_track)
  browser_malloc_sum = 0
  browser_malloc_max = 0
  for dump_event in browser_dump_events:
    attr = dump_event.args['dumps']['allocators']['malloc']['attrs']['size']
    assert attr['units'] == 'bytes'
    size = int(attr['value'], 16)
    browser_malloc_sum += size
    browser_malloc_max = max(browser_malloc_max, size)
  return {
    'browser_malloc_avg': browser_malloc_sum / float(len(browser_dump_events)),
    'browser_malloc_max': browser_malloc_max
  }


def _ExtractCompletenessRecordFromVideo(video_path):
  """Extracts the completeness record from a video.

  The video must start with a filled rectangle of orange (RGB: 222, 100, 13), to
  give the view-port size/location from where to compute the completeness.

  Args:
    video_path: Path of the video to extract the completeness list from.

  Returns:
    list(CompletenessPoint)
  """
  video_file = tempfile.NamedTemporaryFile()
  shutil.copy(video_path, video_file.name)
  video_capture = video.Video(video_file)

  histograms = [
      (time, image_util.GetColorHistogram(
          image, ignore_color=rgba_color.WHITE, tolerance=8))
      for time, image in video_capture.GetVideoFrameIter()
  ]

  start_histogram = histograms[1][1]
  final_histogram = histograms[-1][1]
  total_distance = start_histogram.Distance(final_histogram)

  def FrameProgress(histogram):
    if total_distance == 0:
      if histogram.Distance(final_histogram) == 0:
        return 1.0
      else:
        return 0.0
    return 1 - histogram.Distance(final_histogram) / total_distance

  return [(time, FrameProgress(hist)) for time, hist in histograms]


def _ComputeSpeedIndex(completeness_record):
  """Computes the speed-index from a completeness record.

  Args:
    completeness_record: list(CompletenessPoint)

  Returns:
    Speed-index value.
  """
  speed_index = 0.0
  last_time = completeness_record[0][0]
  last_completness = completeness_record[0][1]
  for time, completeness in completeness_record:
    if time < last_time:
      raise ValueError('Completeness record must be sorted by timestamps.')
    elapsed = time - last_time
    speed_index += elapsed * (1.0 - last_completness)
    last_time = time
    last_completness = completeness
  return speed_index


def ExtractCommonMetricsFromRepeatDirectory(repeat_dir, trace):
  """Extracts all the metrics from traces and video of a sandwich run repeat
  directory.

  Args:
    repeat_dir: Path of the repeat directory within a run directory.
    trace: preloaded LoadingTrace in |repeat_dir|

  Contract:
    trace == LoadingTrace.FromJsonFile(
        os.path.join(repeat_dir, sandwich_runner.TRACE_FILENAME))

  Returns:
    Dictionary of extracted metrics.
  """
  run_metrics = {
      'chromium_commit': trace.metadata['chromium_commit'],
      'platform': (trace.metadata['platform']['os'] + '-' +
          trace.metadata['platform']['product_model'])
  }
  run_metrics.update(_ExtractDefaultMetrics(trace))
  run_metrics.update(_ExtractMemoryMetrics(trace))
  run_metrics['first_meaningful_paint'] = _ExtractTimeToFirstMeaningfulPaint(
      trace)
  video_path = os.path.join(repeat_dir, sandwich_runner.VIDEO_FILENAME)
  if os.path.isfile(video_path):
    logging.info('processing speed-index video \'%s\'' % video_path)
    try:
      completeness_record = _ExtractCompletenessRecordFromVideo(video_path)
      run_metrics['speed_index'] = _ComputeSpeedIndex(completeness_record)
    except video.BoundingBoxNotFoundException:
      # Sometimes the bounding box for the web content area is not present. Skip
      # calculating Speed Index.
      run_metrics['speed_index'] = _FAILED_CSV_VALUE
  else:
    run_metrics['speed_index'] = _UNAVAILABLE_CSV_VALUE
  for key, value in trace.metadata['network_emulation'].iteritems():
    run_metrics['net_emul.' + key] = value
  assert set(run_metrics.keys()) == set(COMMON_CSV_COLUMN_NAMES)
  return run_metrics
