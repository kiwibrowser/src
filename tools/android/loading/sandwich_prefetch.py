# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Implements a task builder for benchmarking effects of NoState Prefetch.
Noticeable steps of the task pipeline:
  * Save a WPR archive
  * Process the WPR archive to make all resources cacheable
  * Process cache archive to patch response headers back to their original
      values.
  * Find out which resources are discoverable by NoState Prefetch
      (HTMLPreloadScanner)
  * Load pages with empty/full/prefetched cache
  * Extract most important metrics to a CSV
"""

import csv
import logging
import json
import os
import re
import shutil
import urlparse

import chrome_cache
import common_util
import loading_trace
from prefetch_view import PrefetchSimulationView
from request_dependencies_lens import RequestDependencyLens
import sandwich_metrics
import sandwich_runner
import sandwich_utils
import task_manager
import wpr_backend


class Discoverer(object):
  # Do not prefetch anything.
  EmptyCache = 'empty-cache'

  # Prefetches everything to load fully from cache (impossible in practice).
  FullCache = 'full-cache'

  # Prefetches the first resource following the redirection chain.
  MainDocument = 'main-document'

  # All resources which are fetched from the main document and their
  # redirections.
  Parser = 'parser'

  # Simulation of HTMLPreloadScanner on the main document and their
  # redirections and subsets:
  #   Store: only resources that don't have Cache-Control: No-Store.
  HTMLPreloadScanner = 'html-scanner'
  HTMLPreloadScannerStore = 'html-scanner-store'


# List of all available sub-resource discoverers.
SUBRESOURCE_DISCOVERERS = set([
  Discoverer.EmptyCache,
  Discoverer.FullCache,
  Discoverer.MainDocument,
  Discoverer.Parser,
  Discoverer.HTMLPreloadScanner,
  Discoverer.HTMLPreloadScannerStore,
])


_UPLOAD_DATA_STREAM_REQUESTS_REGEX = re.compile(r'^\d+/(?P<url>.*)$')


def _NormalizeUrl(url):
  """Returns normalized URL such as removing trailing slashes."""
  parsed_url = list(urlparse.urlparse(url))
  parsed_url[2] = re.sub(r'/{2,}', r'/', parsed_url[2])
  return urlparse.urlunparse(parsed_url)


def _PatchCacheArchive(cache_archive_path, loading_trace_path,
                       cache_archive_dest_path):
  """Patch the cache archive.

  Note: This method update the raw response headers of cache entries' to store
    the ones such as Set-Cookie that were pruned by the
    net::HttpCacheTransaction, and remove the stream index 2 holding resource's
    compile meta data.

  Args:
    cache_archive_path: Input archive's path to patch.
    loading_trace_path: Path of the loading trace that have recorded the cache
        archive <cache_archive_path>.
    cache_archive_dest_path: Archive destination's path.
  """
  trace = loading_trace.LoadingTrace.FromJsonFile(loading_trace_path)
  with common_util.TemporaryDirectory(prefix='sandwich_tmp') as tmp_path:
    cache_path = os.path.join(tmp_path, 'cache')
    chrome_cache.UnzipDirectoryContent(cache_archive_path, cache_path)
    cache_backend = chrome_cache.CacheBackend(cache_path, 'simple')
    cache_entries = set(cache_backend.ListKeys())
    logging.info('Original cache size: %d bytes' % cache_backend.GetSize())
    for request in sandwich_utils.FilterOutDataAndIncompleteRequests(
        trace.request_track.GetEvents()):
      # On requests having an upload data stream such as POST requests,
      # net::HttpCache::GenerateCacheKey() prefixes the cache entry's key with
      # the upload data stream's session unique identifier.
      #
      # It is fine to not patch these requests since when reopening Chrome,
      # there is no way the entry can be reused since the upload data stream's
      # identifier will be different.
      #
      # The fact that these entries are kept in the cache after closing Chrome
      # properly by closing the Chrome tab as the ChromeControler.SetSlowDeath()
      # do is known chrome bug (crbug.com/610725).
      if request.url not in cache_entries:
        continue
      # Chrome prunes Set-Cookie from response headers before storing them in
      # disk cache. Also, it adds implicit "Vary: cookie" header to all redirect
      # response headers. Sandwich manages the cache, but between recording the
      # cache and benchmarking the cookie jar is invalidated. This leads to
      # invalidation of all cacheable redirects.
      raw_headers = request.GetRawResponseHeaders()
      cache_backend.UpdateRawResponseHeaders(request.url, raw_headers)
      # NoState-Prefetch would only fetch the resources, but not parse them.
      cache_backend.DeleteStreamForKey(request.url, 2)
    chrome_cache.ZipDirectoryContent(cache_path, cache_archive_dest_path)
    logging.info('Patched cache size: %d bytes' % cache_backend.GetSize())


def _DiscoverRequests(dependencies_lens, subresource_discoverer):
  trace = dependencies_lens.loading_trace
  first_resource_request = trace.request_track.GetFirstResourceRequest()

  if subresource_discoverer == Discoverer.EmptyCache:
    requests = []
  elif subresource_discoverer == Discoverer.FullCache:
    requests = dependencies_lens.loading_trace.request_track.GetEvents()
  elif subresource_discoverer == Discoverer.MainDocument:
    requests = [dependencies_lens.GetRedirectChain(first_resource_request)[-1]]
  elif subresource_discoverer == Discoverer.Parser:
    requests = PrefetchSimulationView.ParserDiscoverableRequests(
        first_resource_request, dependencies_lens)
  elif subresource_discoverer == Discoverer.HTMLPreloadScanner:
    requests = PrefetchSimulationView.PreloadedRequests(
        first_resource_request, dependencies_lens, trace)
  else:
    assert False
  logging.info('number of requests discovered by %s: %d',
      subresource_discoverer, len(requests))
  return requests


def _PruneOutOriginalNoStoreRequests(original_headers_path, requests):
  with open(original_headers_path) as file_input:
    original_headers = json.load(file_input)
  pruned_requests = set()
  for request in requests:
    url = _NormalizeUrl(request.url)
    if url not in original_headers:
      # TODO(gabadie): Investigate why these requests were not in WPR.
      assert request.failed
      logging.warning(
          'could not find original headers for: %s (failure: %s)',
          url, request.error_text)
      continue
    request_original_headers = original_headers[url]
    if ('cache-control' in request_original_headers and
        'no-store' in request_original_headers['cache-control'].lower()):
      pruned_requests.add(request)
  return [r for r in requests if r not in pruned_requests]


def _ExtractDiscoverableUrls(
    original_headers_path, loading_trace_path, subresource_discoverer):
  """Extracts discoverable resource urls from a loading trace according to a
  sub-resource discoverer.

  Args:
    original_headers_path: Path of JSON containing the original headers.
    loading_trace_path: Path of the loading trace recorded at original cache
      creation.
    subresource_discoverer: The sub-resources discoverer that should white-list
      the resources to keep in cache for the NoState-Prefetch benchmarks.

  Returns:
    A set of urls.
  """
  assert subresource_discoverer in SUBRESOURCE_DISCOVERERS, \
      'unknown prefetch simulation {}'.format(subresource_discoverer)
  logging.info('loading %s', loading_trace_path)
  trace = loading_trace.LoadingTrace.FromJsonFile(loading_trace_path)
  dependencies_lens = RequestDependencyLens(trace)

  # Build the list of discovered requests according to the desired simulation.
  discovered_requests = []
  if subresource_discoverer == Discoverer.HTMLPreloadScannerStore:
    requests = _DiscoverRequests(
        dependencies_lens, Discoverer.HTMLPreloadScanner)
    discovered_requests = _PruneOutOriginalNoStoreRequests(
        original_headers_path, requests)
  else:
    discovered_requests = _DiscoverRequests(
        dependencies_lens, subresource_discoverer)

  whitelisted_urls = set()
  for request in sandwich_utils.FilterOutDataAndIncompleteRequests(
      discovered_requests):
    logging.debug('white-listing %s', request.url)
    whitelisted_urls.add(request.url)
  logging.info('number of white-listed resources: %d', len(whitelisted_urls))
  return whitelisted_urls


def _PrintUrlSetComparison(ref_url_set, url_set, url_set_name):
  """Compare URL sets and log the diffs.

  Args:
    ref_url_set: Set of reference urls.
    url_set: Set of urls to compare to the reference.
    url_set_name: The set name for logging purposes.
  """
  assert type(ref_url_set) == set
  assert type(url_set) == set
  if ref_url_set == url_set:
    logging.info('  %d %s are matching.' % (len(ref_url_set), url_set_name))
    return
  missing_urls = ref_url_set.difference(url_set)
  unexpected_urls = url_set.difference(ref_url_set)
  logging.error('  %s are not matching (expected %d, had %d)' % \
      (url_set_name, len(ref_url_set), len(url_set)))
  logging.error('    List of %d missing resources:' % len(missing_urls))
  for url in sorted(missing_urls):
    logging.error('-     ' + url)
  logging.error('    List of %d unexpected resources:' % len(unexpected_urls))
  for url in sorted(unexpected_urls):
    logging.error('+     ' + url)


class _RunOutputVerifier(object):
  """Object to verify benchmark run from traces and WPR log stored in the
  runner output directory.
  """

  def __init__(self, cache_validation_result, benchmark_setup):
    """Constructor.

    Args:
      cache_validation_result: JSON of the cache validation task.
      benchmark_setup: JSON of the benchmark setup.
    """
    self._cache_whitelist = set(benchmark_setup['cache_whitelist'])
    self._original_requests = set(
        cache_validation_result['effective_encoded_data_lengths'].keys())
    self._original_post_requests = set(
        cache_validation_result['effective_post_requests'])
    self._original_cached_requests = self._original_requests.intersection(
        self._cache_whitelist)
    self._original_uncached_requests = self._original_requests.difference(
        self._cache_whitelist)
    self._all_sent_url_requests = set()

  def VerifyTrace(self, trace):
    """Verifies a trace with the cache validation result and the benchmark
    setup.
    """
    effective_requests = sandwich_utils.ListUrlRequests(
        trace, sandwich_utils.RequestOutcome.All)
    effective_post_requests = sandwich_utils.ListUrlRequests(
        trace, sandwich_utils.RequestOutcome.Post)
    effective_cached_requests = sandwich_utils.ListUrlRequests(
        trace, sandwich_utils.RequestOutcome.ServedFromCache)
    effective_uncached_requests = sandwich_utils.ListUrlRequests(
        trace, sandwich_utils.RequestOutcome.NotServedFromCache)

    missing_requests = self._original_requests.difference(effective_requests)
    unexpected_requests = effective_requests.difference(self._original_requests)
    expected_cached_requests = \
        self._original_cached_requests.difference(missing_requests)
    expected_uncached_requests = self._original_uncached_requests.union(
        unexpected_requests).difference(missing_requests)

    # POST requests are known to be unable to use the cache.
    expected_cached_requests.difference_update(effective_post_requests)
    expected_uncached_requests.update(effective_post_requests)

    _PrintUrlSetComparison(self._original_requests, effective_requests,
                           'All resources')
    _PrintUrlSetComparison(set(), effective_post_requests, 'POST resources')
    _PrintUrlSetComparison(expected_cached_requests, effective_cached_requests,
                           'Cached resources')
    _PrintUrlSetComparison(expected_uncached_requests,
                           effective_uncached_requests, 'Non cached resources')

    self._all_sent_url_requests.update(effective_uncached_requests)

  def VerifyWprLog(self, wpr_log_path):
    """Verifies WPR log with previously verified traces."""
    all_wpr_requests = wpr_backend.ExtractRequestsFromLog(wpr_log_path)
    all_wpr_urls = set()
    unserved_wpr_urls = set()
    wpr_command_colliding_urls = set()

    for request in all_wpr_requests:
      if request.is_wpr_host:
        continue
      if urlparse.urlparse(request.url).path.startswith('/web-page-replay'):
        wpr_command_colliding_urls.add(request.url)
      elif request.is_served is False:
        unserved_wpr_urls.add(request.url)
      all_wpr_urls.add(request.url)

    _PrintUrlSetComparison(set(), unserved_wpr_urls,
                           'Distinct unserved resources from WPR')
    _PrintUrlSetComparison(set(), wpr_command_colliding_urls,
                           'Distinct resources colliding to WPR commands')
    _PrintUrlSetComparison(all_wpr_urls, self._all_sent_url_requests,
                           'Distinct resource requests to WPR')


def _ValidateCacheArchiveContent(cache_build_trace_path, cache_archive_path):
  """Validates a cache archive content.

  Args:
    cache_build_trace_path: Path of the generated trace at the cache build time.
    cache_archive_path: Cache archive's path to validate.

  Returns:
    {
      'effective_encoded_data_lengths':
        {URL of all requests: encoded_data_length},
      'effective_post_requests': [URLs of POST requests],
      'expected_cached_resources': [URLs of resources expected to be cached],
      'successfully_cached': [URLs of cached sub-resources]
    }
  """
  # TODO(gabadie): What's the best way of propagating errors happening in here?
  logging.info('lists cached urls from %s' % cache_archive_path)
  with common_util.TemporaryDirectory() as cache_directory:
    chrome_cache.UnzipDirectoryContent(cache_archive_path, cache_directory)
    cache_keys = set(
        chrome_cache.CacheBackend(cache_directory, 'simple').ListKeys())
  trace = loading_trace.LoadingTrace.FromJsonFile(cache_build_trace_path)
  effective_requests = sandwich_utils.ListUrlRequests(
      trace, sandwich_utils.RequestOutcome.All)
  effective_post_requests = sandwich_utils.ListUrlRequests(
      trace, sandwich_utils.RequestOutcome.Post)
  effective_encoded_data_lengths = {}
  for request in sandwich_utils.FilterOutDataAndIncompleteRequests(
      trace.request_track.GetEvents()):
    if request.from_disk_cache or request.served_from_cache:
      # At cache archive creation time, a request might be loaded several times,
      # but avoid the request.encoded_data_length == 0 if loaded from cache.
      continue
    if request.url in effective_encoded_data_lengths:
      effective_encoded_data_lengths[request.url] = max(
          effective_encoded_data_lengths[request.url],
          request.GetResponseTransportLength())
    else:
      effective_encoded_data_lengths[request.url] = (
          request.GetResponseTransportLength())

  upload_data_stream_cache_entry_keys = set()
  upload_data_stream_requests = set()
  for cache_entry_key in cache_keys:
    match = _UPLOAD_DATA_STREAM_REQUESTS_REGEX.match(cache_entry_key)
    if not match:
      continue
    upload_data_stream_cache_entry_keys.add(cache_entry_key)
    upload_data_stream_requests.add(match.group('url'))

  expected_cached_requests = effective_requests.difference(
      effective_post_requests)
  effective_cache_keys = cache_keys.difference(
      upload_data_stream_cache_entry_keys)

  _PrintUrlSetComparison(effective_post_requests, upload_data_stream_requests,
                         'POST resources')
  _PrintUrlSetComparison(expected_cached_requests, effective_cache_keys,
                         'Cached resources')

  return {
      'effective_encoded_data_lengths': effective_encoded_data_lengths,
      'effective_post_requests': [url for url in effective_post_requests],
      'expected_cached_resources': [url for url in expected_cached_requests],
      'successfully_cached_resources': [url for url in effective_cache_keys]
  }


def _ProcessRunOutputDir(
    cache_validation_result, benchmark_setup, runner_output_dir):
  """Process benchmark's run output directory.

  Args:
    cache_validation_result: Same as for _RunOutputVerifier
    benchmark_setup: Same as for _RunOutputVerifier
    runner_output_dir: Same as for SandwichRunner.output_dir

  Returns:
    List of dictionary.
  """
  run_metrics_list = []
  run_output_verifier = _RunOutputVerifier(
      cache_validation_result, benchmark_setup)
  cached_encoded_data_lengths = (
      cache_validation_result['effective_encoded_data_lengths'])
  for repeat_id, repeat_dir in sandwich_runner.WalkRepeatedRuns(
      runner_output_dir):
    trace_path = os.path.join(repeat_dir, sandwich_runner.TRACE_FILENAME)

    logging.info('loading trace: %s', trace_path)
    trace = loading_trace.LoadingTrace.FromJsonFile(trace_path)

    logging.info('verifying trace: %s', trace_path)
    run_output_verifier.VerifyTrace(trace)

    logging.info('extracting metrics from trace: %s', trace_path)

    # Gather response size per URLs.
    response_sizes = {}
    for request in sandwich_utils.FilterOutDataAndIncompleteRequests(
        trace.request_track.GetEvents()):
      # Ignore requests served from the blink's cache.
      if request.served_from_cache:
        continue
      if request.from_disk_cache:
        if request.url in cached_encoded_data_lengths:
          response_size = cached_encoded_data_lengths[request.url]
        else:
          # Some fat webpages may overflow the Memory cache, and so some
          # requests might be served from disk cache couple of times per page
          # load.
          logging.warning('Looks like could be served from memory cache: %s',
              request.url)
          if request.url in response_sizes:
            response_size = response_sizes[request.url]
      else:
        response_size = request.GetResponseTransportLength()
      response_sizes[request.url] = response_size

    # Sums the served from cache/network bytes.
    served_from_network_bytes = 0
    served_from_cache_bytes = 0
    urls_hitting_network = set()
    for request in sandwich_utils.FilterOutDataAndIncompleteRequests(
        trace.request_track.GetEvents()):
      # Ignore requests served from the blink's cache.
      if request.served_from_cache:
        continue
      urls_hitting_network.add(request.url)
      if request.from_disk_cache:
        served_from_cache_bytes += response_sizes[request.url]
      else:
        served_from_network_bytes += response_sizes[request.url]

    # Make sure the served from blink's cache requests have at least one
    # corresponding request that was not served from the blink's cache.
    for request in sandwich_utils.FilterOutDataAndIncompleteRequests(
        trace.request_track.GetEvents()):
      assert (request.url in urls_hitting_network or
              not request.served_from_cache)

    run_metrics = {
        'url': trace.url,
        'repeat_id': repeat_id,
        'subresource_discoverer': benchmark_setup['subresource_discoverer'],
        'cache_recording.subresource_count':
            len(cache_validation_result['effective_encoded_data_lengths']),
        'cache_recording.cached_subresource_count_theoretic':
            len(cache_validation_result['successfully_cached_resources']),
        'cache_recording.cached_subresource_count':
            len(cache_validation_result['expected_cached_resources']),
        'benchmark.subresource_count': len(sandwich_utils.ListUrlRequests(
            trace, sandwich_utils.RequestOutcome.All)),
        'benchmark.served_from_cache_count_theoretic':
            len(benchmark_setup['cache_whitelist']),
        'benchmark.served_from_cache_count': len(sandwich_utils.ListUrlRequests(
            trace, sandwich_utils.RequestOutcome.ServedFromCache)),
        'benchmark.served_from_network_bytes': served_from_network_bytes,
        'benchmark.served_from_cache_bytes': served_from_cache_bytes
    }
    run_metrics.update(
        sandwich_metrics.ExtractCommonMetricsFromRepeatDirectory(
            repeat_dir, trace))
    run_metrics_list.append(run_metrics)
  run_metrics_list.sort(key=lambda e: e['repeat_id'])

  wpr_log_path = os.path.join(
      runner_output_dir, sandwich_runner.WPR_LOG_FILENAME)
  logging.info('verifying wpr log: %s', wpr_log_path)
  run_output_verifier.VerifyWprLog(wpr_log_path)
  return run_metrics_list


class PrefetchBenchmarkBuilder(task_manager.Builder):
  """A builder for a graph of tasks for NoState-Prefetch emulated benchmarks."""

  def __init__(self, common_builder):
    task_manager.Builder.__init__(self,
                                  common_builder.output_directory,
                                  common_builder.output_subdirectory)
    self._common_builder = common_builder

    self._original_headers_path = None
    self._wpr_archive_path = None
    self._cache_path = None
    self._trace_from_grabbing_reference_cache = None
    self._cache_validation_task = None
    self._PopulateCommonPipelines()

  def _PopulateCommonPipelines(self):
    """Creates necessary tasks to produce initial cache archive.

    Also creates a task for producing a json file with a mapping of URLs to
    subresources (urls-resources.json).

    Here is the full dependency tree for the returned task:
    common/patched-cache-validation.json
      depends on: common/patched-cache.zip
        depends on: common/original-cache.zip
          depends on: common/webpages-patched.wpr
            depends on: common/webpages.wpr
    """
    self._original_headers_path = self.RebaseOutputPath(
        'common/response-headers.json')

    @self.RegisterTask('common/webpages-patched.wpr',
                       dependencies=[self._common_builder.original_wpr_task])
    def BuildPatchedWpr():
      shutil.copyfile(
          self._common_builder.original_wpr_task.path, BuildPatchedWpr.path)
      wpr_archive = wpr_backend.WprArchiveBackend(BuildPatchedWpr.path)

      # Save up original response headers.
      original_response_headers = {e.url: e.GetResponseHeadersDict() \
          for e in wpr_archive.ListUrlEntries()}
      logging.info('save up response headers for %d resources',
                   len(original_response_headers))
      if not original_response_headers:
        # TODO(gabadie): How is it possible to not even have the main resource
        # in the WPR archive? Example URL can be found in:
        # http://crbug.com/623966#c5
        raise Exception(
            'Looks like no resources were recorded in WPR during: {}'.format(
                self._common_builder.original_wpr_task.name))
      with open(self._original_headers_path, 'w') as file_output:
        json.dump(original_response_headers, file_output)

      # Patch WPR.
      wpr_url_entries = wpr_archive.ListUrlEntries()
      for wpr_url_entry in wpr_url_entries:
        sandwich_utils.PatchWprEntryToBeCached(wpr_url_entry)
      logging.info('number of patched entries: %d', len(wpr_url_entries))
      wpr_archive.Persist()

    @self.RegisterTask('common/original-cache.zip', [BuildPatchedWpr])
    def BuildOriginalCache():
      runner = self._common_builder.CreateSandwichRunner()
      runner.wpr_archive_path = BuildPatchedWpr.path
      runner.cache_archive_path = BuildOriginalCache.path
      runner.cache_operation = sandwich_runner.CacheOperation.SAVE
      runner.output_dir = BuildOriginalCache.run_path
      runner.Run()
    BuildOriginalCache.run_path = BuildOriginalCache.path[:-4] + '-run'
    original_cache_trace_path = os.path.join(
        BuildOriginalCache.run_path, '0', sandwich_runner.TRACE_FILENAME)

    @self.RegisterTask('common/patched-cache.zip', [BuildOriginalCache])
    def BuildPatchedCache():
      _PatchCacheArchive(BuildOriginalCache.path,
          original_cache_trace_path, BuildPatchedCache.path)

    @self.RegisterTask('common/patched-cache-validation.json',
                       [BuildPatchedCache])
    def ValidatePatchedCache():
      cache_validation_result = _ValidateCacheArchiveContent(
          original_cache_trace_path, BuildPatchedCache.path)
      with open(ValidatePatchedCache.path, 'w') as output:
        json.dump(cache_validation_result, output)

    self._wpr_archive_path = BuildPatchedWpr.path
    self._trace_from_grabbing_reference_cache = original_cache_trace_path
    self._cache_path = BuildPatchedCache.path
    self._cache_validation_task = ValidatePatchedCache

    self._common_builder.default_final_tasks.append(ValidatePatchedCache)

  def PopulateLoadBenchmark(self, subresource_discoverer,
                            transformer_list_name, transformer_list):
    """Populate benchmarking tasks from its setup tasks.

    Args:
      subresource_discoverer: Name of a subresources discoverer.
      transformer_list_name: A string describing the transformers, will be used
          in Task names (prefer names without spaces and special characters).
      transformer_list: An ordered list of function that takes an instance of
          SandwichRunner as parameter, would be applied immediately before
          SandwichRunner.Run() in the given order.

    Here is the full dependency of the added tree for the returned task:
    <transformer_list_name>/<subresource_discoverer>-metrics.csv
      depends on: <transformer_list_name>/<subresource_discoverer>-run/
        depends on: common/<subresource_discoverer>-cache.zip
          depends on: common/<subresource_discoverer>-setup.json
            depends on: common/patched-cache-validation.json
    """
    additional_column_names = [
        'url',
        'repeat_id',
        'subresource_discoverer',
        'cache_recording.subresource_count',
        'cache_recording.cached_subresource_count_theoretic',
        'cache_recording.cached_subresource_count',
        'benchmark.subresource_count',
        'benchmark.served_from_cache_count_theoretic',
        'benchmark.served_from_cache_count',
        'benchmark.served_from_network_bytes',
        'benchmark.served_from_cache_bytes']

    assert subresource_discoverer in SUBRESOURCE_DISCOVERERS
    assert 'common' not in SUBRESOURCE_DISCOVERERS
    shared_task_prefix = os.path.join('common', subresource_discoverer)
    task_prefix = os.path.join(transformer_list_name, subresource_discoverer)

    @self.RegisterTask(shared_task_prefix + '-setup.json', merge=True,
                       dependencies=[self._cache_validation_task])
    def SetupBenchmark():
      whitelisted_urls = _ExtractDiscoverableUrls(
          original_headers_path=self._original_headers_path,
          loading_trace_path=self._trace_from_grabbing_reference_cache,
          subresource_discoverer=subresource_discoverer)

      common_util.EnsureParentDirectoryExists(SetupBenchmark.path)
      with open(SetupBenchmark.path, 'w') as output:
        json.dump({
            'cache_whitelist': [url for url in whitelisted_urls],
            'subresource_discoverer': subresource_discoverer,
          }, output)

    @self.RegisterTask(shared_task_prefix + '-cache.zip', merge=True,
                       dependencies=[SetupBenchmark])
    def BuildBenchmarkCacheArchive():
      benchmark_setup = json.load(open(SetupBenchmark.path))
      chrome_cache.ApplyUrlWhitelistToCacheArchive(
          cache_archive_path=self._cache_path,
          whitelisted_urls=benchmark_setup['cache_whitelist'],
          output_cache_archive_path=BuildBenchmarkCacheArchive.path)

    @self.RegisterTask(task_prefix + '-run/',
                       dependencies=[BuildBenchmarkCacheArchive])
    def RunBenchmark():
      runner = self._common_builder.CreateSandwichRunner()
      for transformer in transformer_list:
        transformer(runner)
      runner.wpr_archive_path = self._common_builder.original_wpr_task.path
      runner.wpr_out_log_path = os.path.join(
          RunBenchmark.path, sandwich_runner.WPR_LOG_FILENAME)
      runner.cache_archive_path = BuildBenchmarkCacheArchive.path
      runner.cache_operation = sandwich_runner.CacheOperation.PUSH
      runner.output_dir = RunBenchmark.path
      runner.Run()

    @self.RegisterTask(task_prefix + '-metrics.csv',
                       dependencies=[RunBenchmark])
    def ProcessRunOutputDir():
      benchmark_setup = json.load(open(SetupBenchmark.path))
      cache_validation_result = json.load(
          open(self._cache_validation_task.path))

      run_metrics_list = _ProcessRunOutputDir(
          cache_validation_result, benchmark_setup, RunBenchmark.path)
      with open(ProcessRunOutputDir.path, 'w') as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=(additional_column_names +
                                    sandwich_metrics.COMMON_CSV_COLUMN_NAMES))
        writer.writeheader()
        for trace_metrics in run_metrics_list:
          writer.writerow(trace_metrics)

    self._common_builder.default_final_tasks.append(ProcessRunOutputDir)
