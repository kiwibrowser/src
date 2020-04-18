#!/usr/bin/env vpython
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Script to check validity of StoryExpectations."""

import optparse
import argparse
import json
import os

from core import benchmark_finders
from core import path_util
path_util.AddTelemetryToPath()
path_util.AddAndroidPylibToPath()


from telemetry.internal.browser import browser_options


CLUSTER_TELEMETRY_DIR = os.path.join(
    path_util.GetChromiumSrcDir(), 'tools', 'perf', 'contrib',
    'cluster_telemetry')
CLUSTER_TELEMETRY_BENCHMARKS = [
    ct_benchmark.Name() for ct_benchmark in
    benchmark_finders.GetBenchmarksInSubDirectory(CLUSTER_TELEMETRY_DIR)
]


def validate_story_names(benchmarks, raw_expectations_data):
  for benchmark in benchmarks:
    if benchmark.Name() in CLUSTER_TELEMETRY_BENCHMARKS:
      continue
    b = benchmark()
    b.AugmentExpectationsWithParser(raw_expectations_data)
    options = browser_options.BrowserFinderOptions()

    # Add default values for any extra commandline options
    # provided by the benchmark.
    parser = optparse.OptionParser()
    before, _ = parser.parse_args([])
    benchmark.AddBenchmarkCommandLineArgs(parser)
    after, _ = parser.parse_args([])
    for extra_option in dir(after):
        if extra_option not in dir(before):
            setattr(options, extra_option, getattr(after, extra_option))

    story_set = b.CreateStorySet(options)
    failed_stories = b.GetBrokenExpectations(story_set)
    assert not failed_stories, 'Incorrect story names: %s' % str(failed_stories)


def GetDisabledStories(benchmarks, raw_expectations_data):
  # Creates a dictionary of the format:
  # {
  #   'benchmark_name1' : {
  #     'story_1': [
  #       {'conditions': conditions, 'reason': reason},
  #       ...
  #     ],
  #     ...
  #   },
  #   ...
  # }
  disables = {}
  for benchmark in benchmarks:
    name = benchmark.Name()
    disables[name] = {}
    b = benchmark()
    b.AugmentExpectationsWithParser(raw_expectations_data)
    expectations = b.expectations.AsDict()['stories']
    for story in expectations:
      for conditions, reason in  expectations[story]:
        if not disables[name].get(story):
          disables[name][story] = []
          conditions_str = [str(a) for a in conditions]
        disables[name][story].append((conditions_str, reason))
  return disables


def main(args):
  parser = argparse.ArgumentParser(
      description=('Tests if disabled stories exist.'))
  parser.add_argument(
      '--list', action='store_true', default=False,
      help=('Prints list of disabled stories.'))
  options = parser.parse_args(args)
  benchmarks = benchmark_finders.GetAllBenchmarks()
  with open(path_util.GetExpectationsPath()) as fp:
    raw_expectations_data = fp.read()
  if options.list:
    stories = GetDisabledStories(benchmarks, raw_expectations_data)
    print json.dumps(stories, sort_keys=True, indent=4, separators=(',', ': '))
  else:
    validate_story_names(benchmarks, raw_expectations_data)
  return 0
