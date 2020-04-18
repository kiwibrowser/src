# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import devtools_monitor
from loading_trace import LoadingTrace
from request_dependencies_lens import RequestDependencyLens
from request_track import Request
import test_utils


class TestRequests(object):
  FIRST_REDIRECT_REQUEST = Request.FromJsonDict(
      {'url': 'http://bla.com', 'request_id': '1234.redirect.1',
       'initiator': {'type': 'other'},
       'timestamp': 0.5, 'timing': {}})
  SECOND_REDIRECT_REQUEST = Request.FromJsonDict(
      {'url': 'http://bla.com/redirect1', 'request_id': '1234.redirect.2',
       'initiator': {'type': 'redirect',
                     'initiating_request': '1234.redirect.1'},
       'timestamp': 1, 'timing': {}})
  REDIRECTED_REQUEST = Request.FromJsonDict({
      'url': 'http://bla.com/index.html',
      'request_id': '1234.1',
      'frame_id': '123.1',
      'initiator': {'type': 'redirect',
                    'initiating_request': '1234.redirect.2'},
      'timestamp': 2,
      'timing': {}})
  REQUEST = Request.FromJsonDict({'url': 'http://bla.com/index.html',
                                  'request_id': '1234.1',
                                  'frame_id': '123.1',
                                  'initiator': {'type': 'other'},
                                  'timestamp': 2,
                                  'timing': {}})
  JS_REQUEST = Request.FromJsonDict({'url': 'http://bla.com/nyancat.js',
                                     'request_id': '1234.12',
                                     'frame_id': '123.123',
                                     'initiator': {
                                         'type': 'parser',
                                         'url': 'http://bla.com/index.html'},
                                     'timestamp': 3,
                                     'timing': {}})
  JS_REQUEST_OTHER_FRAME = Request.FromJsonDict(
      {'url': 'http://bla.com/nyancat.js',
       'request_id': '1234.42',
       'frame_id': '123.13',
       'initiator': {'type': 'parser',
                     'url': 'http://bla.com/index.html'},
       'timestamp': 4, 'timing': {}})
  JS_REQUEST_UNRELATED_FRAME = Request.FromJsonDict(
      {'url': 'http://bla.com/nyancat.js',
       'request_id': '1234.56',
       'frame_id': '123.99',
       'initiator': {'type': 'parser',
                     'url': 'http://bla.com/index.html'},
       'timestamp': 5, 'timing': {}})
  JS_REQUEST_2 = Request.FromJsonDict(
      {'url': 'http://bla.com/cat.js', 'request_id': '1234.13',
       'frame_id': '123.123',
       'initiator': {'type': 'script',
                     'stack': {'callFrames': [
                         {'url': 'unknown'},
                         {'url': 'http://bla.com/nyancat.js'}]}},
       'timestamp': 10, 'timing': {}})
  PAGE_EVENTS = [{'method': 'Page.frameAttached',
                   'frame_id': '123.13', 'parent_frame_id': '123.1'},
                 {'method': 'Page.frameAttached',
                  'frame_id': '123.123', 'parent_frame_id': '123.1'}]

  @classmethod
  def CreateLoadingTrace(cls, trace_events=None):
    # This creates a set of requests with the following dependency structure.
    #
    # 1234.redirect.1 -> 1234.redirect.2
    # 1234.redirect.2 -> 1234.1
    # 1234.1 -> 1234.12
    # 1234.1 -> 1234.42
    # 1234.1 -> 1234.56
    # 1234.12 -> 1234.13

    trace = test_utils.LoadingTraceFromEvents(
        [cls.FIRST_REDIRECT_REQUEST, cls.SECOND_REDIRECT_REQUEST,
         cls.REDIRECTED_REQUEST, cls.REQUEST, cls.JS_REQUEST, cls.JS_REQUEST_2,
         cls.JS_REQUEST_OTHER_FRAME, cls.JS_REQUEST_UNRELATED_FRAME],
        cls.PAGE_EVENTS, trace_events)
    # Serialize and deserialize so that clients can change events without
    # affecting future tests.
    return LoadingTrace.FromJsonDict(trace.ToJsonDict())


class RequestDependencyLensTestCase(unittest.TestCase):
  def testRedirectDependency(self):
    loading_trace = test_utils.LoadingTraceFromEvents(
        [TestRequests.FIRST_REDIRECT_REQUEST,
         TestRequests.SECOND_REDIRECT_REQUEST, TestRequests.REDIRECTED_REQUEST])
    request_dependencies_lens = RequestDependencyLens(loading_trace)
    deps = request_dependencies_lens.GetRequestDependencies()
    self.assertEquals(2, len(deps))
    (first, second, reason) = deps[0]
    self.assertEquals('redirect', reason)
    self.assertEquals(TestRequests.FIRST_REDIRECT_REQUEST.request_id,
                      first.request_id)
    self.assertEquals(TestRequests.SECOND_REDIRECT_REQUEST.request_id,
                      second.request_id)
    (first, second, reason) = deps[1]
    self.assertEquals('redirect', reason)
    self.assertEquals(TestRequests.SECOND_REDIRECT_REQUEST.request_id,
                      first.request_id)
    self.assertEquals(TestRequests.REQUEST.request_id, second.request_id)

  def testGetRedirectChain(self):
    loading_trace = test_utils.LoadingTraceFromEvents(
        [TestRequests.FIRST_REDIRECT_REQUEST,
         TestRequests.SECOND_REDIRECT_REQUEST, TestRequests.REDIRECTED_REQUEST])
    request_dependencies_lens = RequestDependencyLens(loading_trace)
    whole_chain = [TestRequests.FIRST_REDIRECT_REQUEST,
                   TestRequests.SECOND_REDIRECT_REQUEST,
                   TestRequests.REDIRECTED_REQUEST]
    chain = request_dependencies_lens.GetRedirectChain(
        TestRequests.FIRST_REDIRECT_REQUEST)
    self.assertListEqual(whole_chain, chain)
    chain = request_dependencies_lens.GetRedirectChain(
        TestRequests.SECOND_REDIRECT_REQUEST)
    self.assertListEqual(whole_chain[1:], chain)
    chain = request_dependencies_lens.GetRedirectChain(
        TestRequests.REDIRECTED_REQUEST)
    self.assertEquals(whole_chain[2:], chain)

  def testScriptDependency(self):
    loading_trace = test_utils.LoadingTraceFromEvents(
        [TestRequests.JS_REQUEST, TestRequests.JS_REQUEST_2])
    request_dependencies_lens = RequestDependencyLens(loading_trace)
    deps = request_dependencies_lens.GetRequestDependencies()
    self.assertEquals(1, len(deps))
    self._AssertDependencyIs(
        deps[0],
        TestRequests.JS_REQUEST.request_id,
        TestRequests.JS_REQUEST_2.request_id, 'script')

  def testAsyncScriptDependency(self):
    JS_REQUEST_WITH_ASYNC_STACK = Request.FromJsonDict(
        {'url': 'http://bla.com/cat.js', 'request_id': '1234.14',
         'initiator': {
             'type': 'script',
             'stack': {'callFrames': [],
                       'parent': {'callFrames': [
                                      {'url': 'http://bla.com/nyancat.js'}]}}},
         'timestamp': 10, 'timing': {}})
    loading_trace = test_utils.LoadingTraceFromEvents(
        [TestRequests.JS_REQUEST, JS_REQUEST_WITH_ASYNC_STACK])
    request_dependencies_lens = RequestDependencyLens(loading_trace)
    deps = request_dependencies_lens.GetRequestDependencies()
    self.assertEquals(1, len(deps))
    self._AssertDependencyIs(
        deps[0], TestRequests.JS_REQUEST.request_id,
        JS_REQUEST_WITH_ASYNC_STACK.request_id, 'script')

  def testParserDependency(self):
    loading_trace = test_utils.LoadingTraceFromEvents(
        [TestRequests.REQUEST, TestRequests.JS_REQUEST])
    request_dependencies_lens = RequestDependencyLens(loading_trace)
    deps = request_dependencies_lens.GetRequestDependencies()
    self.assertEquals(1, len(deps))
    self._AssertDependencyIs(
        deps[0],
        TestRequests.REQUEST.request_id, TestRequests.JS_REQUEST.request_id,
        'parser')

  def testSeveralDependencies(self):
    loading_trace = test_utils.LoadingTraceFromEvents(
        [TestRequests.FIRST_REDIRECT_REQUEST,
         TestRequests.SECOND_REDIRECT_REQUEST,
         TestRequests.REDIRECTED_REQUEST,
         TestRequests.JS_REQUEST, TestRequests.JS_REQUEST_2])
    request_dependencies_lens = RequestDependencyLens(loading_trace)
    deps = request_dependencies_lens.GetRequestDependencies()
    self.assertEquals(4, len(deps))
    self._AssertDependencyIs(
        deps[0], TestRequests.FIRST_REDIRECT_REQUEST.request_id,
        TestRequests.SECOND_REDIRECT_REQUEST.request_id, 'redirect')
    self._AssertDependencyIs(
        deps[1], TestRequests.SECOND_REDIRECT_REQUEST.request_id,
        TestRequests.REQUEST.request_id, 'redirect')
    self._AssertDependencyIs(
        deps[2],
        TestRequests.REQUEST.request_id, TestRequests.JS_REQUEST.request_id,
        'parser')
    self._AssertDependencyIs(
        deps[3],
        TestRequests.JS_REQUEST.request_id,
        TestRequests.JS_REQUEST_2.request_id, 'script')

  def testDependencyDifferentFrame(self):
    """Checks that a more recent request from another frame is ignored."""
    loading_trace = test_utils.LoadingTraceFromEvents(
        [TestRequests.JS_REQUEST, TestRequests.JS_REQUEST_OTHER_FRAME,
         TestRequests.JS_REQUEST_2])
    request_dependencies_lens = RequestDependencyLens(loading_trace)
    deps = request_dependencies_lens.GetRequestDependencies()
    self.assertEquals(1, len(deps))
    self._AssertDependencyIs(
        deps[0],
        TestRequests.JS_REQUEST.request_id,
        TestRequests.JS_REQUEST_2.request_id, 'script')

  def testDependencySameParentFrame(self):
    """Checks that a more recent request from an unrelated frame is ignored
    if there is one from a related frame."""
    loading_trace = test_utils.LoadingTraceFromEvents(
        [TestRequests.JS_REQUEST_OTHER_FRAME,
         TestRequests.JS_REQUEST_UNRELATED_FRAME, TestRequests.JS_REQUEST_2],
        TestRequests.PAGE_EVENTS)
    request_dependencies_lens = RequestDependencyLens(loading_trace)
    deps = request_dependencies_lens.GetRequestDependencies()
    self.assertEquals(1, len(deps))
    self._AssertDependencyIs(
        deps[0],
        TestRequests.JS_REQUEST_OTHER_FRAME.request_id,
        TestRequests.JS_REQUEST_2.request_id, 'script')

  def _AssertDependencyIs(
      self, dep, first_request_id, second_request_id, reason):
    (first, second, dependency_reason) = dep
    self.assertEquals(reason, dependency_reason)
    self.assertEquals(first_request_id, first.request_id)
    self.assertEquals(second_request_id, second.request_id)


if __name__ == '__main__':
  unittest.main()
