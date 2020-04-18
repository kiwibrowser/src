# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import dependency_graph
import request_dependencies_lens
from request_dependencies_lens_unittest import TestRequests
import request_track
import test_utils


class RequestDependencyGraphTestCase(unittest.TestCase):
  def setUp(self):
    super(RequestDependencyGraphTestCase, self).setUp()
    self.trace = TestRequests.CreateLoadingTrace()

  def testUpdateRequestCost(self, serialize=False):
    requests = self.trace.request_track.GetEvents()
    requests[0].timing = request_track.Timing.FromDevToolsDict(
        {'requestTime': 12, 'loadingFinished': 10})
    dependencies_lens = request_dependencies_lens.RequestDependencyLens(
        self.trace)
    g = dependency_graph.RequestDependencyGraph(requests, dependencies_lens)
    if serialize:
      g = self._SerializeDeserialize(g)
    self.assertEqual(10, g.Cost())
    request_id = requests[0].request_id
    g.UpdateRequestsCost({request_id: 100})
    self.assertEqual(100, g.Cost())
    g.UpdateRequestsCost({'unrelated_id': 1000})
    self.assertEqual(100, g.Cost())

  def testCost(self, serialize=False):
    requests = self.trace.request_track.GetEvents()
    for (index, request) in enumerate(requests):
      request.timing = request_track.Timing.FromDevToolsDict(
          {'requestTime': index, 'receiveHeadersEnd': 10,
           'loadingFinished': 10})
    dependencies_lens = request_dependencies_lens.RequestDependencyLens(
        self.trace)
    g = dependency_graph.RequestDependencyGraph(requests, dependencies_lens)
    if serialize:
      g = self._SerializeDeserialize(g)
    # First redirect -> Second redirect -> Redirected Request -> Request ->
    # JS Request 2
    self.assertEqual(7010, g.Cost())
    # Not on the critical path
    g.UpdateRequestsCost({TestRequests.JS_REQUEST.request_id: 0})
    self.assertEqual(7010, g.Cost())
    g.UpdateRequestsCost({TestRequests.FIRST_REDIRECT_REQUEST.request_id: 0})
    self.assertEqual(7000, g.Cost())
    g.UpdateRequestsCost({TestRequests.SECOND_REDIRECT_REQUEST.request_id: 0})
    self.assertEqual(6990, g.Cost())

  def testHandleTimingDependencies(self, serialize=False):
    # Timing adds node 1 as a parent to 2 but not 3.
    requests = [
        test_utils.MakeRequest(0, 'null', 100, 110, 110,
                               magic_content_type=True),
        test_utils.MakeRequest(1, 0, 115, 120, 120,
                               magic_content_type=True),
        test_utils.MakeRequest(2, 0, 121, 122, 122,
                               magic_content_type=True),
        test_utils.MakeRequest(3, 0, 112, 119, 119,
                               magic_content_type=True),
        test_utils.MakeRequest(4, 2, 122, 126, 126),
        test_utils.MakeRequest(5, 2, 122, 126, 126)]

    g = self._GraphFromRequests(requests)
    if serialize:
      g = self._SerializeDeserialize(g)
    self.assertSetEqual(
        self._Successors(g, requests[0]), set([requests[1], requests[3]]))
    self.assertSetEqual(
        self._Successors(g, requests[1]), set([requests[2]]))
    self.assertSetEqual(
        self._Successors(g, requests[2]), set([requests[4], requests[5]]))
    self.assertSetEqual(self._Successors(g, requests[3]), set())
    self.assertSetEqual(self._Successors(g, requests[4]), set())
    self.assertSetEqual(self._Successors(g, requests[5]), set())

    # Change node 1 so it is a parent of 3, which becomes the parent of 2.
    requests[1] = test_utils.MakeRequest(
        1, 0, 110, 111, 111, magic_content_type=True)
    g = self._GraphFromRequests(requests)
    self.assertSetEqual(self._Successors(g, requests[0]), set([requests[1]]))
    self.assertSetEqual(self._Successors(g, requests[1]), set([requests[3]]))
    self.assertSetEqual(self._Successors(g, requests[2]),
                        set([requests[4], requests[5]]))
    self.assertSetEqual(self._Successors(g, requests[3]), set([requests[2]]))
    self.assertSetEqual(self._Successors(g, requests[4]), set())
    self.assertSetEqual(self._Successors(g, requests[5]), set())

    # Add an initiator dependence to 1 that will become the parent of 3.
    requests[1] = test_utils.MakeRequest(
        1, 0, 110, 111, 111, magic_content_type=True)
    requests.append(test_utils.MakeRequest(6, 1, 111, 112, 112))
    g = self._GraphFromRequests(requests)
    # Check it doesn't change until we change the content type of 6.
    self.assertEqual(self._Successors(g, requests[6]), set())
    requests[6] = test_utils.MakeRequest(6, 1, 111, 112, 112,
                                         magic_content_type=True)
    g = self._GraphFromRequests(requests)
    self.assertSetEqual(self._Successors(g, requests[0]), set([requests[1]]))
    self.assertSetEqual(self._Successors(g, requests[1]), set([requests[6]]))
    self.assertSetEqual(self._Successors(g, requests[2]),
                        set([requests[4], requests[5]]))
    self.assertSetEqual(self._Successors(g, requests[3]), set([requests[2]]))
    self.assertSetEqual(self._Successors(g, requests[4]), set())
    self.assertSetEqual(self._Successors(g, requests[5]), set())
    self.assertSetEqual(self._Successors(g, requests[6]), set([requests[3]]))

  def testHandleTimingDependenciesImages(self, serialize=False):
    # If we're all image types, then we shouldn't split by timing.
    requests = [test_utils.MakeRequest(0, 'null', 100, 110, 110),
                test_utils.MakeRequest(1, 0, 115, 120, 120),
                test_utils.MakeRequest(2, 0, 121, 122, 122),
                test_utils.MakeRequest(3, 0, 112, 119, 119),
                test_utils.MakeRequest(4, 2, 122, 126, 126),
                test_utils.MakeRequest(5, 2, 122, 126, 126)]
    for r in requests:
      r.response_headers['Content-Type'] = 'image/gif'
    g = self._GraphFromRequests(requests)
    if serialize:
      g = self._SerializeDeserialize(g)
    self.assertSetEqual(self._Successors(g, requests[0]),
                        set([requests[1], requests[2], requests[3]]))
    self.assertSetEqual(self._Successors(g, requests[1]), set())
    self.assertSetEqual(self._Successors(g, requests[2]),
                        set([requests[4], requests[5]]))
    self.assertSetEqual(self._Successors(g, requests[3]), set())
    self.assertSetEqual(self._Successors(g, requests[4]), set())
    self.assertSetEqual(self._Successors(g, requests[5]), set())

  def testSerializeDeserialize(self):
    # Redo the tests, with a graph that has been serialized / deserialized.
    self.testUpdateRequestCost(True)
    self.testCost(True)
    self.testHandleTimingDependencies(True)
    self.testHandleTimingDependenciesImages(True)

  @classmethod
  def _SerializeDeserialize(cls, g):
    json_dict = g.ToJsonDict()
    return dependency_graph.RequestDependencyGraph.FromJsonDict(
        json_dict, dependency_graph.RequestNode, dependency_graph.Edge)

  @classmethod
  def _GraphFromRequests(cls, requests):
    trace = test_utils.LoadingTraceFromEvents(requests)
    deps_lens = test_utils.SimpleLens(trace)
    return dependency_graph.RequestDependencyGraph(requests, deps_lens)

  @classmethod
  def _Successors(cls, g, parent_request):
    parent_node = g._nodes_by_id[parent_request.request_id]
    edges = g._deps_graph.OutEdges(parent_node)
    return set(e.to_node.request for e in edges)


if __name__ == '__main__':
  unittest.main()
