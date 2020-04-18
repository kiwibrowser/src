# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import optparse
import sys

from py_utils import class_util
from py_utils import expectations_parser
from telemetry import decorators
from telemetry.internal import story_runner
from telemetry.internal.util import command_line
from telemetry.page import legacy_page_test
from telemetry.story import expectations as expectations_module
from telemetry.web_perf import timeline_based_measurement
from tracing.value.diagnostics import generic_set

Owner = decorators.Owner # pylint: disable=invalid-name


class InvalidOptionsError(Exception):
  """Raised for invalid benchmark options."""
  pass


class BenchmarkMetadata(object):

  def __init__(self, name, description='', rerun_options=None):
    self._name = name
    self._description = description
    self._rerun_options = rerun_options

  @property
  def name(self):
    return self._name

  @property
  def description(self):
    return self._description

  @property
  def rerun_options(self):
    return self._rerun_options

  def AsDict(self):
    return {
        'type': 'telemetry_benchmark',
        'name': self._name,
        'description': self._description,
        'rerun_options': self._rerun_options,
    }


class Benchmark(command_line.Command):
  """Base class for a Telemetry benchmark.

  A benchmark packages a measurement and a PageSet together.
  Benchmarks default to using TBM unless you override the value of
  Benchmark.test, or override the CreatePageTest method.

  New benchmarks should override CreateStorySet.
  """
  options = {}
  page_set = None
  test = timeline_based_measurement.TimelineBasedMeasurement
  SUPPORTED_PLATFORMS = [expectations_module.ALL]

  MAX_NUM_VALUES = sys.maxint

  def __init__(self, max_failures=None):
    """Creates a new Benchmark.

    Args:
      max_failures: The number of story run's failures before bailing
          from executing subsequent page runs. If None, we never bail.
    """
    self._expectations = expectations_module.StoryExpectations()
    self._max_failures = max_failures
    # TODO: There should be an assertion here that checks that only one of
    # the following is true:
    # * It's a TBM benchmark, with CreateCoreTimelineBasedMeasurementOptions
    #   defined.
    # * It's a legacy benchmark, with either CreatePageTest defined or
    #   Benchmark.test set.
    # See https://github.com/catapult-project/catapult/issues/3708


  def _CanRunOnPlatform(self, platform, finder_options):
    for p in self.SUPPORTED_PLATFORMS:
      # This is reusing StoryExpectation code, so it is a bit unintuitive. We
      # are trying to detect the opposite of the usual case in StoryExpectations
      # so we want to return True when ShouldDisable returns true, even though
      # we do not want to disable.
      if p.ShouldDisable(platform, finder_options):
        return True
    return False

  def Run(self, finder_options):
    """Do not override this method."""
    return story_runner.RunBenchmark(self, finder_options)

  @property
  def max_failures(self):
    return self._max_failures

  @classmethod
  def Name(cls):
    return '%s.%s' % (cls.__module__.split('.')[-1], cls.__name__)

  @classmethod
  def AddCommandLineArgs(cls, parser):
    group = optparse.OptionGroup(parser, '%s test options' % cls.Name())
    cls.AddBenchmarkCommandLineArgs(group)

    if cls.HasTraceRerunDebugOption():
      group.add_option(
          '--rerun-with-debug-trace',
          action='store_true',
          help='Rerun option that enables more extensive tracing.')

    if group.option_list:
      parser.add_option_group(group)

  @classmethod
  def AddBenchmarkCommandLineArgs(cls, group):
    del group  # unused

  @classmethod
  def HasTraceRerunDebugOption(cls):
    return False

  def GetTraceRerunCommands(self):
    if self.HasTraceRerunDebugOption():
      return [['Debug Trace', '--rerun-with-debug-trace']]
    return []

  def SetupTraceRerunOptions(self, browser_options, tbm_options):
    if self.HasTraceRerunDebugOption():
      if browser_options.rerun_with_debug_trace:
        self.SetupBenchmarkDebugTraceRerunOptions(tbm_options)
      else:
        self.SetupBenchmarkDefaultTraceRerunOptions(tbm_options)

  def SetupBenchmarkDefaultTraceRerunOptions(self, tbm_options):
    """Setup tracing categories associated with default trace option."""

  def SetupBenchmarkDebugTraceRerunOptions(self, tbm_options):
    """Setup tracing categories associated with debug trace option."""

  @classmethod
  def SetArgumentDefaults(cls, parser):
    default_values = parser.get_default_values()
    invalid_options = [o for o in cls.options if not hasattr(default_values, o)]
    if invalid_options:
      raise InvalidOptionsError('Invalid benchmark options: %s',
                                ', '.join(invalid_options))
    parser.set_defaults(**cls.options)

  @classmethod
  def ProcessCommandLineArgs(cls, parser, args):
    pass

  # pylint: disable=unused-argument
  @classmethod
  def ShouldAddValue(cls, name, from_first_story_run):
    """Returns whether the named value should be added to PageTestResults.

    Override this method to customize the logic of adding values to test
    results. SkipValues and TraceValues will be added regardless
    of logic here.

    Args:
      name: The string name of a value being added.
      from_first_story_run: True if the named value was produced during the
          first run of the corresponding story.

    Returns:
      True if the value should be added to the test results, False otherwise.
    """
    return True

  def CustomizeBrowserOptions(self, options):
    """Add browser options that are required by this benchmark."""

  def GetMetadata(self):
    return BenchmarkMetadata(self.Name(), self.__doc__,
                             self.GetTraceRerunCommands())

  def GetBugComponents(self):
    """Returns a GenericSet Diagnostic containing the benchmark's Monorail
       component.

    Returns:
      GenericSet Diagnostic with the benchmark's bug component name
    """
    benchmark_component = decorators.GetComponent(self)
    component_diagnostic_value = (
        [benchmark_component] if benchmark_component else [])
    return generic_set.GenericSet(component_diagnostic_value)

  def GetOwners(self):
    """Returns a Generic Diagnostic containing the benchmark's owners' emails
       in a list.

    Returns:
      Diagnostic with a list of the benchmark's owners' emails
    """
    return generic_set.GenericSet(decorators.GetEmails(self) or [])

  @decorators.Deprecated(
      2017, 7, 29, 'Use CreateCoreTimelineBasedMeasurementOptions instead.')
  def CreateTimelineBasedMeasurementOptions(self):
    """See CreateCoreTimelineBasedMeasurementOptions."""
    return self.CreateCoreTimelineBasedMeasurementOptions()

  def CreateCoreTimelineBasedMeasurementOptions(self):
    """Return the base TimelineBasedMeasurementOptions for this Benchmark.

    Additional chrome and atrace categories can be appended when running the
    benchmark with the --extra-chrome-categories and --extra-atrace-categories
    flags.

    Override this method to configure a TimelineBasedMeasurement benchmark. If
    this is not a TimelineBasedMeasurement benchmark, override CreatePageTest
    for PageTest tests. Do not override both methods.
    """
    return timeline_based_measurement.Options()

  def _GetTimelineBasedMeasurementOptions(self, options):
    """Return all timeline based measurements for the curren benchmark run.

    This includes the benchmark-configured measurements in
    CreateCoreTimelineBasedMeasurementOptions as well as the user-flag-
    configured options from --extra-chrome-categories and
    --extra-atrace-categories.
    """
    # TODO(sullivan): the benchmark options should all be configured in
    # CreateCoreTimelineBasedMeasurementOptions. Remove references to
    # CreateCoreTimelineBasedMeasurementOptions when it is fully deprecated.
    # In the short term, if the benchmark overrides
    # CreateCoreTimelineBasedMeasurementOptions use the overridden version,
    # otherwise call CreateCoreTimelineBasedMeasurementOptions.
    # https://github.com/catapult-project/catapult/issues/3450
    tbm_options = None
    assert not (
        class_util.IsMethodOverridden(Benchmark, self.__class__,
                                      'CreateTimelineBasedMeasurementOptions')
        and class_util.IsMethodOverridden(
            Benchmark, self.__class__,
            'CreateCoreTimelineBasedMeasurementOptions')
    ), ('Benchmarks should override CreateCoreTimelineBasedMeasurementOptions '
        'and NOT also CreateTimelineBasedMeasurementOptions.')
    if class_util.IsMethodOverridden(
        Benchmark, self.__class__, 'CreateCoreTimelineBasedMeasurementOptions'):
      tbm_options = self.CreateCoreTimelineBasedMeasurementOptions()
    else:
      tbm_options = self.CreateTimelineBasedMeasurementOptions()
    if options and options.extra_chrome_categories:
      # If Chrome tracing categories for this benchmark are not already
      # enabled, there is probably a good reason why (for example, maybe
      # it is the benchmark that runs a BattOr without Chrome to get an energy
      # baseline). Don't change whether Chrome tracing is enabled.
      assert tbm_options.config.enable_chrome_trace, (
          'This benchmark does not support Chrome tracing.')
      tbm_options.config.chrome_trace_config.category_filter.AddFilterString(
          options.extra_chrome_categories)
    if options and options.extra_atrace_categories:
      # Many benchmarks on Android run without atrace by default. Hopefully the
      # user understands that atrace is only supported on Android when setting
      # this option.
      tbm_options.config.enable_atrace_trace = True

      categories = tbm_options.config.atrace_config.categories
      if type(categories) != list:
        # Categories can either be a list or comma-separated string.
        # https://github.com/catapult-project/catapult/issues/3712
        categories = categories.split(',')
      for category in options.extra_atrace_categories.split(','):
        if category not in categories:
          categories.append(category)
      tbm_options.config.atrace_config.categories = categories
    if options and options.enable_systrace:
      tbm_options.config.chrome_trace_config.SetEnableSystrace()
    return tbm_options


  def CreatePageTest(self, options):  # pylint: disable=unused-argument
    """Return the PageTest for this Benchmark.

    Override this method for PageTest tests.
    Override, CreateCoreTimelineBasedMeasurementOptions to configure
    TimelineBasedMeasurement tests. Do not override both methods.

    Args:
      options: a browser_options.BrowserFinderOptions instance
    Returns:
      |test()| if |test| is a PageTest class.
      Otherwise, a TimelineBasedMeasurement instance.
    """
    is_page_test = issubclass(self.test, legacy_page_test.LegacyPageTest)
    is_tbm = issubclass(
        self.test, timeline_based_measurement.TimelineBasedMeasurement)
    if not is_page_test and not is_tbm:
      raise TypeError('"%s" is not a PageTest or a TimelineBasedMeasurement.' %
                      self.test.__name__)
    if is_page_test:
      # TODO: assert that CreateCoreTimelineBasedMeasurementOptions is not
      # defined. That's incorrect for a page test. See
      # https://github.com/catapult-project/catapult/issues/3708
      return self.test()  # pylint: disable=no-value-for-parameter

    opts = self._GetTimelineBasedMeasurementOptions(options)
    self.SetupTraceRerunOptions(options, opts)
    return self.test(opts)

  def CreateStorySet(self, options):
    """Creates the instance of StorySet used to run the benchmark.

    Can be overridden by subclasses.
    """
    del options  # unused
    # TODO(aiolos, nednguyen, eakufner): replace class attribute page_set with
    # story_set.
    if not self.page_set:
      raise NotImplementedError('This test has no "page_set" attribute.')
    return self.page_set()  # pylint: disable=not-callable

  def GetBrokenExpectations(self, story_set):
    if self._expectations:
      return self._expectations.GetBrokenExpectations(story_set)
    return []

  def AugmentExpectationsWithParser(self, data):
    parser = expectations_parser.TestExpectationParser(data)
    self._expectations.GetBenchmarkExpectationsFromParser(
        parser.expectations, self.Name())

  @property
  def expectations(self):
    return self._expectations


def AddCommandLineArgs(parser):
  story_runner.AddCommandLineArgs(parser)


def ProcessCommandLineArgs(parser, args):
  story_runner.ProcessCommandLineArgs(parser, args)
