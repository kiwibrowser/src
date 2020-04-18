# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Models the effect of prefetching resources from a loading trace.

For example, this can be used to evaluate NoState Prefetch
(https://goo.gl/B3nRUR).

When executed as a script, takes a trace as a command-line arguments and shows
statistics about it.
"""

import itertools
import operator

import common_util
import dependency_graph
import graph
import loading_trace
import user_satisfied_lens
import request_dependencies_lens
import request_track


class RequestNode(dependency_graph.RequestNode):
  """Simulates the effect of prefetching resources discoverable by the preload
  scanner.
  """
  _ATTRS = ['preloaded', 'before']
  def __init__(self, request=None):
    super(RequestNode, self).__init__(request)
    self.preloaded = False
    self.before = False

  def ToJsonDict(self):
    result = super(RequestNode, self).ToJsonDict()
    return common_util.SerializeAttributesToJsonDict(result, self, self._ATTRS)

  @classmethod
  def FromJsonDict(cls, json_dict):
    result = super(RequestNode, cls).FromJsonDict(json_dict)
    return common_util.DeserializeAttributesFromJsonDict(
        json_dict, result, cls._ATTRS)


class PrefetchSimulationView(object):
  """Simulates the effect of prefetch."""
  def __init__(self, trace, dependencies_lens, user_lens):
    self.postload_msec = None
    self.graph = None
    if trace is None:
      return
    requests = trace.request_track.GetEvents()
    critical_requests_ids = user_lens.CriticalRequestIds()
    self.postload_msec = user_lens.PostloadTimeMsec()
    self.graph = dependency_graph.RequestDependencyGraph(
        requests, dependencies_lens, node_class=RequestNode)
    preloaded_requests = [r.request_id for r in self.PreloadedRequests(
        requests[0], dependencies_lens, trace)]
    self._AnnotateNodes(self.graph.graph.Nodes(), preloaded_requests,
                        critical_requests_ids)

  def Cost(self):
    """Returns the cost of the graph, restricted to the critical requests."""
    pruned_graph = self._PrunedGraph()
    return pruned_graph.Cost() + self.postload_msec

  def UpdateNodeCosts(self, node_to_cost):
    """Updates the cost of nodes, according to |node_to_cost|.

    Args:
      node_to_cost: (Callable) RequestNode -> float. Callable returning the cost
                    of a node.
    """
    pruned_graph = self._PrunedGraph()
    for node in pruned_graph.Nodes():
      node.cost = node_to_cost(node)

  def ToJsonDict(self):
    """Returns a dict representing this instance."""
    result = {'graph': self.graph.ToJsonDict()}
    return common_util.SerializeAttributesToJsonDict(
        result, self, ['postload_msec'])

  @classmethod
  def FromJsonDict(cls, json_dict):
    """Returns an instance of PrefetchSimulationView from a dict dumped by
    ToJSonDict().
    """
    result = cls(None, None, None)
    result.graph = dependency_graph.RequestDependencyGraph.FromJsonDict(
        json_dict['graph'], RequestNode, dependency_graph.Edge)
    return common_util.DeserializeAttributesFromJsonDict(
        json_dict, result, ['postload_msec'])

  @classmethod
  def _AnnotateNodes(cls, nodes, preloaded_requests_ids,
                     critical_requests_ids,):
    for node in nodes:
      node.preloaded = node.request.request_id in preloaded_requests_ids
      node.before = node.request.request_id in critical_requests_ids

  @classmethod
  def ParserDiscoverableRequests(
      cls, request, dependencies_lens, recurse=False):
    """Returns a list of requests IDs dicovered by the parser.

    Args:
      request: (Request) Root request.

    Returns:
      [Request]
    """
    # TODO(lizeb): handle the recursive case.
    assert not recurse
    discoverable_requests = [request]
    first_request = dependencies_lens.GetRedirectChain(request)[-1]
    deps = dependencies_lens.GetRequestDependencies()
    for (first, second, reason) in deps:
      if first.request_id == first_request.request_id and reason == 'parser':
        discoverable_requests.append(second)
    return discoverable_requests

  @classmethod
  def _ExpandRedirectChains(cls, requests, dependencies_lens):
    return list(itertools.chain.from_iterable(
        [dependencies_lens.GetRedirectChain(r) for r in requests]))

  @classmethod
  def PreloadedRequests(cls, request, dependencies_lens, trace):
    """Returns the requests that have been preloaded from a given request.

    This list is the set of request that are:
    - Discoverable by the parser
    - Found in the trace log.

    Before looking for dependencies, this follows the redirect chain.

    Args:
      request: (Request) Root request.

    Returns:
      A list of Request. Does not include the root request. This list is a
      subset of the one returned by ParserDiscoverableRequests().
    """
    # Preload step events are emitted in ResourceFetcher::preloadStarted().
    resource_events = trace.tracing_track.Filter(
        categories=set([u'blink.net']))
    preload_step_events = filter(
        lambda e:  e.args.get('step') == 'Preload',
        resource_events.GetEvents())
    preloaded_urls = set()
    for preload_step_event in preload_step_events:
      preload_event = resource_events.EventFromStep(preload_step_event)
      if preload_event:
        preloaded_urls.add(preload_event.args['data']['url'])
    parser_requests = cls.ParserDiscoverableRequests(
        request, dependencies_lens)
    preloaded_root_requests = filter(
        lambda r: r.url in preloaded_urls, parser_requests)
    # We can actually fetch the whole redirect chain.
    return [request] + list(itertools.chain.from_iterable(
        [dependencies_lens.GetRedirectChain(r)
         for r in preloaded_root_requests]))

  def _PrunedGraph(self):
    roots = self.graph.graph.RootNodes()
    nodes = self.graph.graph.ReachableNodes(
        roots, should_stop=lambda n: not n.before)
    return graph.DirectedGraph(nodes, self.graph.graph.Edges())


def _PrintSumamry(trace, dependencies_lens, user_lens):
  prefetch_view = PrefetchSimulationView(trace, dependencies_lens, user_lens)
  print 'Time to First Contentful Paint = %.02fms' % prefetch_view.Cost()
  print 'Set costs of prefetched requests to 0.'
  prefetch_view.UpdateNodeCosts(lambda n: 0 if n.preloaded else n.cost)
  print 'Time to First Contentful Paint = %.02fms' % prefetch_view.Cost()


def main(filename):
  trace = loading_trace.LoadingTrace.FromJsonFile(filename)
  dependencies_lens = request_dependencies_lens.RequestDependencyLens(trace)
  user_lens = user_satisfied_lens.FirstContentfulPaintLens(trace)
  _PrintSumamry(trace, dependencies_lens, user_lens)


if __name__ == '__main__':
  import sys
  main(sys.argv[1])
