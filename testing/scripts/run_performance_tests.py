#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs several telemetry benchmarks.

This script attempts to emulate the contract of gtest-style tests
invoked via recipes. The main contract is that the caller passes the
argument:

  --isolated-script-test-output=[FILENAME]

json is written to that file in the format detailed here:
https://www.chromium.org/developers/the-json-test-results-format

Optional argument:

  --isolated-script-test-filter=[TEST_NAMES]

is a double-colon-separated ("::") list of test names, to run just that subset
of tests. This list is forwarded to the run_telemetry_benchmark_as_googletest
script.

This script is intended to be the base command invoked by the isolate,
followed by a subsequent Python script. It could be generalized to
invoke an arbitrary executable.

It currently runs several benchmarks. The benchmarks it will execute are
based on the shard it is running on and the sharding_map_path.

If this is executed with a non-telemetry perf test, the flag --non-telemetry
has to be passed in to the script so the script knows it is running
an executable and not the run_benchmark command.

The results of running the benchmark are put in separate directories per
benchmark. Two files will be present in each directory; perf_results.json, which
is the perf specific results (with unenforced format, could be histogram,
legacy, or chartjson), and test_results.json, which is a JSON test results
format file
(https://www.chromium.org/developers/the-json-test-results-format)

This script was derived from run_telemetry_benchmark_as_googletest, and calls
into that script.
"""

import argparse
import json
import os
import shutil
import sys
import tempfile
import traceback

import common

import run_telemetry_benchmark_as_googletest
import run_gtest_perf_test

# Current whitelist of benchmarks outputting histograms
BENCHMARKS_TO_OUTPUT_HISTOGRAMS = [
    'dummy_benchmark.histogram_benchmark_1',
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
    'memory.top_10_mobile',
    'system_health.common_desktop',
    'system_health.common_mobile',
    'system_health.memory_desktop',
    'system_health.memory_mobile',
    'system_health.webview_startup',
    'smoothness.gpu_rasterization.tough_filters_cases',
    'smoothness.gpu_rasterization.tough_path_rendering_cases',
    'smoothness.gpu_rasterization.tough_scrolling_cases',
    'smoothness.gpu_rasterization_and_decoding.image_decoding_cases',
    'smoothness.image_decoding_cases',
    'smoothness.key_desktop_move_cases',
    'smoothness.maps',
    'smoothness.oop_rasterization.top_25_smooth',
    'smoothness.top_25_smooth',
    'smoothness.tough_ad_cases',
    'smoothness.tough_animation_cases',
    'smoothness.tough_canvas_cases',
    'smoothness.tough_filters_cases',
    'smoothness.tough_image_decode_cases',
    'smoothness.tough_path_rendering_cases',
    'smoothness.tough_scrolling_cases',
    'smoothness.tough_texture_upload_cases',
    'smoothness.tough_webgl_ad_cases',
    'smoothness.tough_webgl_cases',
    'dromaeo',
    'jetstream',
    'kraken',
    'octane',
    'speedometer',
    'speedometer-future',
    'speedometer2',
    'speedometer2-future',
    'wasm',
    'battor.steady_state',
    'battor.trivial_pages',
    'rasterize_and_record_micro.partial_invalidation',
    'rasterize_and_record_micro.top_25',
    'scheduler.tough_scheduling_cases',
    'tab_switching.typical_25',
    'thread_times.key_hit_test_cases',
    'thread_times.key_idle_power_cases',
    'thread_times.key_mobile_sites_smooth',
    'thread_times.key_noop_cases',
    'thread_times.key_silk_cases',
    'thread_times.simple_mobile_sites',
    'thread_times.oop_rasterization.key_mobile',
    'thread_times.tough_compositor_cases',
    'thread_times.tough_scrolling_cases',
    'tracing.tracing_with_background_memory_infra',
    'tracing.tracing_with_debug_overhead',
    'v8.browsing_desktop',
    'v8.browsing_mobile',
    'v8.browsing_desktop-future',
    'v8.browsing_mobile-future',
]

def get_sharding_map_path(args):
  return os.path.join(
      os.path.dirname(__file__), '..', '..', 'tools', 'perf', 'core',
      args.test_shard_map_filename)

def write_results(
    perf_test_name, perf_results, json_test_results, benchmark_log,
    isolated_out_dir, encoded):
  benchmark_path = os.path.join(isolated_out_dir, perf_test_name)

  os.makedirs(benchmark_path)
  with open(os.path.join(benchmark_path, 'perf_results.json'), 'w') as f:
    # non telemetry perf results are already json encoded
    if encoded:
      f.write(perf_results)
    else:
      json.dump(perf_results, f)
  with open(os.path.join(benchmark_path, 'test_results.json'), 'w') as f:
    json.dump(json_test_results, f)

  with open(os.path.join(benchmark_path, 'benchmark_log.txt'), 'w') as f:
    f.write(benchmark_log)


def execute_benchmark(benchmark, isolated_out_dir,
                      args, rest_args, is_reference):
  # While we are between chartjson and histogram set we need
  # to determine which output format to look for or see if it was
  # already passed in in which case that format applies to all benchmarks
  # in this run.
  is_histograms = append_output_format(benchmark, args, rest_args)
  # Insert benchmark name as first argument to run_benchmark call
  # which is the first argument in the rest_args.  Also need to append
  # output format and smoke test mode.
  per_benchmark_args = (rest_args[:1] + [benchmark] + rest_args[1:])
  benchmark_name = benchmark
  if is_reference:
    # Need to parse out the browser to replace browser flag with
    # reference build so we run it reference build as well
    browser_index = 0
    for arg in per_benchmark_args:
      if "browser" in arg:
        break
      browser_index = browser_index + 1
    per_benchmark_args[browser_index] = '--browser=reference'
    # Now we need to add in the rest of the reference build args
    per_benchmark_args.append('--max-failures=5')
    per_benchmark_args.append('--output-trace-tag=_ref')
    benchmark_name = benchmark + '.reference'

  # We don't care exactly what these are. In particular, the perf results
  # could be any format (chartjson, legacy, histogram). We just pass these
  # through, and expose these as results for this task.
  rc, perf_results, json_test_results, benchmark_log = (
      run_telemetry_benchmark_as_googletest.run_benchmark(
          args, per_benchmark_args, is_histograms))

  write_results(
      benchmark_name, perf_results, json_test_results, benchmark_log,
      isolated_out_dir, False)
  return rc


def append_output_format(benchmark, args, rest_args):
  # We need to determine if the output format is already passed in
  # or if we need to define it for this benchmark
  perf_output_specified = False
  is_histograms = False
  if args.output_format:
    for output_format in args.output_format:
      if 'histograms' in output_format:
        perf_output_specified = True
        is_histograms = True
      if 'chartjson' in output_format:
        perf_output_specified = True
      rest_args.append('--output-format=' + output_format)
  # When crbug.com/744736 is resolved we no longer have to check
  # the type of format per benchmark and can rely on it being passed
  # in as an arg as all benchmarks will output the same format.
  if not perf_output_specified:
    if benchmark in BENCHMARKS_TO_OUTPUT_HISTOGRAMS:
      rest_args.append('--output-format=histograms')
      is_histograms = True
    else:
      rest_args.append('--output-format=chartjson')
  return is_histograms

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--isolated-script-test-output', required=True)
  # These two flags are passed in from the swarming recipe
  # but will no longer be needed when we migrate to this new recipe.
  # For now we need to recognize them so they don't get passed
  # through to telemetry.
  parser.add_argument(
      '--isolated-script-test-chartjson-output', required=False)
  parser.add_argument(
      '--isolated-script-test-perf-output', required=False)

  parser.add_argument(
      '--isolated-script-test-filter', type=str, required=False)
  parser.add_argument('--xvfb', help='Start xvfb.', action='store_true')
  parser.add_argument('--non-telemetry',
                      help='Type of perf test', type=bool, default=False)
  parser.add_argument('--benchmarks',
                      help='Comma separated list of benchmark names'
                      ' to run in lieu of indexing into our benchmark bot maps',
                      required=False)
  # Some executions may have a different sharding scheme and/or set of tests.
  # These files must live in src/tools/perf/core/
  parser.add_argument('--test-shard-map-filename', type=str, required=False)
  parser.add_argument('--output-format', action='append')
  parser.add_argument('--run-ref-build',
                      help='Run test on reference browser', action='store_true')

  args, rest_args = parser.parse_known_args()
  isolated_out_dir = os.path.dirname(args.isolated_script_test_output)
  return_code = 0

  if args.non_telemetry:
    # For non telemetry tests the benchmark name is the name of the executable.
    benchmark_name = rest_args[0]
    return_code, charts, output_json = run_gtest_perf_test.execute_perf_test(
        args, rest_args)

    write_results(benchmark_name, charts, output_json,
                  benchmark_log='Not available for C++ perf test',
                  isolated_out_dir=isolated_out_dir, encoded=True)
  else:
    # If the user has supplied a list of benchmark names, execute those instead
    # of the entire suite of benchmarks.
    if args.benchmarks:
      benchmarks = args.benchmarks.split(',')
      for benchmark in benchmarks:
        return_code = (execute_benchmark(
            benchmark, isolated_out_dir, args, rest_args, False) or return_code)
    else:
      # First determine what shard we are running on to know how to
      # index into the bot map to get list of benchmarks to run.
      total_shards = None
      shard_index = None

      env = os.environ.copy()
      if 'GTEST_TOTAL_SHARDS' in env:
        total_shards = env['GTEST_TOTAL_SHARDS']
      if 'GTEST_SHARD_INDEX' in env:
        shard_index = env['GTEST_SHARD_INDEX']

      if not (total_shards or shard_index):
        raise Exception('Shard indicators must be present for perf tests')

      sharding_map_path = get_sharding_map_path(args)
      with open(sharding_map_path) as f:
        sharding_map = json.load(f)
      sharding = None
      sharding = sharding_map[shard_index]['benchmarks']

      for benchmark in sharding:
        # Need to run the benchmark twice on browser and reference build
        return_code = (execute_benchmark(
            benchmark, isolated_out_dir, args, rest_args, False) or return_code)
        # We ignore the return code of the reference build since we do not
        # monitor it.
        if args.run_ref_build:
          execute_benchmark(benchmark, isolated_out_dir, args, rest_args, True)

  return return_code


# This is not really a "script test" so does not need to manually add
# any additional compile targets.
def main_compile_targets(args):
  json.dump([], args.output)


if __name__ == '__main__':
  # Conform minimally to the protocol defined by ScriptTest.
  if 'compile_targets' in sys.argv:
    funcs = {
      'run': None,
      'compile_targets': main_compile_targets,
    }
    sys.exit(common.run_script(sys.argv[1:], funcs))
  sys.exit(main())
