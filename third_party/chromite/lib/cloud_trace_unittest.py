# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for chromite.lib.cloud_trace."""
from __future__ import print_function

import os

from chromite.lib import cloud_trace
from chromite.lib import cros_test_lib


class SpanTest(cros_test_lib.MockTestCase):
  """Tests for the Span class."""

  def testToDict(self):
    """Tests that Span instances can be turned into dicts."""
    traceId = 'deadbeefdeadbeefdeadbeefdeadbeef'
    span = cloud_trace.Span(spanId='1234', traceId=traceId, name='Test span')
    self.assertEqual(span.ToDict(), {
        'labels': {},
        'spanId': '1234',
        'name': 'Test span'
    })


class SpanStackTest(cros_test_lib.MockTestCase):
  """Tests for the SpanStack class."""

  def setUp(self):
    self.log_span_mock = self.PatchObject(cloud_trace, 'LogSpan')

  def testLogSingleSpan(self):
    """Tests that SpanStack.Span logs a span and sends it."""
    stack = cloud_trace.SpanStack()
    context = stack.Span('foo')
    self.assertEqual(0, self.log_span_mock.call_count)
    with context:
      self.assertEqual(0, self.log_span_mock.call_count)
    self.assertEqual(1, self.log_span_mock.call_count)

  def testCallLogSpanAtCloseOfStack(self):
    """Test that LogSpans is called after each span is popped."""
    stack = cloud_trace.SpanStack()
    with stack.Span('foo'):
      self.assertEqual(0, self.log_span_mock.call_count)
      with stack.Span('bar'):
        self.assertEqual(0, self.log_span_mock.call_count)
        with stack.Span('zap'):
          self.assertEqual(0, self.log_span_mock.call_count)
        self.assertEqual(1, self.log_span_mock.call_count)
      self.assertEqual(2, self.log_span_mock.call_count)
    self.assertEqual(3, self.log_span_mock.call_count)

  def testSpannedDecorator(self):
    """Tests that @stack.Spanned() works."""
    stack = cloud_trace.SpanStack()
    @stack.Spanned('foo')
    def decorated():
      pass

    self.assertEqual(0, self.log_span_mock.call_count)
    decorated()
    self.assertEqual(1, self.log_span_mock.call_count)


class SpanLogTest(cros_test_lib.MockTestCase, cros_test_lib.TempDirTestCase):
  """Tests that spans can be logged correctly."""

  def setUp(self):
    self._old_log = cloud_trace.SPANS_LOG
    cloud_trace.SPANS_LOG = os.path.join(self.tempdir, '{pid}.json')

  def tearDown(self):
    cloud_trace.SPANS_LOG = self._old_log

  def testCanSendLog(self):
    """Tests that Spans are sent to a log."""
    stack = cloud_trace.SpanStack()
    with stack.Span('foo'):
      pass
    self.assertExists(cloud_trace.SPANS_LOG.format(pid=os.getpid()))


class CloudTraceContextTest(cros_test_lib.MockTestCase):
  """Tests the CLOUD_TRACE_CONTEXT environment variable functionality.."""
  def setUp(self):
    self.log_span_mock = self.PatchObject(cloud_trace, 'LogSpan')
    os_mock = self.PatchObject(cloud_trace, 'os')
    os_mock.environ = self.env = {}

  def testHeaderValue(self):
    """Tests that the header value makes sense."""
    trace_id = 'deadbeef12345678deadbeef12345678',
    stack = cloud_trace.SpanStack(
        traceId=trace_id,
        parentSpanId='0',
        global_context='deadbeefdeadbeefdeadbeefdeadbeef/0;o=1')

    self.assertTrue(stack.enabled)
    self.assertEqual(stack.traceId, trace_id)
    self.assertEqual(stack.last_span_id, '0')

  def testCloudTraceContextPattern(self):
    """Tests that the regex matches an example context."""
    trace_id = 'deadbeef12345678deadbeef12345678'
    global_context = '{trace_id}/0;o=1'.format(trace_id=trace_id)
    self.assertTrue(bool(
        cloud_trace.SpanStack.CLOUD_TRACE_CONTEXT_PATTERN.match(
            global_context)))

    # pylint: disable=protected-access
    result = cloud_trace.SpanStack._ParseCloudTraceContext(global_context)
    self.assertEqual(result, {
        'traceId': trace_id,
        'parentSpanId': '0',
        'options': '1'
    })

  def testInitReadsEnvironment(self):
    """Tests that SpanStack reads the enivornment on init."""
    trace_id = 'deadbeef12345678deadbeef12345678'
    global_context = '{trace_id}/0;o=1'.format(trace_id=trace_id)
    old_env = self.env.get(cloud_trace.SpanStack.CLOUD_TRACE_CONTEXT_ENV)
    try:
      self.env[cloud_trace.SpanStack.CLOUD_TRACE_CONTEXT_ENV] = global_context
      stack = cloud_trace.SpanStack()
      self.assertEqual(stack.traceId, trace_id)
      self.assertEqual(stack.last_span_id, '0')
    finally:
      if old_env is not None:
        self.env[cloud_trace.SpanStack.CLOUD_TRACE_CONTEXT_ENV] = old_env

  def testEnvironmentContextManager(self):
    """Tests that the environment context manager works."""
    trace_id = 'deadbeef12345678deadbeef12345678'
    stack = cloud_trace.SpanStack(
        global_context='{trace_id}/0;o=1'.format(trace_id=trace_id))

    old_env = self.env.get(cloud_trace.SpanStack.CLOUD_TRACE_CONTEXT_ENV)
    with stack.EnvironmentContext():
      new_env = self.env.get(cloud_trace.SpanStack.CLOUD_TRACE_CONTEXT_ENV)
    after_env = self.env.get(cloud_trace.SpanStack.CLOUD_TRACE_CONTEXT_ENV)

    self.assertEqual(old_env, after_env)
    # Note the lack of a /0; the /0 is optional.
    self.assertEqual(new_env, "deadbeef12345678deadbeef12345678;o=1")

  def testSpanContextEnabled(self):
    """Tests that the span context manager updates the environment."""
    trace_id = 'deadbeef12345678deadbeef12345678'
    stack = cloud_trace.SpanStack(
        traceId=trace_id,
        parentSpanId='0',
        global_context='deadbeefdeadbeefdeadbeefdeadbeef/0;o=1')

    self.assertTrue(stack.enabled)
    old_env = self.env.get(cloud_trace.SpanStack.CLOUD_TRACE_CONTEXT_ENV)
    with stack.Span('foo') as span:
      new_env = self.env.get(cloud_trace.SpanStack.CLOUD_TRACE_CONTEXT_ENV)
      self.assertTrue(new_env.startswith("deadbeef12345678deadbeef12345678/"))
      self.assertEqual(span.parentSpanId, '0')
      self.assertEqual(span.traceId, trace_id)
    after_env = self.env.get(cloud_trace.SpanStack.CLOUD_TRACE_CONTEXT_ENV)

    self.assertEqual(self.log_span_mock.call_count, 1)
    self.assertEqual(old_env, after_env)

  def testSpanContextDisabled(self):
    """Tests that o=1 in the global_context disables the spanstack."""
    trace_id = 'deadbeef12345678deadbeef12345678'
    stack = cloud_trace.SpanStack(
        traceId=trace_id,
        parentSpanId='0',
        global_context='deadbeefdeadbeefdeadbeefdeadbeef/0;o=0')

    self.assertFalse(stack.enabled)
    with stack.Span('foo') as span:
      self.assertEqual(span.parentSpanId, '0')
      self.assertEqual(span.traceId, trace_id)
    self.assertEqual(self.log_span_mock.call_count, 0)
