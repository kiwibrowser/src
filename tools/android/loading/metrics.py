# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Descriptive metrics for Clovis.

When executed as a script, prints the amount of data attributed to Ads, and
shows a graph of the amount of data to download for a new visit to the same
page, with a given time interval.
"""

import collections
import urlparse

import content_classification_lens
from request_track import CachingPolicy

HTTP_OK_LENGTH = len("HTTP/1.1 200 OK\r\n")

def _RequestTransferSize(request):
  def HeadersSize(headers):
    # 4: ':', ' ', '\r', '\n'
    return sum(len(k) + len(v) + 4 for (k, v) in headers.items())
  if request.protocol == 'data':
    return {'get': 0, 'request_headers': 0, 'response_headers': 0, 'body': 0}
  return {'get': len('GET ') + len(request.url) + 2,
          'request_headers': HeadersSize(request.request_headers or {}),
          'response_headers': HeadersSize(request.response_headers or {}),
          'body': request.encoded_data_length}


def TransferSize(requests):
  """Returns the total transfer size (uploaded, downloaded) of requests.

  This is an estimate as we assume:
  - 200s (for the size computation)
  - GET only.

  Args:
    requests: ([Request]) List of requests.

  Returns:
    (uploaded_bytes (int), downloaded_bytes (int))
  """
  uploaded_bytes = 0
  downloaded_bytes = 0
  for request in requests:
    request_bytes = _RequestTransferSize(request)
    uploaded_bytes += request_bytes['get'] + request_bytes['request_headers']
    downloaded_bytes += (HTTP_OK_LENGTH
                         + request_bytes['response_headers']
                         + request_bytes['body'])
  return (uploaded_bytes, downloaded_bytes)


def TotalTransferSize(trace):
  """Returns the total transfer size (uploaded, downloaded) from a trace."""
  return TransferSize(trace.request_track.GetEvents())


def TransferredDataRevisit(trace, after_time_s, assume_validation_ok=False):
  """Returns the amount of data transferred for a revisit.

  Args:
    trace: (LoadingTrace) loading trace.
    after_time_s: (float) Time in s after which the site is revisited.
    assume_validation_ok: (bool) Assumes that the resources to validate return
                          304s.

  Returns:
    (uploaded_bytes, downloaded_bytes)
  """
  uploaded_bytes = 0
  downloaded_bytes = 0
  for request in trace.request_track.GetEvents():
    caching_policy = CachingPolicy(request)
    policy = caching_policy.PolicyAtDate(request.wall_time + after_time_s)
    request_bytes = _RequestTransferSize(request)
    if policy == CachingPolicy.VALIDATION_NONE:
      continue
    uploaded_bytes += request_bytes['get'] + request_bytes['request_headers']
    if (policy in (CachingPolicy.VALIDATION_SYNC,
                   CachingPolicy.VALIDATION_ASYNC)
        and caching_policy.HasValidators() and assume_validation_ok):
      downloaded_bytes += len('HTTP/1.1 304 NOT MODIFIED\r\n')
      continue
    downloaded_bytes += (HTTP_OK_LENGTH
                         + request_bytes['response_headers']
                         + request_bytes['body'])
  return (uploaded_bytes, downloaded_bytes)


def AdsAndTrackingTransferSize(trace, ad_rules_filename,
                               tracking_rules_filename):
  """Returns the transfer size attributed to ads and tracking.

  Args:
    trace: (LoadingTrace) a loading trace.
    ad_rules_filename: (str) Path to an ad rules file.
    tracking_rules_filename: (str) Path to a tracking rules file.

  Returns:
    (uploaded_bytes (int), downloaded_bytes (int))
  """
  content_lens = (
      content_classification_lens.ContentClassificationLens.WithRulesFiles(
          trace, ad_rules_filename, tracking_rules_filename))
  requests = content_lens.AdAndTrackingRequests()
  return TransferSize(requests)


def DnsRequestsAndCost(trace):
  """Returns the number and cost of DNS requests for a trace."""
  requests = trace.request_track.GetEvents()
  requests_with_dns = [r for r in requests if r.timing.dns_start != -1]
  dns_requests_count = len(requests_with_dns)
  dns_cost = sum(r.timing.dns_end - r.timing.dns_start
                 for r in requests_with_dns)
  return (dns_requests_count, dns_cost)


def ConnectionMetrics(trace):
  """Returns the connection metrics for a given trace.

  Returns:
  {
    'connections': int,
    'connection_cost_ms': float,
    'ssl_connections': int,
    'ssl_cost_ms': float,
    'http11_requests': int,
    'h2_requests': int,
    'data_requests': int,
    'domains': int
  }
  """
  requests = trace.request_track.GetEvents()
  requests_with_connect = [r for r in requests if r.timing.connect_start != -1]
  requests_with_connect_count = len(requests_with_connect)
  connection_cost = sum(r.timing.connect_end - r.timing.connect_start
                        for r in requests_with_connect)
  ssl_requests = [r for r in requests if r.timing.ssl_start != -1]
  ssl_requests_count = len(ssl_requests)
  ssl_cost = sum(r.timing.ssl_end - r.timing.ssl_start for r in ssl_requests)
  requests_per_protocol = collections.defaultdict(int)
  for r in requests:
    requests_per_protocol[r.protocol] += 1

  domains = set()
  for r in requests:
    if r.protocol == 'data':
      continue
    domain = urlparse.urlparse(r.url).hostname
    domains.add(domain)

  return {
    'connections': requests_with_connect_count,
    'connection_cost_ms': connection_cost,
    'ssl_connections': ssl_requests_count,
    'ssl_cost_ms': ssl_cost,
    'http11_requests': requests_per_protocol['http/1.1'],
    'h2_requests': requests_per_protocol['h2'],
    'data_requests': requests_per_protocol['data'],
    'domains': len(domains)
  }


def PlotTransferSizeVsTimeBetweenVisits(trace):
  times = [10, 60, 300, 600, 3600, 4 * 3600, 12 * 3600, 24 * 3600]
  labels = ['10s', '1m', '10m', '1h', '4h', '12h', '1d']
  (_, total_downloaded) = TotalTransferSize(trace)
  downloaded = [TransferredDataRevisit(trace, delta_t)[1] for delta_t in times]
  plt.figure()
  plt.title('Amount of data to download for a revisit - %s' % trace.url)
  plt.xlabel('Time between visits (log)')
  plt.ylabel('Amount of data (bytes)')
  plt.plot(times, downloaded, 'k+--')
  plt.axhline(total_downloaded, color='k', linewidth=2)
  plt.xscale('log')
  plt.xticks(times, labels)
  plt.show()


def main(trace_filename, ad_rules_filename, tracking_rules_filename):
  trace = loading_trace.LoadingTrace.FromJsonFile(trace_filename)
  (_, ads_downloaded_bytes) = AdsAndTrackingTransferSize(
      trace, ad_rules_filename, tracking_rules_filename)
  (_, total_downloaded_bytes) = TotalTransferSize(trace)
  print '%e bytes linked to Ads/Tracking (%.02f%%)' % (
      ads_downloaded_bytes,
      (100. * ads_downloaded_bytes) / total_downloaded_bytes)
  PlotTransferSizeVsTimeBetweenVisits(trace)


if __name__ == '__main__':
  import sys
  from matplotlib import pylab as plt
  import loading_trace
  if len(sys.argv) != 4:
    print (
        'Usage: %s trace_filename ad_rules_filename tracking_rules_filename'
        % sys.argv[0])
    sys.exit(0)
  main(*sys.argv[1:])
