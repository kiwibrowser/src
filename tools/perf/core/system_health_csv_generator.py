# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import csv
import sys

from core import path_util

sys.path.insert(1, path_util.GetPerfDir())  # To resolve perf imports
path_util.AddPyUtilsToPath()
path_util.AddTelemetryToPath()
import page_sets
from py_utils import expectations_parser
from telemetry.story import expectations as expectations

def IterAllSystemHealthStories():
  for s in page_sets.SystemHealthStorySet(platform='desktop'):
    yield s
  for s in page_sets.SystemHealthStorySet(platform='mobile'):
    if len(s.SUPPORTED_PLATFORMS) < 2:
      yield s


def PopulateExpectations(all_expectations):
  """Accepts Expectations and parses out the storyname and disabled platforms.

  Args:
    all_expectations = {
        story_name: [[conditions], reason]}
    conditions: list of disabled platforms for story_name
    reason: Bug referencing why the test is disabled on the platform

  Returns:
    A dictionary containing the disabled platforms for each story.
    disables = {
        story_name: "Disabled Platforms"}
  """
  disables = {}
  for exp in all_expectations:
    exp_keys = exp.keys()
    exp_keys.sort()

    for story in exp_keys:
      for conditions, _ in exp[story]:
        conditions_str = ", ".join(map(str, conditions))
        if story in disables:
          if conditions_str not in disables[story]:
            disables[story] += ", " + conditions_str
        else:
          disables[story] = conditions_str
  return disables

def GenerateSystemHealthCSV(file_path):
  system_health_stories = list(IterAllSystemHealthStories())

  e = expectations.StoryExpectations()
  with open(path_util.GetExpectationsPath()) as fp:
    parser = expectations_parser.TestExpectationParser(fp.read())

  benchmarks = ['system_health.common_desktop', 'system_health.common_mobile',
                'system_health.memory_desktop', 'system_health.memory_mobile']
  for benchmark in benchmarks:
    e.GetBenchmarkExpectationsFromParser(parser.expectations, benchmark)

  disabed_platforms = PopulateExpectations([e.AsDict()['stories']])

  system_health_stories.sort(key=lambda s: s.name)
  with open(file_path, 'w') as f:
    csv_writer = csv.writer(f)
    csv_writer.writerow([
        'Story name', 'Platform', 'Description', 'Disabled Platforms'])
    for s in system_health_stories:
      p = s.SUPPORTED_PLATFORMS
      if len(p) == 2:
        p = 'all'
      else:
        p = list(p)[0]
      if s.name in disabed_platforms:
        csv_writer.writerow(
            [s.name, p, s.GetStoryDescription(), disabed_platforms[s.name]])
      else:
        csv_writer.writerow([s.name, p, s.GetStoryDescription(), " "])
  return 0
