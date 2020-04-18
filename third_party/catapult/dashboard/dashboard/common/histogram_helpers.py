# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Helper methods for working with histograms and diagnostics."""

import re

from tracing.value.diagnostics import reserved_infos

# List of non-TBMv2 chromium.perf Telemetry benchmarks
_LEGACY_BENCHMARKS = [
    'blink_perf.bindings',
    'blink_perf.canvas',
    'blink_perf.css',
    'blink_perf.dom',
    'blink_perf.events',
    'blink_perf.image_decoder',
    'blink_perf.layout',
    'blink_perf.owp_storage',
    'blink_perf.paint',
    'blink_perf.parser',
    'blink_perf.shadow_dom',
    'blink_perf.svg',
    'cronet_perf_tests',
    'dromaeo',
    'dummy_benchmark.noisy_benchmark_1',
    'dummy_benchmark.stable_benchmark_1',
    'jetstream',
    'kraken',
    'octane',
    'rasterize_and_record_micro.partial_invalidation',
    'rasterize_and_record_micro.top_25',
    'scheduler.tough_scheduling_cases',
    'smoothness.desktop_tough_pinch_zoom_cases',
    'smoothness.gpu_rasterization.polymer',
    'smoothness.gpu_rasterization.top_25_smooth',
    'smoothness.gpu_rasterization.tough_filters_cases',
    'smoothness.gpu_rasterization.tough_path_rendering_cases',
    'smoothness.gpu_rasterization.tough_pinch_zoom_cases',
    'smoothness.gpu_rasterization.tough_scrolling_cases',
    'smoothness.gpu_rasterization_and_decoding.image_decoding_cases',
    'smoothness.image_decoding_cases',
    'smoothness.key_desktop_move_cases',
    'smoothness.key_mobile_sites_smooth',
    'smoothness.key_silk_cases',
    'smoothness.maps',
    'smoothness.pathological_mobile_sites',
    'smoothness.simple_mobile_sites',
    'smoothness.sync_scroll.key_mobile_sites_smooth',
    'smoothness.top_25_smooth',
    'smoothness.tough_ad_cases',
    'smoothness.tough_animation_cases',
    'smoothness.tough_canvas_cases',
    'smoothness.tough_filters_cases',
    'smoothness.tough_image_decode_cases',
    'smoothness.tough_path_rendering_cases',
    'smoothness.tough_pinch_zoom_cases',
    'smoothness.tough_scrolling_cases',
    'smoothness.tough_texture_upload_cases',
    'smoothness.tough_webgl_ad_cases',
    'smoothness.tough_webgl_cases',
    'speedometer',
    'speedometer-future',
    'speedometer2',
    'speedometer2-future',
    'start_with_url.cold.startup_pages',
    'start_with_url.warm.startup_pages',
    'thread_times.key_hit_test_cases',
    'thread_times.key_idle_power_cases',
    'thread_times.key_mobile_sites_smooth',
    'thread_times.key_noop_cases',
    'thread_times.key_silk_cases',
    'thread_times.simple_mobile_sites',
    'thread_times.tough_compositor_cases',
    'thread_times.tough_scrolling_cases',
    'v8.detached_context_age_in_gc'
]
_STATS_BLACKLIST = ['std', 'count', 'max', 'min', 'sum']


def GetTIRLabelFromHistogram(hist):
  tags = hist.diagnostics.get(reserved_infos.STORY_TAGS.name) or []

  tags_to_use = [t.split(':') for t in tags if ':' in t]

  return '_'.join(v for _, v in sorted(tags_to_use))


def IsLegacyBenchmark(benchmark_name):
  return benchmark_name in _LEGACY_BENCHMARKS


def ShouldFilterStatistic(test_name, benchmark_name, stat_name):
  if test_name == 'benchmark_total_duration':
    return True
  if benchmark_name.startswith('memory') and not benchmark_name.startswith(
      'memory.long_running'):
    if 'memory:' in test_name and stat_name in _STATS_BLACKLIST:
      return True
  if benchmark_name.startswith('memory.long_running'):
    value_name = '%s_%s' % (test_name, stat_name)
    return not _ShouldAddMemoryLongRunningValue(value_name)
  if benchmark_name == 'media.desktop' or benchmark_name == 'media.mobile':
    value_name = '%s_%s' % (test_name, stat_name)
    return not _ShouldAddMediaValue(value_name)
  if benchmark_name.startswith('system_health'):
    if stat_name in _STATS_BLACKLIST:
      return True
  if benchmark_name.startswith('v8.browsing'):
    value_name = '%s_%s' % (test_name, stat_name)
    return not _ShouldAddV8BrowsingValue(value_name)
  return False


def _ShouldAddMediaValue(value_name):
  media_re = re.compile(
      r'(?<!dump)(?<!process)_(std|count|max|min|sum|pct_\d{4}(_\d+)?)$')
  return not media_re.search(value_name)


def _ShouldAddMemoryLongRunningValue(value_name):
  v8_re = re.compile(
      r'renderer_processes:'
      r'(reported_by_chrome:v8|reported_by_os:system_memory:[^:]+$)')
  if 'memory:chrome' in value_name:
    return ('renderer:subsystem:v8' in value_name or
            'renderer:vmstats:overall' in value_name or
            bool(v8_re.search(value_name)))
  return 'v8' in value_name


def _ShouldAddV8BrowsingValue(value_name):
  v8_gc_re = re.compile(
      r'^v8-gc-('
      r'full-mark-compactor_|'
      r'incremental-finalize_|'
      r'incremental-step_|'
      r'latency-mark-compactor_|'
      r'memory-mark-compactor_|'
      r'scavenger_|'
      r'total_)')
  stats_re = re.compile(r'_(std|count|min|sum|pct_\d{4}(_\d+)?)$')
  v8_stats_re = re.compile(
      r'_(idle_deadline_overrun|percentage_idle|outside_idle)')
  if 'memory:unknown_browser' in value_name:
    return ('renderer_processes' in value_name and
            not stats_re.search(value_name))
  if 'memory:chrome' in value_name:
    return ('renderer_processes' in value_name and
            not stats_re.search(value_name))
  if 'v8-gc' in value_name:
    return v8_gc_re.search(value_name) and not v8_stats_re.search(value_name)
  return True
