# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import copy
import datetime
import json
import logging
import os
import random
import sys
import tempfile
import time
import traceback
import uuid

from py_utils import cloud_storage  # pylint: disable=import-error

from telemetry import value as value_module
from telemetry.internal.results import chart_json_output_formatter
from telemetry.internal.results import html_output_formatter
from telemetry.internal.results import progress_reporter as reporter_module
from telemetry.internal.results import story_run
from telemetry.value import skip
from telemetry.value import trace

from tracing.value import convert_chart_json
from tracing.value import histogram
from tracing.value import histogram_set
from tracing.value.diagnostics import generic_set
from tracing.value.diagnostics import reserved_infos

class TelemetryInfo(object):
  def __init__(self, upload_bucket=None, output_dir=None):
    self._benchmark_name = None
    self._benchmark_start_epoch = None
    self._benchmark_interrupted = False
    self._benchmark_descriptions = None
    self._label = None
    self._story_name = ''
    self._story_tags = set()
    self._story_grouping_keys = {}
    self._storyset_repeat_counter = 0
    self._trace_start_ms = None
    self._upload_bucket = upload_bucket
    self._trace_remote_path = None
    self._output_dir = output_dir
    self._trace_local_path = None
    self._had_failures = None

  @property
  def upload_bucket(self):
    return self._upload_bucket

  @property
  def benchmark_name(self):
    return self._benchmark_name

  @benchmark_name.setter
  def benchmark_name(self, benchmark_name):
    assert self.benchmark_name is None, (
        'benchmark_name must be set exactly once')
    self._benchmark_name = benchmark_name

  @property
  def benchmark_start_epoch(self):
    return self._benchmark_start_epoch

  @benchmark_start_epoch.setter
  def benchmark_start_epoch(self, benchmark_start_epoch):
    assert self.benchmark_start_epoch is None, (
        'benchmark_start_epoch must be set exactly once')
    self._benchmark_start_epoch = benchmark_start_epoch

  @property
  def benchmark_descriptions(self):
    return self._benchmark_descriptions

  @benchmark_descriptions.setter
  def benchmark_descriptions(self, benchmark_descriptions):
    assert self._benchmark_descriptions is None, (
        'benchmark_descriptions must be set exactly once')
    self._benchmark_descriptions = benchmark_descriptions

  @property
  def trace_start_ms(self):
    return self._trace_start_ms

  @property
  def benchmark_interrupted(self):
    return self._benchmark_interrupted

  @property
  def label(self):
    return self._label

  @label.setter
  def label(self, label):
    assert self.label is None, 'label cannot be set more than once'
    self._label = label

  @property
  def story_display_name(self):
    return self._story_name

  @property
  def story_grouping_keys(self):
    return self._story_grouping_keys

  @property
  def story_tags(self):
    return self._story_tags

  @property
  def storyset_repeat_counter(self):
    return self._storyset_repeat_counter

  @property
  def had_failures(self):
    return self._had_failures

  @had_failures.setter
  def had_failures(self, had_failures):
    assert self.had_failures is None, (
        'had_failures cannot be set more than once')
    self._had_failures = had_failures

  def InterruptBenchmark(self):
    self._benchmark_interrupted = True

  def WillRunStory(self, story, storyset_repeat_counter):
    self._trace_start_ms = 1000 * time.time()
    self._story_name = story.name
    self._story_grouping_keys = story.grouping_keys
    self._story_tags = story.tags
    self._storyset_repeat_counter = storyset_repeat_counter

    trace_name = '%s_%s_%s.html' % (
        story.file_safe_name,
        datetime.datetime.now().strftime('%Y-%m-%d_%H-%M-%S'),
        random.randint(1, 1e5))

    if self._upload_bucket:
      self._trace_remote_path = trace_name

    if self._output_dir:
      self._trace_local_path = os.path.abspath(os.path.join(
          self._output_dir, trace_name))

  @property
  def trace_local_path(self):
    return self._trace_local_path

  @property
  def trace_local_url(self):
    if self._trace_local_path:
      return 'file://' + self._trace_local_path
    return None

  @property
  def trace_remote_path(self):
    return self._trace_remote_path

  @property
  def trace_remote_url(self):
    if self._trace_remote_path:
      return 'https://console.developers.google.com/m/cloudstorage/b/%s/o/%s' % (
          self._upload_bucket, self._trace_remote_path)
    return None

  @property
  def trace_url(self):
    # This is MRE's canonicalUrl.
    if self._upload_bucket is None:
      return self.trace_local_url
    return self.trace_remote_url

  def AsDict(self):
    assert self.benchmark_name is not None, (
        'benchmark_name must be set exactly once')
    assert self.benchmark_start_epoch is not None, (
        'benchmark_start_epoch must be set exactly once')
    d = {}
    d[reserved_infos.BENCHMARKS.name] = [self.benchmark_name]
    d[reserved_infos.BENCHMARK_START.name] = self.benchmark_start_epoch * 1000
    if self.benchmark_descriptions:
      d[reserved_infos.BENCHMARK_DESCRIPTIONS.name] = [
          self.benchmark_descriptions]
    if self.label:
      d[reserved_infos.LABELS.name] = [self.label]
    d[reserved_infos.STORIES.name] = [self._story_name]
    d[reserved_infos.STORY_TAGS.name] = list(self.story_tags) + [
        '%s:%s' % kv for kv in self.story_grouping_keys.iteritems()]
    d[reserved_infos.STORYSET_REPEATS.name] = [self.storyset_repeat_counter]
    d[reserved_infos.TRACE_START.name] = self.trace_start_ms
    d[reserved_infos.TRACE_URLS.name] = [self.trace_url]
    if self.had_failures:
      d[reserved_infos.HAD_FAILURES.name] = [self.had_failures]
    return d


class PageTestResults(object):
  def __init__(self, output_formatters=None,
               progress_reporter=None, trace_tag='', output_dir=None,
               should_add_value=lambda v, is_first: True,
               benchmark_enabled=True, upload_bucket=None,
               artifact_results=None, benchmark_metadata=None):
    """
    Args:
      output_formatters: A list of output formatters. The output
          formatters are typically used to format the test results, such
          as CsvPivotTableOutputFormatter, which output the test results as CSV.
      progress_reporter: An instance of progress_reporter.ProgressReporter,
          to be used to output test status/results progressively.
      trace_tag: A string to append to the buildbot trace name. Currently only
          used for buildbot.
      output_dir: A string specified the directory where to store the test
          artifacts, e.g: trace, videos,...
      should_add_value: A function that takes two arguments: a value name and
          a boolean (True when the value belongs to the first run of the
          corresponding story). It returns True if the value should be added
          to the test results and False otherwise.
      artifact_results: An artifact results object. This is used to contain
          any artifacts from tests. Stored so that clients can call AddArtifact.
      benchmark_metadata: A benchmark.BenchmarkMetadata object. This is used in
          the chart JSON output formatter.
    """
    # TODO(chrishenry): Figure out if trace_tag is still necessary.

    super(PageTestResults, self).__init__()
    self._progress_reporter = (
        progress_reporter if progress_reporter is not None
        else reporter_module.ProgressReporter())
    self._output_formatters = (
        output_formatters if output_formatters is not None else [])
    self._trace_tag = trace_tag
    self._output_dir = output_dir
    self._should_add_value = should_add_value

    self._current_page_run = None
    self._all_page_runs = []
    self._all_stories = set()
    self._representative_value_for_each_value_name = {}
    self._all_summary_values = []
    self._serialized_trace_file_ids_to_paths = {}

    self._histograms = histogram_set.HistogramSet()

    self._telemetry_info = TelemetryInfo(
        upload_bucket=upload_bucket, output_dir=output_dir)

    # State of the benchmark this set of results represents.
    self._benchmark_enabled = benchmark_enabled

    self._artifact_results = artifact_results
    self._benchmark_metadata = benchmark_metadata

  @property
  def telemetry_info(self):
    return self._telemetry_info

  def AsHistogramDicts(self):
    return self._histograms.AsDicts()

  def PopulateHistogramSet(self):
    if len(self._histograms):
      return

    chart_json = chart_json_output_formatter.ResultsAsChartDict(
        self._benchmark_metadata, self)
    info = self.telemetry_info
    chart_json['label'] = info.label
    chart_json['benchmarkStartMs'] = info.benchmark_start_epoch * 1000.0

    file_descriptor, chart_json_path = tempfile.mkstemp()
    os.close(file_descriptor)
    json.dump(chart_json, file(chart_json_path, 'w'))

    vinn_result = convert_chart_json.ConvertChartJson(chart_json_path)

    os.remove(chart_json_path)

    if vinn_result.returncode != 0:
      logging.error('Error converting chart json to Histograms:\n' +
                    vinn_result.stdout)
      return []
    self._histograms.ImportDicts(json.loads(vinn_result.stdout))
    self._histograms.ResolveRelatedHistograms()

  def __copy__(self):
    cls = self.__class__
    result = cls.__new__(cls)
    for k, v in self.__dict__.items():
      if isinstance(v, collections.Container):
        v = copy.copy(v)
      setattr(result, k, v)
    return result

  @property
  def serialized_trace_file_ids_to_paths(self):
    return self._serialized_trace_file_ids_to_paths

  @property
  def all_page_specific_values(self):
    values = []
    for run in self._all_page_runs:
      values += run.values
    if self._current_page_run:
      values += self._current_page_run.values
    return values

  @property
  def all_summary_values(self):
    return self._all_summary_values

  @property
  def current_page(self):
    assert self._current_page_run, 'Not currently running test.'
    return self._current_page_run.story

  @property
  def current_page_run(self):
    assert self._current_page_run, 'Not currently running test.'
    return self._current_page_run

  @property
  def all_page_runs(self):
    return self._all_page_runs

  @property
  def pages_that_succeeded(self):
    """Returns the set of pages that succeeded.

    Note: This also includes skipped pages.
    """
    pages = set(run.story for run in self.all_page_runs)
    pages.difference_update(self.pages_that_failed)
    return pages

  @property
  def pages_that_succeeded_and_not_skipped(self):
    """Returns the set of pages that succeeded and werent skipped."""
    skipped_stories = [x.page.name for x in self.skipped_values]
    pages = self.pages_that_succeeded
    for page in self.pages_that_succeeded:
      if page.name in skipped_stories:
        pages.remove(page)
    return pages

  @property
  def pages_that_failed(self):
    """Returns the set of failed pages."""
    failed_pages = set()
    for run in self.all_page_runs:
      if run.failed:
        failed_pages.add(run.story)
    return failed_pages

  @property
  def had_failures(self):
    return any(run.failed for run in self.all_page_runs)

  @property
  def num_failed(self):
    return sum(1 for run in self.all_page_runs if run.failed)

  # TODO(#4229): Remove this once tools/perf is migrated.
  @property
  def failures(self):
    return [None] * self.num_failed

  @property
  def skipped_values(self):
    values = self.all_page_specific_values
    return [v for v in values if isinstance(v, skip.SkipValue)]

  @property
  def artifact_results(self):
    return self._artifact_results

  def _GetStringFromExcInfo(self, err):
    return ''.join(traceback.format_exception(*err))

  def CleanUp(self):
    """Clean up any TraceValues contained within this results object."""
    for run in self._all_page_runs:
      for v in run.values:
        if isinstance(v, trace.TraceValue):
          v.CleanUp()
          run.values.remove(v)

  def CloseOutputFormatters(self):
    """
    Clean up any open output formatters contained within this results object
    """
    for output_formatter in self._output_formatters:
      output_formatter.output_stream.close()

  def __enter__(self):
    return self

  def __exit__(self, _, __, ___):
    self.CleanUp()
    self.CloseOutputFormatters()

  def WillRunPage(self, page, storyset_repeat_counter=0):
    assert not self._current_page_run, 'Did not call DidRunPage.'
    self._current_page_run = story_run.StoryRun(page)
    self._progress_reporter.WillRunPage(self)
    self.telemetry_info.WillRunStory(
        page, storyset_repeat_counter)

  def DidRunPage(self, page):  # pylint: disable=unused-argument
    """
    Args:
      page: The current page under test.
    """
    assert self._current_page_run, 'Did not call WillRunPage.'
    self._progress_reporter.DidRunPage(self)
    self._all_page_runs.append(self._current_page_run)
    self._all_stories.add(self._current_page_run.story)
    self._current_page_run = None

  def AddDurationHistogram(self, duration_in_milliseconds):
    hist = histogram.Histogram(
        'benchmark_total_duration', 'ms_smallerIsBetter')
    hist.AddSample(duration_in_milliseconds)
    # TODO(#4244): Do this generally.
    if self.telemetry_info.label:
      hist.diagnostics[reserved_infos.LABELS.name] = generic_set.GenericSet(
          [self.telemetry_info.label])
    hist.diagnostics[reserved_infos.BENCHMARKS.name] = generic_set.GenericSet(
        [self.telemetry_info.benchmark_name])
    hist.diagnostics[reserved_infos.BENCHMARK_START.name] = histogram.DateRange(
        self.telemetry_info.benchmark_start_epoch * 1000)
    if self.telemetry_info.benchmark_descriptions:
      hist.diagnostics[
          reserved_infos.BENCHMARK_DESCRIPTIONS.name] = generic_set.GenericSet([
              self.telemetry_info.benchmark_descriptions])
    self._histograms.AddHistogram(hist)

  def AddHistogram(self, hist):
    if self._ShouldAddHistogram(hist):
      self._histograms.AddHistogram(hist)

  def ImportHistogramDicts(self, histogram_dicts):
    dicts_to_add = []
    for d in histogram_dicts:
      # If there's a type field, it's a diagnostic.
      if 'type' in d:
        dicts_to_add.append(d)
      else:
        hist = histogram.Histogram.FromDict(d)
        if self._ShouldAddHistogram(hist):
          dicts_to_add.append(d)
    self._histograms.ImportDicts(dicts_to_add)

  def _ShouldAddHistogram(self, hist):
    assert self._current_page_run, 'Not currently running test.'
    is_first_result = (
        self._current_page_run.story not in self._all_stories)
    # TODO(eakuefner): Stop doing this once AddValue doesn't exist
    stat_names = [
        '%s_%s' % (hist.name, s) for  s in hist.statistics_scalars.iterkeys()]
    return any(self._should_add_value(s, is_first_result) for s in stat_names)

  def AddValue(self, value):
    assert self._current_page_run, 'Not currently running test.'
    assert self._benchmark_enabled, 'Cannot add value to disabled results'
    self._ValidateValue(value)
    is_first_result = (
        self._current_page_run.story not in self._all_stories)

    story_keys = self._current_page_run.story.grouping_keys

    if story_keys:
      for k, v in story_keys.iteritems():
        assert k not in value.grouping_keys, (
            'Tried to add story grouping key ' + k + ' already defined by ' +
            'value')
        value.grouping_keys[k] = v

      # We sort by key name to make building the tir_label deterministic.
      story_keys_label = '_'.join(v for _, v in sorted(story_keys.iteritems()))
      if value.tir_label:
        assert value.tir_label == story_keys_label, (
            'Value has an explicit tir_label (%s) that does not match the '
            'one computed from story_keys (%s)' % (value.tir_label, story_keys))
      else:
        value.tir_label = story_keys_label

    if not (isinstance(value, skip.SkipValue) or
            isinstance(value, trace.TraceValue) or
            self._should_add_value(value.name, is_first_result)):
      return
    # TODO(eakuefner/chrishenry): Add only one skip per pagerun assert here
    self._current_page_run.AddValue(value)
    self._progress_reporter.DidAddValue(value)

  def AddSharedDiagnostic(self, name, diagnostic):
    self._histograms.AddSharedDiagnostic(name, diagnostic)

  def Fail(self, failure):
    """Mark the current story run as failed.

    This method will print a GTest-style failure annotation and mark the
    current story run as failed.

    Args:
      failure: A string or exc_info describing the reason for failure.
    """
    # TODO(#4258): Relax this assertion.
    assert self._current_page_run, 'Not currently running test.'
    if isinstance(failure, basestring):
      failure_str = 'Failure recorded: %s' % failure
    else:
      failure_str = ''.join(traceback.format_exception(*failure))
    self._current_page_run.SetFailed(failure_str)

  def Skip(self, reason):
    assert self._current_page_run, 'Not currently running test.'
    self.AddValue(skip.SkipValue(self.current_page, reason))

  def CreateArtifact(self, story, name, prefix='', suffix=''):
    return self._artifact_results.CreateArtifact(
        story, name, prefix=prefix, suffix=suffix)

  def AddArtifact(self, story, name, path):
    self._artifact_results.AddArtifact(story, name, path)

  def AddSummaryValue(self, value):
    assert value.page is None
    self._ValidateValue(value)
    self._all_summary_values.append(value)

  def _ValidateValue(self, value):
    assert isinstance(value, value_module.Value)
    if value.name not in self._representative_value_for_each_value_name:
      self._representative_value_for_each_value_name[value.name] = value
    representative_value = self._representative_value_for_each_value_name[
        value.name]
    assert value.IsMergableWith(representative_value)

  def PrintSummary(self):
    if self._benchmark_enabled:
      self._progress_reporter.DidFinishAllTests(self)

      # Only serialize the trace if output_format is json or html.
      if (self._output_dir and
          any(isinstance(o, html_output_formatter.HtmlOutputFormatter)
              for o in self._output_formatters)):
        self._SerializeTracesToDirPath()

      for output_formatter in self._output_formatters:
        output_formatter.Format(self)
        output_formatter.PrintViewResults()
    else:
      for output_formatter in self._output_formatters:
        output_formatter.FormatDisabled(self)

  def FindValues(self, predicate):
    """Finds all values matching the specified predicate.

    Args:
      predicate: A function that takes a Value and returns a bool.
    Returns:
      A list of values matching |predicate|.
    """
    values = []
    for value in self.all_page_specific_values:
      if predicate(value):
        values.append(value)
    return values

  def FindPageSpecificValuesForPage(self, page, value_name):
    return self.FindValues(lambda v: v.page == page and v.name == value_name)

  def FindAllPageSpecificValuesNamed(self, value_name):
    return self.FindValues(lambda v: v.name == value_name)

  def FindAllPageSpecificValuesFromIRNamed(self, tir_label, value_name):
    return self.FindValues(lambda v: v.name == value_name
                           and v.tir_label == tir_label)

  def FindAllTraceValues(self):
    return self.FindValues(lambda v: isinstance(v, trace.TraceValue))

  def _SerializeTracesToDirPath(self):
    """ Serialize all trace values to files in dir_path and return a list of
    file handles to those files. """
    for value in self.FindAllTraceValues():
      fh = value.Serialize()
      self._serialized_trace_file_ids_to_paths[fh.id] = fh.GetAbsPath()

  def UploadTraceFilesToCloud(self):
    for value in self.FindAllTraceValues():
      value.UploadToCloud()

  #TODO(crbug.com/772216): Remove this once the uploading is done by Chromium
  # test recipe.
  def UploadArtifactsToCloud(self):
    bucket = self.telemetry_info.upload_bucket
    for test_name, artifacts in self._artifact_results.IterTestAndArtifacts():
      for artifact_type in artifacts:
        total_num_artifacts = len(artifacts[artifact_type])
        for i, artifact_path in enumerate(artifacts[artifact_type]):
          artifact_path = artifacts[artifact_type][i]
          abs_artifact_path = os.path.abspath(os.path.join(
              self._artifact_results.artifact_dir, '..', artifact_path))
          remote_path = str(uuid.uuid1())
          cloud_url = cloud_storage.Insert(
              bucket, remote_path, abs_artifact_path)
          artifacts[artifact_type][i] = cloud_url
          sys.stderr.write(
              'Uploading %s of page %s to %s (%d out of %d)\n' %
              (artifact_type, test_name, cloud_url, i + 1,
               total_num_artifacts))
