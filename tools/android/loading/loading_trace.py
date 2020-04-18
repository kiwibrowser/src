# Copyright (c) 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Represents the trace of a page load."""

import datetime
try:
  import ujson as json
except ImportError:
  import json
import time

import devtools_monitor
import page_track
import request_track
import tracing_track


class LoadingTrace(object):
  """Represents the trace of a page load."""
  _URL_KEY = 'url'
  _METADATA_KEY = 'metadata'
  _PAGE_KEY = 'page_track'
  _REQUEST_KEY = 'request_track'
  _TRACING_KEY = 'tracing_track'

  def __init__(self, url, metadata, page, request, track):
    """Initializes a loading trace instance.

    Args:
      url: (str) URL that has been loaded
      metadata: (dict) Metadata associated with the load.
      page: (PageTrack) instance of PageTrack.
      request: (RequestTrack) instance of RequestTrack.
      track: (TracingTrack) instance of TracingTrack.
    """
    self.url = url
    self.metadata = metadata
    self.page_track = page
    self.request_track = request
    self._tracing_track = track
    self._tracing_json_str = None

  def ToJsonDict(self):
    """Returns a dictionary representing this instance."""
    result = {self._URL_KEY: self.url, self._METADATA_KEY: self.metadata,
              self._PAGE_KEY: self.page_track.ToJsonDict(),
              self._REQUEST_KEY: self.request_track.ToJsonDict(),
              self._TRACING_KEY: (self.tracing_track.ToJsonDict()
                                  if self.tracing_track else None)}
    return result

  def ToJsonFile(self, json_path):
    """Save a json file representing this instance."""
    json_dict = self.ToJsonDict()
    with open(json_path, 'w') as output_file:
       json.dump(json_dict, output_file)

  @classmethod
  def FromJsonDict(cls, json_dict):
    """Returns an instance from a dictionary returned by ToJsonDict()."""
    keys = (cls._URL_KEY, cls._METADATA_KEY, cls._PAGE_KEY, cls._REQUEST_KEY,
            cls._TRACING_KEY)
    assert all(key in json_dict for key in keys)
    page = page_track.PageTrack.FromJsonDict(json_dict[cls._PAGE_KEY])
    request = request_track.RequestTrack.FromJsonDict(
        json_dict[cls._REQUEST_KEY])
    track = tracing_track.TracingTrack.FromJsonDict(
        json_dict[cls._TRACING_KEY])
    return LoadingTrace(json_dict[cls._URL_KEY], json_dict[cls._METADATA_KEY],
                        page, request, track)

  @classmethod
  def FromJsonFile(cls, json_path):
    """Returns an instance from a json file saved by ToJsonFile()."""
    with open(json_path) as input_file:
      return cls.FromJsonDict(json.load(input_file))

  @classmethod
  def RecordUrlNavigation(
      cls, url, connection, chrome_metadata, categories,
      timeout_seconds=devtools_monitor.DEFAULT_TIMEOUT_SECONDS,
      stop_delay_multiplier=0):
    """Create a loading trace by using controller to fetch url.

    Args:
      url: (str) url to fetch.
      connection: An opened devtools connection.
      chrome_metadata: Dictionary of chrome metadata.
      categories: as in tracing_track.TracingTrack
      timeout_seconds: monitoring connection timeout in seconds.
      stop_delay_multiplier: How long to wait after page load completed before
        tearing down, relative to the time it took to reach the page load to
        complete.

    Returns:
      LoadingTrace instance.
    """
    page = page_track.PageTrack(connection)
    request = request_track.RequestTrack(connection)
    trace = tracing_track.TracingTrack(connection, categories)
    start_date_str = datetime.datetime.utcnow().isoformat()
    seconds_since_epoch=time.time()
    connection.MonitorUrl(url,
                          timeout_seconds=timeout_seconds,
                          stop_delay_multiplier=stop_delay_multiplier)
    trace = cls(url, chrome_metadata, page, request, trace)
    trace.metadata.update(date=start_date_str,
                          seconds_since_epoch=seconds_since_epoch)
    return trace

  @property
  def tracing_track(self):
    if not self._tracing_track:
      self._RestoreTracingTrack()
    return self._tracing_track

  def Slim(self):
    """Slims the memory usage of a trace by dropping the TraceEvents from it.

    The tracing track is restored on-demand when accessed.
    """
    self._tracing_json_str = json.dumps(self._tracing_track.ToJsonDict())
    self._tracing_track = None

  def _RestoreTracingTrack(self):
    if not self._tracing_json_str:
      return None
    self._tracing_track = tracing_track.TracingTrack.FromJsonDict(
        json.loads(self._tracing_json_str))
    self._tracing_json_str = None
