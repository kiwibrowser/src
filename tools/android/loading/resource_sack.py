# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A collection of ResourceGraphs.

Processes multiple ResourceGraphs, all presumably from requests to the same
site. Common urls are collected in Bags and different statistics on the
relationship between bags are collected.
"""

import collections
import json
import sys
import urlparse

from collections import defaultdict

import content_classification_lens
import graph
import user_satisfied_lens

class GraphSack(object):
  """Aggreate of RequestDependencyGraphs.

  Collects RequestDependencyGraph nodes into bags, where each bag contains the
  nodes with common urls. Dependency edges are tracked between bags (so that
  each bag may be considered as a node of a graph). This graph of bags is
  referred to as a sack.

  Each bag is associated with a dag.Node, even though the bag graph may not be a
  DAG. The edges are annotated with list of graphs and nodes that generated
  them.
  """
  # See CoreSet().
  CORE_THRESHOLD = 0.8

  _GraphInfo = collections.namedtuple('_GraphInfo', (
      'cost',   # The graph cost (aka critical path length).
      ))

  def __init__(self):
    # Each bag in our sack is named as indicated by this map.
    self._name_to_bag = {}
    # List our edges by bag pairs: (from_bag, to_bag) -> graph.Edge.
    self._edges = {}
    # Maps graph -> _GraphInfo structures for each graph we've consumed.
    self._graph_info = {}

    # How we generate names.
    self._name_generator = lambda n: n.request.url

    # Our graph, updated after each ConsumeGraph.
    self._graph = None

  def SetNameGenerator(self, generator):
    """Set the generator we use for names.

    This will define the equivalence class of requests we use to define sacks.

    Args:
      generator: a function taking a RequestDependencyGraph node and returning a
        string.
    """
    self._name_generator = generator

  def ConsumeGraph(self, request_graph):
    """Add a graph and process.

    Args:
      graph: (RequestDependencyGraph) the graph to add.
    """
    assert graph not in self._graph_info
    cost = request_graph.Cost()
    self._graph_info[request_graph] = self._GraphInfo(cost=cost)
    for n in request_graph.graph.Nodes():
      self.AddNode(request_graph, n)

    # TODO(mattcary): this is inefficient but our current API doesn't require an
    # explicit graph creation from the client.
    self._graph = graph.DirectedGraph(self.bags, self._edges.itervalues())

  def GetBag(self, node):
    """Find the bag for a node, or None if not found."""
    return self._name_to_bag.get(self._name_generator(node), None)

  def AddNode(self, request_graph, node):
    """Add a node to our collection.

    Args:
      graph: (RequestDependencyGraph) the graph in which the node lives.
      node: (RequestDependencyGraph node) the node to add.

    Returns:
      The Bag containing the node.
    """
    sack_name = self._name_generator(node)
    if sack_name not in self._name_to_bag:
      self._name_to_bag[sack_name] = Bag(self, sack_name)
    bag = self._name_to_bag[sack_name]
    bag.AddNode(request_graph, node)
    return bag

  def AddEdge(self, from_bag, to_bag):
    """Add an edge between two bags."""
    if (from_bag, to_bag) not in self._edges:
      self._edges[(from_bag, to_bag)] = graph.Edge(from_bag, to_bag)

  def CoreSet(self, *graph_sets):
    """Compute the core set of this sack.

    The core set of a sack is the set of resource that are common to most of the
    graphs in the sack. A core set of a set of graphs are the resources that
    appear with frequency at least CORE_THRESHOLD. For a collection of graph
    sets, for instance pulling the same page under different network
    connections, we intersect the core sets to produce a page core set that
    describes the key resources used by the page. See https://goo.gl/LmqQRS for
    context and discussion.

    Args:
      graph_sets: one or more collection of graphs to compute core sets. If one
        graph set is given, its core set is computed. If more than one set is
        given, the page core set of all sets is computed (the intersection of
        core sets). If no graph set is given, the core of all graphs is
        computed.

    Returns:
      A set of bags in the core set.
    """
    if not graph_sets:
      graph_sets = [self._graph_info.keys()]
    return reduce(lambda a, b: a & b,
                  (self._SingleCore(s) for s in graph_sets))

  @classmethod
  def CoreSimilarity(cls, a, b):
    """Compute the similarity of two core sets.

    We use the Jaccard index. See https://goo.gl/LmqQRS for discussion.

    Args:
      a: The first core set, as a set of strings.
      b: The second core set, as a set of strings.

    Returns:
      A similarity score between zero and one. If both sets are empty the
      similarity is zero.
    """
    if not a and not b:
      return 0
    return float(len(a & b)) / len(a | b)

  @property
  def num_graphs(self):
    return len(self.graph_info)

  @property
  def graph_info(self):
    return self._graph_info

  @property
  def bags(self):
    return self._name_to_bag.values()

  def _SingleCore(self, graph_set):
    core = set()
    graph_set = set(graph_set)
    num_graphs = len(graph_set)
    for b in self.bags:
      count = sum([g in graph_set for g in b.graphs])
      if float(count) / num_graphs > self.CORE_THRESHOLD:
        core.add(b)
    return core

  @classmethod
  def _MakeShortname(cls, url):
    # TODO(lizeb): Move this method to a convenient common location.
    parsed = urlparse.urlparse(url)
    if parsed.scheme == 'data':
      if ';' in parsed.path:
        kind, _ = parsed.path.split(';', 1)
      else:
        kind, _ = parsed.path.split(',', 1)
      return 'data:' + kind
    path = parsed.path[:10]
    hostname = parsed.hostname if parsed.hostname else '?.?.?'
    return hostname + '/' + path


class Bag(graph.Node):
  def __init__(self, sack, name):
    super(Bag, self).__init__()
    self._sack = sack
    self._name = name
    self._label = GraphSack._MakeShortname(name)
    # Maps a ResourceGraph to its Nodes contained in this Bag.
    self._graphs = defaultdict(set)

  @property
  def name(self):
    return self._name

  @property
  def label(self):
    return self._label

  @property
  def graphs(self):
    return self._graphs.iterkeys()

  @property
  def num_nodes(self):
    return sum(len(g) for g in self._graphs.itervalues())

  def GraphNodes(self, g):
    return self._graphs.get(g, set())

  def AddNode(self, request_graph, node):
    if node in self._graphs[request_graph]:
      return  # Already added.
    self._graphs[request_graph].add(node)
    for edge in request_graph.graph.OutEdges(node):
      out_bag = self._sack.AddNode(request_graph, edge.to_node)
      self._sack.AddEdge(self, out_bag)
