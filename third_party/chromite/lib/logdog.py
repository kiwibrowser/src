# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Helpers for interacting with LUCI Logdog service."""

from __future__ import print_function

import base64
import json
import time
import urllib

from chromite.lib.protos import annotations_pb2
from chromite.lib import prpc
from chromite.cbuildbot import topology


class LogdogResponseException(Exception):
  """Exception got from invalid Logdog response."""


class LogdogTimeoutException(Exception):
  """Exception got from Logdog request taking too long."""


class LogdogClient(prpc.PRPCClient):
  """Logdog client to interact with the LUCI Logdog service."""

  def _GetHost(self):
    """Get LUCI Logdog Server host from topology."""
    return topology.topology.get(topology.LUCI_LOGDOG_HOST_KEY)

  def LogsQueryAnnotations(self, master, builder, build_num, dryrun=False):
    """Logdog request to query for annotations pathname.

    Args:
      master: waterfall master to query.
      builder: builder to query.
      build_num: build number to query.
      dryrun: Whether a dryrun.

    Returns:
      annotations pathname or None if no streams are available.

    Raises:
      LogdogResponseException if response is invalid.
    """
    body = json.dumps({
        'project': master,
        'tags': {
            'buildbot.master': master,
            'buildbot.builder': builder,
            'buildbot.buildnumber': str(build_num),
        },
        'content_type': 'text/x-chrome-infra-annotations; version=2',
    })
    resp = self.SendRequest('prpc/logdog.Logs', 'Query', body, dryrun=dryrun)
    if 'streams' not in resp or len(resp['streams']) == 0:
      return None
    try:
      return resp['streams'][0]['path']
    except KeyError:
      raise LogdogResponseException('Unable to retrieve logs path')

  def LogsGet(self, project, path, state=True, index=0, byte_count=0,
              log_count=0, dryrun=False):
    """Logdog Logs.Get request to retrieve logs.

    Args:
      project: Logdog project of stream.
      path: Logdog path of stream.
      state: Boolean indicating if state descriptor should be returned.
      index: The initial log stream index to retrieve.
      byte_count: A suggested maximum number of bytes to return.  At least
                  one full log line will be returned.
      log_count: The maximum number of logs to return.
      dryrun: Whether a dryrun.

    Returns:
      Full Logs.Get response protobuf.
    """
    body = json.dumps({
        'project': project,
        'path': path,
        'state': bool(state),
        'index': int(index),
        'byte_count': int(byte_count),
        'log_count': int(log_count),
    })
    return self.SendRequest('prpc/logdog.Logs', 'Get', body, dryrun=dryrun)

  def LogsGetChunked(self, project, path, index=0, log_count=0,
                     chunk_bytes=0, allow_retries=True, dryrun=False):
    """Logdog Logs.Get wrapper to request logs in chunks.

    Args:
      project: Logdog project of stream.
      path: Logdog path of stream.
      index: The initial log stream index to retrieve.
      log_count: The maximum number of logs to return.
      chunk_bytes: A suggested maximum number of bytes to return per chunk.
                   At least one full log line will be returned with each chunk.
      allow_retries: Whether or not retries should be attempted.
      dryrun: Whether a dryrun.

    Returns:
      Generator of Logs.Get response protobufs, one per chunk.
    """
    retry_count = 3 if allow_retries else 0
    terminal_index = None
    while True:
      # Retrieve the next chunk.
      resp = self.LogsGet(project, path, state=True,
                          index=index, byte_count=chunk_bytes,
                          log_count=log_count, dryrun=dryrun)

      # Ensure logs came back.
      logs = resp.get('logs', [])
      count = len(logs)
      if count == 0:
        # Logs haven't yet be ingested, retry.
        if retry_count:
          time.sleep(5)
          retry_count -= 1
          continue
        else:
          terminal = '%d' % (terminal_index) if terminal_index else 'None'
          raise LogdogTimeoutException(
              'Request to retrieve %s %s took too long '
              '(index=%d, byte_count=%d, log_count=%d, terminal=%s)' %
              (project, path, index, chunk_bytes, log_count, terminal))

      # Return the current chunk.
      yield resp

      # Track the total number of logs.
      if log_count > 0:
        log_count -= count
        if log_count <= 0:
          return

      # Keep track of the next index to read from.
      final_index = int(logs[-1].get('streamIndex', 0))

      # Was this the last index in the stream?
      state = resp.get('state', {})
      terminal_index = int(state.get('terminalIndex', 0))
      if terminal_index >= 0 and final_index >= terminal_index:
        return

      # Start reading at the next index.
      index = final_index + 1

  def ExtractLines(self, resp):
    """Generate log lines from a Logs.Get request.

    Args:
      resp: Logs.Get response protobuf.

    Returns:
      Generator of delimited log lines.
    """
    for log in resp.get('logs', []):
      if 'text' in log and 'lines' in log['text']:
        for line in log['text']['lines']:
          yield '%s%s' % (line.get('value', ''), line['delimiter'])

  def GetLines(self, project, path, index=0, log_count=0, chunk_bytes=0,
               allow_retries=True, dryrun=False):
    """Retrieve a Logdog stream as lines of text.

    Args:
      project: Logdog project of stream.
      path: Logdog path of stream.
      index: The initial log stream index to retrieve.
      log_count: The maximum number of logs to return.
      chunk_bytes: A suggested maximum number of bytes to retrieve from the
                   server each time.
      allow_retries: Whether or not retries should be attempted.
      dryrun: Whether a dryrun.

    Returns:
      Generator of delimited log lines.
    """
    for resp in self.LogsGetChunked(project, path, index=index,
                                    log_count=log_count,
                                    chunk_bytes=chunk_bytes,
                                    allow_retries=allow_retries,
                                    dryrun=dryrun):
      for line in self.ExtractLines(resp):
        yield line

  def LogsTail(self, project, path, dryrun=False):
    """Logdog Logs.Tail request to retrieve logs.

    Args:
      project: Logdog project of stream.
      path: Logdog path of stream.
      dryrun: Whether a dryrun.

    Returns:
      Full Logs.Tail response protobuf.
    """
    body = json.dumps({
        'project': project,
        'path': path,
    })
    return self.SendRequest('prpc/logdog.Logs', 'Tail', body, dryrun=dryrun)

  def GetAnnotations(self, master, builder, build_num, dryrun=False):
    """Retrieve Logdog annotations for a build.

    Args:
      master: waterfall master to query.
      builder: builder to query.
      build_num: build number to query.
      dryrun: Whether a dryrun.

    Returns:
      (Parsed Logdog annotations protobuf, Logdog prefix string) tuple
      or (None, None) if none is available.

    Raises:
      LogdogResponseException if response is invalid.
    """
    # Determine path to annotations.
    path = self.LogsQueryAnnotations(master, builder, build_num, dryrun=dryrun)
    if path is None:
      return None, None
    (prefix, _) = path.split('/+/')

    # Retrieve annotations datagram.
    annotations_logs = self.LogsTail(master, path)
    try:
      datagram = annotations_logs['logs'][0]['datagram']['data']
    except KeyError:
      raise LogdogResponseException('Unable to retrieve annotation datagram')

    # Parse annotations protobuf.
    annotations = annotations_pb2.Step()
    annotations.ParseFromString(base64.b64decode(datagram))
    return annotations, prefix

  def ConstructViewerURL(self, project, prefix, name):
    """Construct URL to view logs via Logdog.

    Args:
      project: Logdog project of log stream.
      prefix: Logdog prefix name.
      name: Logdog stream name.

    Returns:
      URL of logs viewer.
    """
    stream = '%s/+/%s' % (prefix, name)
    return '%(scheme)s://%(host)s/v/?s=%(query)s' % {
        'scheme': self.GetScheme(),
        'host': self.host,
        'query': urllib.quote_plus('%s/%s' % (project, stream)),
    }
