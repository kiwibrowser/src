# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A library for emitting traces and spans to Google Cloud trace."""
from __future__ import print_function

import contextlib
import errno
import functools
import json
import os
import random
import re

import google.protobuf.internal.well_known_types as types
from infra_libs import ts_mon

from chromite.lib import cros_logging as log
from chromite.lib import metrics
from chromite.lib import structured


SPANS_LOG = '/var/log/trace/{pid}-{span_id}.json'
_SPAN_COUNT_METRIC = 'chromeos/trace/client/logged_count'

#--- Code for logging spans to a file for later processing. --------------------
def GetSpanLogFilePath(span):
  """Gets the path to write a span to.

  Args:
    span: The span to write.
  """
  return SPANS_LOG.format(pid=os.getpid(), span_id=span.spanId)


def LogSpan(span):
  """Serializes and logs a Span to a file.

  Args:
    span: A Span instance to serialize.
  """
  _RecordSpanMetrics(span)
  try:
    with open(GetSpanLogFilePath(span), 'w') as fh:
      fh.write(json.dumps(span.ToDict()))
  # Catch various configuration errors
  except (OSError, IOError) as error:
    if error.errno == errno.EPERM:
      log.warning(
          'Received permissions error while trying to open the span log file.')
      return None
    elif error.errno == errno.ENOENT:
      log.warning('/var/log/traces does not exist; skipping trace log.')
      return None
    else:
      raise


def _RecordSpanMetrics(span):
  """Increments the count of spans logged.

  Args:
    span: The span to record.
  """
  m = metrics.Counter(
      _SPAN_COUNT_METRIC,
      description="A count of spans logged by a client.",
      field_spec=[ts_mon.StringField('name')])
  m.increment(fields={'name': span.name})

#-- User-facing API ------------------------------------------------------------
class Span(structured.Structured):
  """An object corresponding to a cloud trace Span."""

  VISIBLE_KEYS = (
      'name', 'spanId', 'parentSpanId', 'labels',
      'startTime', 'endTime', 'status')

  def __init__(self, name, spanId=None, labels=None, parentSpanId=None,
               traceId=None):
    """Creates a Span object.

    Args:
      name: The name of the span
      spanId: (optional) A 64-bit number as a string. If not provided, it will
          be generated randomly with .GenerateSpanId().
      labels: (optional) a dict<string, string> of key/values
      traceId: (optional) A 32 hex digit string referring to the trace
          containing this span. If not provided, a new trace will be created
          with a random id.
      parentSpanId: (optional) The spanId of the parent.
    """
    # Visible attributes
    self.name = name
    self.spanId = spanId or Span.GenerateSpanId()
    self.parentSpanId = parentSpanId
    self.labels = labels or {}
    self.startTime = None
    self.endTime = None
    # Non-visible attributes
    self.traceId = traceId or Span.GenerateTraceId()

  @staticmethod
  def GenerateSpanId():
    """Returns a random 64-bit number as a string."""
    return str(random.randint(0, 2**64))

  @staticmethod
  def GenerateTraceId():
    """Returns a random 128-bit number as a 32-byte hex string."""
    id_number = random.randint(0, 2**128)
    return '%0.32X' % id_number

  def __enter__(self):
    """Enters the span context.

    Side effect: Records the start time as a Timestamp.
    """
    start = types.Timestamp()
    start.GetCurrentTime()
    self.startTime = start.ToJsonString()
    return self

  def __exit__(self, _type, _value, _traceback):
    """Exits the span context.

    Side-effect:
      Record the end Timestamp.
    """
    end = types.Timestamp()
    end.GetCurrentTime()
    self.endTime = end.ToJsonString()


class SpanStack(object):
  """A stack of Span contexts."""

  CLOUD_TRACE_CONTEXT_ENV = 'CLOUD_TRACE_CONTEXT'
  CLOUD_TRACE_CONTEXT_PATTERN = re.compile(
      r'(?P<traceId>[0-9A-Fa-f]+)'
      r'/'
      r'(?P<parentSpanId>\d+)'
      r';o=(?P<options>\d+)'
  )

  def __init__(self, global_context=None, traceId=None, parentSpanId=None,
               labels=None, enabled=True):
    """Initializes the Span.

      global_context: (optional) A global context str, perhaps read from the
          X-Cloud-Trace-Context header.
      traceId: (optional) A 32 hex digit string referring to the trace
          containing this span. If not provided, a new trace will be created
          with a random id.
      parentSpanId: (optional) The spanId of the parent.
      labels: (optional) a dict<string, string> of key/values to attach to
          each Span created, or None.
      enabled: (optional) a bool indicating whether we should log the spans
          to a file for later uploading by the cloud trace log consumer daemon.
    """
    self.traceId = traceId
    self.spans = []
    self.last_span_id = parentSpanId
    self.labels = labels
    self.enabled = enabled

    global_context = (global_context or
                      os.environ.get(self.CLOUD_TRACE_CONTEXT_ENV, ''))
    context = SpanStack._ParseCloudTraceContext(global_context)

    if traceId is None:
      self.traceId = context.get('traceId')
    if parentSpanId is None:
      self.last_span_id = context.get('parentSpanId')
    if context.get('options') == '0':
      self.enabled = False

  def _CreateSpan(self, name, **kwargs):
    """Creates a span instance, setting certain defaults.

    Args:
      name: The name of the span
      **kwargs: The keyword arguments to configure the span with.
    """
    kwargs.setdefault('traceId', self.traceId)
    kwargs.setdefault('labels', self.labels)
    kwargs.setdefault('parentSpanId', self.last_span_id)
    span = Span(name, **kwargs)
    if self.traceId is None:
      self.traceId = span.traceId
    return span

  @contextlib.contextmanager
  def Span(self, name, **kwargs):
    """Enter a new Span context contained within the top Span of the stack.

    Args:
      name: The name of the span to enter
      **kwargs: The kwargs to construct the span with.

    Side effect:
      Appends the new span object to |spans|, and yields span while in its
      context. Pops the span object when exiting the context.

    Returns:
      A contextmanager whose __enter__() returns the new Span.
    """
    span = self._CreateSpan(name, **kwargs)
    old_span_id, self.last_span_id = self.last_span_id, span.spanId
    self.spans.append(span)

    with span:
      with self.EnvironmentContext():
        yield span

    self.spans.pop()
    self.last_span_id = old_span_id

    # Log each span to a file for later processing.
    if self.enabled:
      LogSpan(span)

  # pylint: disable=docstring-misnamed-args
  def Spanned(self, *span_args, **span_kwargs):
    """A decorator equivalent of 'with span_stack.Span(...)'

    Args:
      *span_args: *args to use with the .Span
      **span_kwargs: **kwargs to use with the .Span

    Returns:
      A decorator to wrap the body of a function in a span block.
    """
    def SpannedDecorator(f):
      """Wraps the body of |f| with a .Span block."""
      @functools.wraps(f)
      def inner(*args, **kwargs):
        with self.Span(*span_args, **span_kwargs):
          f(*args, **kwargs)
      return inner
    return SpannedDecorator

  def _GetCloudTraceContextHeader(self):
    """Gets the Cloud Trace HTTP header context.

    From the cloud trace doc explaining this (
    https://cloud.google.com/trace/docs/support?hl=bg)

      "X-Cloud-Trace-Context: TRACE_ID/SPAN_ID;o=TRACE_TRUE"
      Where:
        - TRACE_ID is a 32-character hex value representing a 128-bit number.
        It should be unique between your requests, unless you intentionally
        want to bundle the requests together. You can use UUIDs.
        - SPAN_ID should be 0 for the first span in your trace. For
        subsequent requests, set SPAN_ID to the span ID of the parent
        request. See the description of TraceSpan (REST, RPC) for more
        information about nested traces.
        - TRACE_TRUE must be 1 to trace this request. Specify 0 to not trace
        the request. For example, to force a trace with cURL:
          curl "http://www.example.com" --header "X-Cloud-Trace-Context:
            105445aa7843bc8bf206b120001000/0;o=1"
    """
    if not self.traceId:
      return ''
    span_postfix = '/%s' % self.spans[-1].spanId if self.spans else ''
    enabled = '1' if self.enabled else '0'
    return "{trace_id}{span_postfix};o={enabled}".format(
        trace_id=self.traceId,
        span_postfix=span_postfix,
        enabled=enabled)

  @contextlib.contextmanager
  def EnvironmentContext(self):
    """Sets CLOUD_TRACE_CONTEXT to the value of X-Cloud-Trace-Context.

    Cloud Trace uses an HTTP header to propagate trace context across RPC
    boundaries. This method does the same across process boundaries using an
    environment variable.
    """
    old_value = os.environ.get(self.CLOUD_TRACE_CONTEXT_ENV)
    try:
      os.environ[self.CLOUD_TRACE_CONTEXT_ENV] = (
          self._GetCloudTraceContextHeader())
      yield
    finally:
      if old_value is not None:
        os.environ[self.CLOUD_TRACE_CONTEXT_ENV] = old_value
      elif self.CLOUD_TRACE_CONTEXT_ENV in os.environ:
        del os.environ[self.CLOUD_TRACE_CONTEXT_ENV]

  @staticmethod
  def _ParseCloudTraceContext(context):
    """Sets current_span_id and trace_id from the |context|.

    See _GetCloudTraceContextHeader.

    Args:
      context: The context variable, either from X-Cloud-Trace-Context
          or from the CLOUD_TRACE_CONTEXT environment variable.

    Returns:
      A dictionary, which if the context string matches
      CLOUD_TRACE_CONTEXT_PATTERN, contains the matched groups. If not matched,
      returns an empty dictionary.
    """
    m = SpanStack.CLOUD_TRACE_CONTEXT_PATTERN.match(context)
    if m:
      return m.groupdict()
    else:
      return {}
