# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Run the first page of one benchmark for every module.

Only benchmarks that have a composable measurement are included.
Ideally this test would be comprehensive, however, running one page
of every benchmark would run impractically long.
"""

import os
import sys
import unittest

from core import path_util

from telemetry import benchmark as benchmark_module
from telemetry import decorators
from telemetry.testing import options_for_unittests
from telemetry.testing import progress_reporter

from py_utils import discover

from benchmarks import battor
from benchmarks import jetstream
from benchmarks import kraken
from benchmarks import octane
from benchmarks import rasterize_and_record_micro
from benchmarks import speedometer
from benchmarks import v8_browsing


MAX_NUM_VALUES = 50000


def SmokeTestGenerator(benchmark, num_pages=1):
  """Generates a benchmark that includes first N pages from pageset.

  Args:
    benchmark: benchmark object to make smoke test.
    num_pages: use the first N pages to run smoke test.
  """
  # NOTE TO SHERIFFS: DO NOT DISABLE THIS TEST.
  #
  # This smoke test dynamically tests all benchmarks. So disabling it for one
  # failing or flaky benchmark would disable a much wider swath of coverage
  # than is usally intended. Instead, if a particular benchmark is failing,
  # disable it in tools/perf/benchmarks/*.
  @decorators.Disabled('chromeos')  # crbug.com/351114
  @decorators.Disabled('android')  # crbug.com/641934
  def BenchmarkSmokeTest(self):
    # Only measure a single page so that this test cycles reasonably quickly.
    benchmark.options['smoke_test_mode'] = True

    # Some benchmarks are running multiple iterations
    # which is not needed for a smoke test
    if hasattr(benchmark, 'enable_smoke_test_mode'):
      benchmark.enable_smoke_test_mode = True

    class SinglePageBenchmark(benchmark):  # pylint: disable=no-init

      def CreateStorySet(self, options):
        # pylint: disable=super-on-old-class
        story_set = super(SinglePageBenchmark, self).CreateStorySet(options)

        # Only smoke test the first story since smoke testing everything takes
        # too long.
        for s in story_set.stories[num_pages:]:
          story_set.RemoveStory(s)
        return story_set

    # Set the benchmark's default arguments.
    options = options_for_unittests.GetCopy()
    options.output_formats = ['none']
    parser = options.CreateParser()

    benchmark.AddCommandLineArgs(parser)
    benchmark_module.AddCommandLineArgs(parser)
    benchmark.SetArgumentDefaults(parser)
    options.MergeDefaultValues(parser.get_default_values())

    # Prevent benchmarks from accidentally trying to upload too much data to the
    # chromeperf dashboard. The number of values uploaded is equal to (the
    # average number of values produced by a single story) * (1 + (the number of
    # stories)). The "1 + " accounts for values summarized across all stories.
    # We can approximate "the average number of values produced by a single
    # story" as the number of values produced by the first story.
    # pageset_repeat doesn't matter because values are summarized across
    # repetitions before uploading.
    story_set = benchmark().CreateStorySet(options)
    SinglePageBenchmark.MAX_NUM_VALUES = MAX_NUM_VALUES / len(story_set.stories)

    benchmark.ProcessCommandLineArgs(None, options)
    benchmark_module.ProcessCommandLineArgs(None, options)

    single_page_benchmark = SinglePageBenchmark()
    with open(path_util.GetExpectationsPath()) as fp:
      single_page_benchmark.AugmentExpectationsWithParser(fp.read())

    self.assertEqual(0, single_page_benchmark.Run(options),
                     msg='Failed: %s' % benchmark)

  return BenchmarkSmokeTest


# The list of benchmark modules to be excluded from our smoke tests.
_BLACK_LIST_TEST_MODULES = {
    octane,  # Often fails & take long time to timeout on cq bot.
    rasterize_and_record_micro,  # Always fails on cq bot.
    speedometer,  # Takes 101 seconds.
    jetstream,  # Take 206 seconds.
    kraken,  # Flaky on Android, crbug.com/626174.
    v8_browsing, # Flaky on Android, crbug.com/628368.
    battor #Flaky on android, crbug.com/618330.
}

# The list of benchmark names to be excluded from our smoke tests.
_BLACK_LIST_TEST_NAMES = [
   'memory.long_running_idle_gmail_background_tbmv2',
   'tab_switching.typical_25',
   'oortonline_tbmv2',
]


def MergeDecorators(method, method_attribute, benchmark, benchmark_attribute):
  # Do set union of attributes to eliminate duplicates.
  merged_attributes = getattr(method, method_attribute, set()).union(
      getattr(benchmark, benchmark_attribute, set()))
  if merged_attributes:
    setattr(method, method_attribute, merged_attributes)


def load_tests(loader, standard_tests, pattern):
  del loader, standard_tests, pattern  # unused
  suite = progress_reporter.TestSuite()

  benchmarks_dir = os.path.dirname(__file__)
  top_level_dir = os.path.dirname(benchmarks_dir)

  # Using the default of |index_by_class_name=False| means that if a module
  # has multiple benchmarks, only the last one is returned.
  all_benchmarks = discover.DiscoverClasses(
      benchmarks_dir, top_level_dir, benchmark_module.Benchmark,
      index_by_class_name=False).values()
  for benchmark in all_benchmarks:
    if sys.modules[benchmark.__module__] in _BLACK_LIST_TEST_MODULES:
      continue
    if benchmark.Name() in _BLACK_LIST_TEST_NAMES:
      continue

    class BenchmarkSmokeTest(unittest.TestCase):
      pass

    # tab_switching needs more than one page to test correctly.
    if 'tab_switching' in benchmark.Name():
      method = SmokeTestGenerator(benchmark, num_pages=2)
    else:
      method = SmokeTestGenerator(benchmark)

    # Make sure any decorators are propagated from the original declaration.
    # (access to protected members) pylint: disable=protected-access
    # TODO(dpranke): Since we only pick the first test from every class
    # (above), if that test is disabled, we'll end up not running *any*
    # test from the class. We should probably discover all of the tests
    # in a class, and then throw the ones we don't need away instead.

    disabled_benchmark_attr = decorators.DisabledAttributeName(benchmark)
    disabled_method_attr = decorators.DisabledAttributeName(method)
    enabled_benchmark_attr = decorators.EnabledAttributeName(benchmark)
    enabled_method_attr = decorators.EnabledAttributeName(method)

    MergeDecorators(method, disabled_method_attr, benchmark,
                    disabled_benchmark_attr)
    MergeDecorators(method, enabled_method_attr, benchmark,
                    enabled_benchmark_attr)

    setattr(BenchmarkSmokeTest, benchmark.Name(), method)

    suite.addTest(BenchmarkSmokeTest(benchmark.Name()))

  return suite
