# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

""" This module implements the Stale-While-Revalidate performance improvement
experiment on third parties' resources.

The top level operations of the experiment are:
  1. Record WPR archive;
  2. Create a patched WPR archive so that all resource are getting cached;
  3. Record original cache using the patched WPR archive;
  4. Setup the benchmark producing the list of URL to enable SWR in a JSON file;
  5. Create the benchmark cache by:
     - Remove No-Store resources;
     - Adding the SWR header on resources that are experimentally required to
       have it;
     - Patch SWR header on resources that already had it to make sure the
       the SWR freshness is not out of date;
     - And restore all other headers so that response headers such as
       Set-Cookie are still in the cache to avoid entropy caused by
       different cookie values.
  6. Run the benchmark;
  7. Extract metrics into CSV files.
"""

import csv
import json
import logging
import os
import shutil
from urlparse import urlparse

import chrome_cache
import common_util
import loading_trace
import request_track
import sandwich_metrics
import sandwich_runner
import sandwich_utils
import task_manager
import wpr_backend


def _ExtractRegexMatchingUrls(urls, domain_regexes):
  urls_to_enable = set()
  for url in urls:
    if url in urls_to_enable:
      continue
    parsed_url = urlparse(url)
    for domain_regex in domain_regexes:
      if domain_regex.search(parsed_url.netloc):
        urls_to_enable.add(url)
        break
  return urls_to_enable


def _BuildBenchmarkCache(
    original_wpr_trace_path, urls_to_enable_swr,
    original_cache_trace_path, original_cache_archive_path,
    cache_archive_dest_path):
  # Load trace that was generated at original cache creation.
  logging.info('loading %s', original_wpr_trace_path)
  trace = loading_trace.LoadingTrace.FromJsonFile(original_wpr_trace_path)

  # Lists URLs that should not be in the cache or already have SWR headers.
  urls_should_not_be_cached = set()
  urls_already_with_swr = set()
  for request in trace.request_track.GetEvents():
    caching_policy = request_track.CachingPolicy(request)
    if not caching_policy.IsCacheable():
      urls_should_not_be_cached.add(request.url)
    elif caching_policy.GetFreshnessLifetimes()[1] > 0:
      urls_already_with_swr.add(request.url)
  # Trace are fat, kill this one to save up memory for the next one to load in
  # this scope.
  del trace

  # Load trace that was generated at original cache creation.
  logging.info('loading %s', original_cache_trace_path)
  trace = loading_trace.LoadingTrace.FromJsonFile(original_cache_trace_path)

  # Create cache contents.
  delete_count = 0
  swr_patch_count = 0
  originaly_swr_patch_count = 0
  noswr_patch_count = 0
  with common_util.TemporaryDirectory(prefix='sandwich_tmp') as tmp_path:
    cache_path = os.path.join(tmp_path, 'cache')
    chrome_cache.UnzipDirectoryContent(original_cache_archive_path, cache_path)
    cache_backend = chrome_cache.CacheBackend(cache_path, 'simple')
    cache_keys = set(cache_backend.ListKeys())
    for request in trace.request_track.GetEvents():
      if request.url not in cache_keys:
        continue
      if request.url in urls_should_not_be_cached:
        cache_backend.DeleteKey(request.url)
        delete_count += 1
        continue
      if not request.HasReceivedResponse():
        continue
      if request.url in urls_to_enable_swr:
        request.SetHTTPResponseHeader(
            'cache-control', 'max-age=0,stale-while-revalidate=315360000')
        request.SetHTTPResponseHeader(
            'last-modified', 'Thu, 23 Jun 2016 11:30:00 GMT')
        swr_patch_count += 1
      elif request.url in urls_already_with_swr:
        # Force to use SWR on resources that originally attempted to use it.
        request.SetHTTPResponseHeader(
            'cache-control', 'max-age=0,stale-while-revalidate=315360000')
        # The resource originally had SWR enabled therefore we don't
        # Last-Modified to repro exactly the performance impact in case these
        # headers were not set properly causing an invalidation instead of a
        # revalidation.
        originaly_swr_patch_count += 1
      else:
        # Force synchronous revalidation.
        request.SetHTTPResponseHeader('cache-control', 'max-age=0')
        noswr_patch_count += 1
      raw_headers = request.GetRawResponseHeaders()
      cache_backend.UpdateRawResponseHeaders(request.url, raw_headers)
    chrome_cache.ZipDirectoryContent(cache_path, cache_archive_dest_path)
  logging.info('patched %d cached resources with forced SWR', swr_patch_count)
  logging.info('patched %d cached resources with original SWR',
      originaly_swr_patch_count)
  logging.info('patched %d cached resources without SWR', noswr_patch_count)
  logging.info('deleted %d cached resources', delete_count)


def _ProcessRunOutputDir(benchmark_setup, runner_output_dir):
  """Process benchmark's run output directory.

  Args:
    cache_validation_result: Same as for _RunOutputVerifier
    benchmark_setup: Same as for _RunOutputVerifier
    runner_output_dir: Same as for SandwichRunner.output_dir

  Returns:
    List of dictionary.
  """
  run_metrics_list = []
  for repeat_id, repeat_dir in sandwich_runner.WalkRepeatedRuns(
      runner_output_dir):
    trace_path = os.path.join(repeat_dir, sandwich_runner.TRACE_FILENAME)
    logging.info('processing trace: %s', trace_path)
    trace = loading_trace.LoadingTrace.FromJsonFile(trace_path)
    served_from_cache_urls = sandwich_utils.ListUrlRequests(
        trace, sandwich_utils.RequestOutcome.ServedFromCache)
    matching_subresource_count_used_from_cache = (
        served_from_cache_urls.intersection(
            set(benchmark_setup['urls_to_enable_swr'])))
    run_metrics = {
        'url': trace.url,
        'repeat_id': repeat_id,
        'benchmark_name': benchmark_setup['benchmark_name'],
        'cache_recording.subresource_count':
            len(benchmark_setup['effective_subresource_urls']),
        'cache_recording.matching_subresource_count':
            len(benchmark_setup['urls_to_enable_swr']),
        'benchmark.matching_subresource_count_used_from_cache':
            len(matching_subresource_count_used_from_cache)
    }
    run_metrics.update(
        sandwich_metrics.ExtractCommonMetricsFromRepeatDirectory(
            repeat_dir, trace))
    run_metrics_list.append(run_metrics)
  return run_metrics_list


class StaleWhileRevalidateBenchmarkBuilder(task_manager.Builder):
  """A builder for a graph of tasks for Stale-While-Revalidate study benchmarks.
  """

  def __init__(self, common_builder):
    task_manager.Builder.__init__(self,
                                  common_builder.output_directory,
                                  common_builder.output_subdirectory)
    self._common_builder = common_builder
    self._patched_wpr_path = None
    self._original_cache_task = None
    self._original_cache_trace_path = None
    self._PopulateCommonPipelines()

  def _PopulateCommonPipelines(self):
    """Creates necessary tasks to produce initial cache archives.

    Here is the full dependency tree for the returned task:
    depends on: common/original-cache.zip
      depends on: common/webpages-patched.wpr
        depends on: common/webpages.wpr
    """
    @self.RegisterTask('common/webpages-patched.wpr',
                       dependencies=[self._common_builder.original_wpr_task])
    def BuildPatchedWpr():
      shutil.copyfile(
          self._common_builder.original_wpr_task.path, BuildPatchedWpr.path)
      wpr_archive = wpr_backend.WprArchiveBackend(BuildPatchedWpr.path)
      wpr_url_entries = wpr_archive.ListUrlEntries()
      for wpr_url_entry in wpr_url_entries:
        sandwich_utils.PatchWprEntryToBeCached(wpr_url_entry)
      logging.info('number of patched entries: %d', len(wpr_url_entries))
      wpr_archive.Persist()

    @self.RegisterTask('common/original-cache.zip',
                       dependencies=[BuildPatchedWpr])
    def BuildOriginalCache():
      runner = self._common_builder.CreateSandwichRunner()
      runner.wpr_archive_path = BuildPatchedWpr.path
      runner.cache_archive_path = BuildOriginalCache.path
      runner.cache_operation = sandwich_runner.CacheOperation.SAVE
      runner.output_dir = BuildOriginalCache.run_path
      runner.Run()
    BuildOriginalCache.run_path = BuildOriginalCache.path[:-4] + '-run'

    self._original_cache_trace_path = os.path.join(
        BuildOriginalCache.run_path, '0', sandwich_runner.TRACE_FILENAME)
    self._patched_wpr_path = BuildPatchedWpr.path
    self._original_cache_task = BuildOriginalCache

  def PopulateBenchmark(self, benchmark_name, domain_regexes,
                        transformer_list_name, transformer_list):
    """Populate benchmarking tasks.

    Args:
      benchmark_name: Name of the benchmark.
      domain_regexes: Compiled regexes of domains to enable SWR.
      transformer_list_name: A string describing the transformers, will be used
          in Task names (prefer names without spaces and special characters).
      transformer_list: An ordered list of function that takes an instance of
          SandwichRunner as parameter, would be applied immediately before
          SandwichRunner.Run() in the given order.


    Here is the full dependency of the added tree for the returned task:
    <transformer_list_name>/<benchmark_name>-metrics.csv
      depends on: <transformer_list_name>/<benchmark_name>-run/
        depends on: common/<benchmark_name>-cache.zip
          depends on: common/<benchmark_name>-setup.json
            depends on: common/patched-cache.zip
    """
    additional_column_names = [
        'url',
        'repeat_id',
        'benchmark_name',

        # Number of resources of the page.
        'cache_recording.subresource_count',

        # Number of resources matching at least one domain regex, to give an
        # idea in the CSV how much the threshold influence additional SWR uses.
        'cache_recording.matching_subresource_count',

        # Number of resources fetched from cache matching at least one domain
        # regex, to give an actual idea if it is possible to have performance
        # improvement on the web page (or not because only XHR), but also tells
        # if the page loading time should see a performance improvement or not
        # compared with a different thresholds.
        'benchmark.matching_subresource_count_used_from_cache']
    shared_task_prefix = os.path.join('common', benchmark_name)
    task_prefix = os.path.join(transformer_list_name, benchmark_name)

    @self.RegisterTask(shared_task_prefix + '-setup.json', merge=True,
                       dependencies=[self._original_cache_task])
    def SetupBenchmark():
      logging.info('loading %s', self._original_cache_trace_path)
      trace = loading_trace.LoadingTrace.FromJsonFile(
          self._original_cache_trace_path)
      logging.info('generating %s', SetupBenchmark.path)
      effective_subresource_urls = sandwich_utils.ListUrlRequests(
          trace, sandwich_utils.RequestOutcome.All)
      urls_to_enable_swr = _ExtractRegexMatchingUrls(
          effective_subresource_urls, domain_regexes)
      logging.info(
          'count of urls to enable SWR: %s', len(urls_to_enable_swr))
      with open(SetupBenchmark.path, 'w') as output:
        json.dump({
            'benchmark_name': benchmark_name,
            'urls_to_enable_swr': [url for url in urls_to_enable_swr],
            'effective_subresource_urls':
                [url for url in effective_subresource_urls]
          }, output)

    @self.RegisterTask(shared_task_prefix + '-cache.zip', merge=True,
                       dependencies=[SetupBenchmark])
    def BuildBenchmarkCacheArchive():
      benchmark_setup = json.load(open(SetupBenchmark.path))
      _BuildBenchmarkCache(
          original_wpr_trace_path=(
              self._common_builder.original_wpr_recording_trace_path),
          urls_to_enable_swr=set(benchmark_setup['urls_to_enable_swr']),
          original_cache_trace_path=self._original_cache_trace_path,
          original_cache_archive_path=self._original_cache_task.path,
          cache_archive_dest_path=BuildBenchmarkCacheArchive.path)

    @self.RegisterTask(task_prefix + '-run/', [BuildBenchmarkCacheArchive])
    def RunBenchmark():
      runner = self._common_builder.CreateSandwichRunner()
      for transformer in transformer_list:
        transformer(runner)
      runner.wpr_archive_path = self._patched_wpr_path
      runner.wpr_out_log_path = os.path.join(
          RunBenchmark.path, sandwich_runner.WPR_LOG_FILENAME)
      runner.cache_archive_path = BuildBenchmarkCacheArchive.path
      runner.cache_operation = sandwich_runner.CacheOperation.PUSH
      runner.output_dir = RunBenchmark.path
      runner.chrome_args.append('--enable-features=StaleWhileRevalidate2')
      runner.Run()

    @self.RegisterTask(task_prefix + '-metrics.csv', [RunBenchmark])
    def ExtractMetrics():
      benchmark_setup = json.load(open(SetupBenchmark.path))
      run_metrics_list = _ProcessRunOutputDir(
          benchmark_setup, RunBenchmark.path)

      run_metrics_list.sort(key=lambda e: e['repeat_id'])
      with open(ExtractMetrics.path, 'w') as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=(additional_column_names +
                                    sandwich_metrics.COMMON_CSV_COLUMN_NAMES))
        writer.writeheader()
        for run_metrics in run_metrics_list:
          writer.writerow(run_metrics)

    self._common_builder.default_final_tasks.append(ExtractMetrics)
