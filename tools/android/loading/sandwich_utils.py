# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

import common_util
import emulation
import sandwich_runner
import task_manager


def NetworkSimulationTransformer(network_condition):
  """Creates a function that accepts a SandwichRunner as a parameter and sets
  network emulation options on it.

  Args:
    network_condition: The network condition to apply to the sandwich runner.

  Returns:
    A callback transforming the SandwichRunner given in argument accordingly
  """
  assert network_condition in emulation.NETWORK_CONDITIONS
  def Transformer(runner):
    assert isinstance(runner, sandwich_runner.SandwichRunner)
    runner.network_condition = network_condition
  return Transformer


def FilterOutDataAndIncompleteRequests(requests):
  for request in filter(lambda r: not r.IsDataRequest(), requests):
    # The protocol is only known once the response has been received. But the
    # trace recording might have been stopped with still some JavaScript
    # originated requests that have not received any responses yet.
    if request.protocol is None:
      assert not request.HasReceivedResponse()
      continue
    if request.protocol in {'about'}:
      continue
    if request.protocol not in {'http/0.9', 'http/1.0', 'http/1.1'}:
      raise RuntimeError('Unknown request protocol {}'.format(request.protocol))
    yield request


class RequestOutcome:
  All, ServedFromCache, NotServedFromCache, Post = range(4)


def ListUrlRequests(trace, request_kind):
  """Lists requested URLs from a trace.

  Args:
    trace: (loading_trace.LoadingTrace) loading trace.
    request_kind: RequestOutcome.* indicating the subset of requests to output.

  Returns:
    set([str])
  """
  urls = set()
  for request_event in FilterOutDataAndIncompleteRequests(
      trace.request_track.GetEvents()):
    if (request_kind == RequestOutcome.ServedFromCache and
        request_event.from_disk_cache):
      urls.add(request_event.url)
    elif (request_kind == RequestOutcome.Post and
        request_event.method.upper().strip() == 'POST'):
      urls.add(request_event.url)
    elif (request_kind == RequestOutcome.NotServedFromCache and
        not request_event.from_disk_cache):
      urls.add(request_event.url)
    elif request_kind == RequestOutcome.All:
      urls.add(request_event.url)
  return urls


def PatchWprEntryToBeCached(wpr_url_entry):
  """Patches a WprUrlEntry to ensure the resources to go into the HTTP cache and
  avoid invalidation and revalidations.

  Args:
    wpr_url_entry: Wpr url entry of the resource to put into the cache.
  """
  MAX_AGE = 10 * 365 * 24 * 60 * 60
  CACHE_CONTROL = 'public, max-age={}'.format(MAX_AGE)

  # TODO(gabadie): may need to patch Last-Modified and If-Modified-Since.
  # TODO(gabadie): may need to delete ETag.
  # TODO(gabadie): may need to take care of x-cache.
  #
  # Override the cache-control header to set the resources max age to MAX_AGE.
  #
  # Important note: Some resources holding sensitive information might have
  # cache-control set to no-store which allow the resource to be cached but
  # not cached in the file system. NoState-Prefetch is going to take care of
  # this case. But in here, to simulate NoState-Prefetch, we don't have other
  # choices but save absolutely all cached resources on disk so they survive
  # after killing chrome for cache save, modification and push.
  wpr_url_entry.SetResponseHeader('cache-control', CACHE_CONTROL)

  # TODO(gabadie): May need to extend Vary blacklist (referer?)
  #
  # All of these Vary and Pragma possibilities need to be removed from
  # response headers in order for Chrome to store a resource in HTTP cache and
  # not to invalidate it.
  wpr_url_entry.RemoveResponseHeaderDirectives('vary', {'*', 'cookie'})
  wpr_url_entry.RemoveResponseHeaderDirectives('pragma', {'no-cache'})


class SandwichCommonBuilder(task_manager.Builder):
  """A builder for a graph of tasks, each prepares or invokes a SandwichRunner.
  """

  def __init__(self, android_device, url, output_directory,
               output_subdirectory):
    """Constructor.

    Args:
      android_device: The android DeviceUtils to run sandwich on or None to run
        it locally.
      url: URL to benchmark.
      output_directory: As in task_manager.Builder.__init__
      output_subdirectory: As in task_manager.Builder.__init__
    """
    task_manager.Builder.__init__(self, output_directory, output_subdirectory)
    self._android_device = android_device
    self._url = url
    self.default_final_tasks = []

    self.original_wpr_task = None
    self.original_wpr_recording_trace_path = None

  def CreateSandwichRunner(self):
    """Create a runner for non benchmark purposes."""
    runner = sandwich_runner.SandwichRunner()
    runner.url = self._url
    runner.android_device = self._android_device
    return runner

  def PopulateWprRecordingTask(self):
    """Records the original WPR archive."""
    @self.RegisterTask('common/webpages.wpr')
    def BuildOriginalWpr():
      common_util.EnsureParentDirectoryExists(BuildOriginalWpr.path)
      runner = self.CreateSandwichRunner()
      runner.wpr_archive_path = BuildOriginalWpr.path
      runner.wpr_record = True
      runner.output_dir = BuildOriginalWpr.run_path
      runner.Run()
    BuildOriginalWpr.run_path = BuildOriginalWpr.path[:-4] + '-run'

    self.original_wpr_task = BuildOriginalWpr
    self.original_wpr_recording_trace_path = os.path.join(
        BuildOriginalWpr.run_path, '0', sandwich_runner.TRACE_FILENAME)
