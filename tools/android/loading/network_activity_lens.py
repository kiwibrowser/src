# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Gives a picture of the network activity between timestamps."""

import bisect
import collections
import itertools
import operator


class NetworkActivityLens(object):
  """Reconstructs the network activity during a trace.

  The {uploaded,downloaded}_bytes_timeline timelines are:
  ([timestamp_msec], [value_at_timestamp]). Bytes are counted when a
  network event completes.

  The rate timelines are:
  ([timestamp_msec], [rate]), where the rate is computed over the time
  period ending at timestamp_msec.

  For all the timelines, the list of timestamps are identical.
  """
  def __init__(self, trace):
    """Initializes a NetworkActivityLens instance.

    Args:
      trace: (LoadingTrace)
    """
    self._trace = trace
    self._start_end_times = []
    self._active_events_list = []
    self._uploaded_bytes_timeline = []
    self._downloaded_bytes_timeline = []
    self._upload_rate_timeline = []
    self._download_rate_timeline = []
    self._total_downloaded_bytes = 0
    requests = trace.request_track.GetEvents()
    self._network_events = list(itertools.chain.from_iterable(
        NetworkEvent.EventsFromRequest(request) for request in requests))
    self._IndexEvents()
    self._CreateTimelines()

  @property
  def uploaded_bytes_timeline(self): # (timestamps, data)
    return (self._start_end_times, self._uploaded_bytes_timeline)

  @property
  def downloaded_bytes_timeline(self):
    return (self._start_end_times, self._downloaded_bytes_timeline)

  @property
  def upload_rate_timeline(self):
    return (self._start_end_times, self._upload_rate_timeline)

  @property
  def download_rate_timeline(self):
    return (self._start_end_times, self._download_rate_timeline)

  @property
  def total_download_bytes(self):
    return self._total_downloaded_bytes

  def DownloadedBytesAt(self, time_msec):
    """Return the the downloaded bytes at a given timestamp.

    Args:
      time_msec: a timestamp, in the same scale as the timelines.

    Returns:
      The total bytes downloaded up until the time period ending at time_msec.
    """
    # We just do a linear cumulative sum. Currently this is only called a couple
    # of times, so making an indexed cumulative sum does not seem to be worth
    # the bother.
    total_bytes = 0
    previous_msec = self.downloaded_bytes_timeline[0][0]
    for msec, nbytes in zip(*self.downloaded_bytes_timeline):
      if msec < time_msec:
        total_bytes += nbytes
        previous_msec = msec
      else:
        if time_msec > previous_msec:
          fraction_of_chunk = ((time_msec - previous_msec)
                               / (msec - previous_msec))
          total_bytes += float(nbytes) * fraction_of_chunk
        break
    return total_bytes

  def _IndexEvents(self):
    start_end_times_set = set()
    for event in self._network_events:
      start_end_times_set.add(event.start_msec)
      start_end_times_set.add(event.end_msec)
    self._start_end_times = sorted(list(start_end_times_set))
    self._active_events_list = [[] for _ in self._start_end_times]
    for event in self._network_events:
      start_index = bisect.bisect_right(
          self._start_end_times, event.start_msec) - 1
      end_index = bisect.bisect_right(
          self._start_end_times, event.end_msec)
      for index in range(start_index, end_index):
        self._active_events_list[index].append(event)

  def _CreateTimelines(self):
    for (index, timestamp) in enumerate(self._start_end_times):
      upload_rate = sum(
          e.UploadRate() for e in self._active_events_list[index]
          if timestamp != e.end_msec)
      download_rate = sum(
          e.DownloadRate() for e in self._active_events_list[index]
          if timestamp != e.end_msec)
      uploaded_bytes = sum(
          e.UploadedBytes() for e in self._active_events_list[index]
          if timestamp == e.end_msec)
      downloaded_bytes = sum(
          e.DownloadedBytes() for e in self._active_events_list[index]
          if timestamp == e.end_msec)
      self._total_downloaded_bytes += downloaded_bytes
      self._uploaded_bytes_timeline.append(uploaded_bytes)
      self._downloaded_bytes_timeline.append(downloaded_bytes)
      self._upload_rate_timeline.append(upload_rate)
      self._download_rate_timeline.append(download_rate)


class NetworkEvent(object):
  """Represents a network event."""
  KINDS = set(
      ('dns', 'connect', 'send', 'receive_headers', 'receive_body'))
  def __init__(self, request, kind, start_msec, end_msec, chunk_index=None):
    """Creates a NetworkEvent."""
    self._request = request
    self._kind = kind
    self.start_msec = start_msec
    self.end_msec = end_msec
    self._chunk_index = chunk_index

  @classmethod
  def _GetStartEndOffsetsMsec(cls, request, kind, index=None):
    start_offset, end_offset = (0, 0)
    r = request
    if kind == 'dns':
      start_offset = r.timing.dns_start
      end_offset = r.timing.dns_end
    elif kind == 'connect':
      start_offset = r.timing.connect_start
      end_offset = r.timing.connect_end
    elif kind == 'send':
      start_offset = r.timing.send_start
      end_offset = r.timing.send_end
    elif kind == 'receive_headers': # There is no responseReceived timing.
      start_offset = r.timing.send_end
      end_offset = r.timing.receive_headers_end
    elif kind == 'receive_body':
      if index is None:
        start_offset = r.timing.receive_headers_end
        end_offset = r.timing.loading_finished
      else:
        # Some chunks can correspond to no data.
        i = index - 1
        while i >= 0:
          (offset, size) = r.data_chunks[i]
          if size != 0:
            previous_chunk_start = offset
            break
          i -= 1
        else:
          previous_chunk_start = r.timing.receive_headers_end
        start_offset = previous_chunk_start
        end_offset = r.data_chunks[index][0]
    return (start_offset, end_offset)

  @classmethod
  def EventsFromRequest(cls, request):
    # TODO(lizeb): This ignore forced revalidations.
    if (request.from_disk_cache or request.served_from_cache
        or request.IsDataRequest()):
      return []
    events = []
    for kind in cls.KINDS - set(['receive_body']):
      event = cls._EventWithKindFromRequest(request, kind)
      if event:
        events.append(event)
    kind = 'receive_body'
    if request.data_chunks:
      for (index, chunk) in enumerate(request.data_chunks):
        if chunk[0] != 0:
          event = cls._EventWithKindFromRequest(request, kind, index)
          if event:
            events.append(event)
    else:
      event = cls._EventWithKindFromRequest(request, kind, None)
      if event:
        events.append(event)
    return events

  @classmethod
  def _EventWithKindFromRequest(cls, request, kind, index=None):
    (start_offset, end_offset) = cls._GetStartEndOffsetsMsec(
        request, kind, index)
    event = cls(request, kind, request.start_msec + start_offset,
                request.start_msec + end_offset, index)
    if start_offset == -1 or end_offset == -1:
      return None
    return event

  def UploadedBytes(self):
    """Returns the number of bytes uploaded during this event."""
    if self._kind not in ('send'):
      return 0
    # Headers are not compressed (ignoring SPDY / HTTP/2)
    if not self._request.request_headers:
      return 0
    return sum(len(k) + len(str(v)) for (k, v)
               in self._request.request_headers.items())

  def DownloadedBytes(self):
    """Returns the number of bytes downloaded during this event."""
    if self._kind not in ('receive_headers', 'receive_body'):
      return 0
    if self._kind == 'receive_headers':
      return sum(len(k) + len(str(v)) for (k, v)
                 in self._request.response_headers.items())
    else:
      if self._chunk_index is None:
        return self._request.encoded_data_length
      else:
        return self._request.data_chunks[self._chunk_index][1]

  def UploadRate(self):
    """Returns the upload rate of this event in Bytes / s."""
    return 1000 * self.UploadedBytes() / float(self.end_msec - self.start_msec)

  def DownloadRate(self):
    """Returns the download rate of this event in Bytes / s."""
    downloaded_bytes = self.DownloadedBytes()
    value = 1000 * downloaded_bytes / float(self.end_msec - self.start_msec)
    return value
