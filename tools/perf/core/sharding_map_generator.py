# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to generate benchmark_sharding_map.json in the //tools/perf/core
directory. This file controls which bots run which tests.

The file is a JSON dictionary. It maps waterfall name to a mapping of benchmark
to bot id. E.g.

{
  "build1-b1": {
    "benchmarks": [
      "battor.steady_state",
      ...
    ],
  }
}

This will be used to manually shard tests to certain bots, to more efficiently
execute all our tests.
"""

import argparse
import json
import os

from core import path_util
path_util.AddTelemetryToPath()


def get_sharding_map_path():
  return os.path.join(
      path_util.GetChromiumSrcDir(), 'tools', 'perf', 'core',
      'benchmark_sharding_map.json')


def load_benchmark_sharding_map():
  with open(get_sharding_map_path()) as f:
    raw = json.load(f)

  # The raw json format is easy for people to modify, but isn't what we want
  # here. Change it to map builder -> benchmark -> device.
  final_map = {}
  for builder, builder_map in raw.items():
    if builder == 'all_benchmarks':
      continue

    # Ignore comment at the top of the builder_map
    if builder == "comment":
      continue
    final_builder_map = {}
    for device, device_value in builder_map.items():
      for benchmark_name in device_value['benchmarks']:
        final_builder_map[benchmark_name] = device
    final_map[builder] = final_builder_map

  return final_map


# Returns a sorted list of (benchmark, avg_runtime) pairs for every
# benchmark in the all_benchmarks list where avg_runtime is in seconds.  Also
# returns a list of benchmarks whose run time have not been seen before
def get_sorted_benchmark_list_by_time(all_benchmarks):
  runtime_list = []
  benchmark_avgs = {}
  timing_file_path = os.path.join(
      path_util.GetChromiumSrcDir(), 'tools', 'perf', 'core',
      'desktop_benchmark_avg_times.json')
  # Load in the avg times as calculated on Nov 1st, 2016
  with open(timing_file_path) as f:
    benchmark_avgs = json.load(f)

  for benchmark in all_benchmarks:
    benchmark_avg_time = benchmark_avgs.get(benchmark.Name(), None)
    assert benchmark_avg_time
    # Need to multiple the seconds by 2 since we will be generating two tests
    # for each benchmark to be run on the same shard for the reference build
    runtime_list.append((benchmark, benchmark_avg_time * 2.0))

  # Return a reverse sorted list by runtime
  runtime_list.sort(key=lambda tup: tup[1], reverse=True)
  return runtime_list


# Returns a map of benchmark name to shard it is on.
def shard_benchmarks(num_shards, all_benchmarks):
  benchmark_to_shard_dict = {}
  shard_execution_times = [0] * num_shards
  sorted_benchmark_list = get_sorted_benchmark_list_by_time(all_benchmarks)
  # Iterate over in reverse order and add them to the current smallest bucket.
  for benchmark in sorted_benchmark_list:
    # Find current smallest bucket
    min_index = shard_execution_times.index(min(shard_execution_times))
    benchmark_to_shard_dict[benchmark[0].Name()] = min_index
    shard_execution_times[min_index] += benchmark[1]
  return benchmark_to_shard_dict

def regenerate(
    benchmarks, waterfall_configs, dry_run, verbose, builder_names=None):
  """Regenerate the shard mapping file.

  This overwrites the current file with fresh data.
  """
  if not builder_names:
    builder_names = []

  with open(get_sharding_map_path()) as f:
    sharding_map = json.load(f)

  all_benchmarks = [b.Name() for b in benchmarks]
  sharding_map[u'all_benchmarks'] = all_benchmarks

  for name, config in waterfall_configs.items():
    for builder, tester in config['testers'].items():
      if not tester.get('swarming'):
        continue

      if builder not in builder_names:
        continue
      per_builder = {}

      devices = tester['swarming_dimensions'][0]['device_ids']
      shard_number = len(devices)
      shard = shard_benchmarks(shard_number, benchmarks)

      for name, index in shard.items():
        device = devices[index]
        device_map = per_builder.get(device, {'benchmarks': []})
        device_map['benchmarks'].append(name)
        per_builder[device] = device_map
      sharding_map[builder] = per_builder


  for name, builder_values in sharding_map.items():
    if name == 'all_benchmarks':
      builder_values.sort()
      continue

    for value in builder_values.values():
      # Remove any deleted benchmarks
      benchmarks = []
      for b in value['benchmarks']:
        if b in all_benchmarks:
          benchmarks.append(b)
      value['benchmarks'] = benchmarks
      value['benchmarks'].sort()

  if not dry_run:
    with open(get_sharding_map_path(), 'w') as f:
      dump_json(sharding_map, f)
  else:
    f_string = 'Would have dumped new json file to %s.'
    if verbose:
      f_string += ' File contents:\n %s'
      print f_string % (get_sharding_map_path(), dumps_json(sharding_map))
    else:
      f_string += ' To see full file contents, pass in --verbose.'
      print f_string % get_sharding_map_path()

  return 0


def get_args():
  parser = argparse.ArgumentParser(
      description=('Generate perf test sharding map.'
                   'This needs to be done anytime you add/remove any existing'
                   'benchmarks in tools/perf/benchmarks.'))

  parser.add_argument(
      '--builder-names', '-b', action='append', default=None,
      help='Specifies a subset of builders which should be affected by commands'
           '. By default, commands affect all builders.')
  parser.add_argument(
      '--dry-run', action='store_true',
      help='If the current run should be a dry run. A dry run means that any'
      ' action which would be taken (write out data to a file, for example) is'
      ' instead simulated.')
  parser.add_argument(
      '--verbose', action='store_true',
      help='Determines how verbose the script is.')
  return parser


def dump_json(data, f):
  """Utility method to dump json which is indented, sorted, and readable"""
  return json.dump(data, f, indent=2, sort_keys=True, separators=(',', ': '))

def dumps_json(data):
  """Utility method to dump json which is indented, sorted, and readable"""
  return json.dumps(data, indent=2, sort_keys=True, separators=(',', ': '))

def main(args, benchmarks, configs):
  return regenerate(
      benchmarks, configs, args.dry_run, args.verbose, args.builder_names)
