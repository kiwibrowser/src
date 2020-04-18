# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from prefetch_view import PrefetchSimulationView
import request_dependencies_lens
from request_dependencies_lens_unittest import TestRequests
import request_track
import test_utils


class PrefetchSimulationViewTestCase(unittest.TestCase):
  def setUp(self):
    super(PrefetchSimulationViewTestCase, self).setUp()
    self._SetUp()

  def testExpandRedirectChains(self):
    self.assertListEqual(
        [TestRequests.FIRST_REDIRECT_REQUEST,
         TestRequests.SECOND_REDIRECT_REQUEST, TestRequests.REDIRECTED_REQUEST],
        PrefetchSimulationView._ExpandRedirectChains(
            [TestRequests.FIRST_REDIRECT_REQUEST], self.dependencies_lens))

  def testParserDiscoverableRequests(self):
    first_request = TestRequests.FIRST_REDIRECT_REQUEST
    discovered_requests = PrefetchSimulationView.ParserDiscoverableRequests(
        first_request, self.dependencies_lens)
    self.assertListEqual(
        [TestRequests.FIRST_REDIRECT_REQUEST,
         TestRequests.JS_REQUEST, TestRequests.JS_REQUEST_OTHER_FRAME,
         TestRequests.JS_REQUEST_UNRELATED_FRAME], discovered_requests)

  def testPreloadedRequests(self):
    first_request = TestRequests.FIRST_REDIRECT_REQUEST
    preloaded_requests = PrefetchSimulationView.PreloadedRequests(
        first_request, self.dependencies_lens, self.trace)
    self.assertListEqual([first_request], preloaded_requests)
    self._SetUp(
        [{'args': {'data': {'url': 'http://bla.com/nyancat.js'}},
          'cat': 'blink.net', 'id': '0xaf9f14fa9dd6c314', 'name': 'Resource',
          'ph': 'X', 'ts': 1, 'dur': 120, 'pid': 12, 'tid': 12},
         {'args': {'step': 'Preload'}, 'cat': 'blink.net',
          'id': '0xaf9f14fa9dd6c314', 'name': 'Resource', 'ph': 'T',
          'ts': 12, 'pid': 12, 'tid': 12}])
    preloaded_requests = PrefetchSimulationView.PreloadedRequests(
        first_request, self.dependencies_lens, self.trace)
    self.assertListEqual([TestRequests.FIRST_REDIRECT_REQUEST,
         TestRequests.JS_REQUEST, TestRequests.JS_REQUEST_OTHER_FRAME,
         TestRequests.JS_REQUEST_UNRELATED_FRAME], preloaded_requests)

  def testCost(self):
    self.assertEqual(40 + 12, self.prefetch_view.Cost())

  def testUpdateNodeCosts(self):
    self.prefetch_view.UpdateNodeCosts(lambda _: 100)
    self.assertEqual(500 + 40 + 12, self.prefetch_view.Cost())

  def testUpdateNodeCostsPartial(self):
    self.prefetch_view.UpdateNodeCosts(
        lambda n: 100 if (n.request.request_id
                          == TestRequests.REDIRECTED_REQUEST.request_id) else 0)
    self.assertEqual(100 + 40 + 12, self.prefetch_view.Cost())

  def testToFromJsonDict(self):
    self.assertEqual(40 + 12, self.prefetch_view.Cost())
    json_dict = self.prefetch_view.ToJsonDict()
    new_view = PrefetchSimulationView.FromJsonDict(json_dict)
    self.assertEqual(40 + 12, new_view.Cost())
    # Updated Costs.
    self.prefetch_view.UpdateNodeCosts(lambda _: 100)
    self.assertEqual(500 + 40 + 12, self.prefetch_view.Cost())
    json_dict = self.prefetch_view.ToJsonDict()
    new_view = PrefetchSimulationView.FromJsonDict(json_dict)
    self.assertEqual(500 + 40 + 12, new_view.Cost())

  def _SetUp(self, added_trace_events=None):
    trace_events = [
        {'ts': 5, 'ph': 'X', 'dur': 10, 'pid': 2, 'tid': 1, 'cat': 'blink.net'}]
    if added_trace_events is not None:
      trace_events += added_trace_events
    self.trace = TestRequests.CreateLoadingTrace(trace_events)
    self.dependencies_lens = request_dependencies_lens.RequestDependencyLens(
        self.trace)
    self.user_satisfied_lens = test_utils.MockUserSatisfiedLens(self.trace)
    self.user_satisfied_lens._postload_msec = 12
    self.prefetch_view = PrefetchSimulationView(
        self.trace, self.dependencies_lens, self.user_satisfied_lens)
    for e in self.prefetch_view.graph.graph.Edges():
      e.cost = 10


if __name__ == '__main__':
  unittest.main()
