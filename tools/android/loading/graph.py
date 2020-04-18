# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Support for graphs."""

import collections

import common_util


class Node(object):
  """A node in a Graph.

  Nodes are identified within a graph using object identity.
  """
  def __init__(self):
    """Create a new node."""
    self.cost = 0

  def ToJsonDict(self):
    return common_util.SerializeAttributesToJsonDict({}, self, ['cost'])

  @classmethod
  def FromJsonDict(cls, json_dict):
    return common_util.DeserializeAttributesFromJsonDict(
        json_dict, cls(), ['cost'])


class Edge(object):
  """Represents an edge in a graph."""
  def __init__(self, from_node, to_node):
    """Creates an Edge.

    Args:
      from_node: (Node) Start node.
      to_node: (Node) End node.
    """
    self.from_node = from_node
    self.to_node = to_node
    self.cost = 0

  def ToJsonDict(self):
    return common_util.SerializeAttributesToJsonDict(
        {}, self, ['from_node', 'to_node', 'cost'])

  @classmethod
  def FromJsonDict(cls, json_dict):
    result = cls(None, None)
    return common_util.DeserializeAttributesFromJsonDict(
        json_dict, result, ['from_node', 'to_node', 'cost'])


class DirectedGraph(object):
  """Directed graph.

  A graph is identified by a list of nodes and a list of edges. It does not need
  to be acyclic, but then some methods will fail.
  """
  __GRAPH_NODE_INDEX = '__graph_node_index'
  __TO_NODE_INDEX = '__to_node_index'
  __FROM_NODE_INDEX = '__from_node_index'

  def __init__(self, nodes, edges):
    """Builds a graph from a set of node and edges.

    Note that the edges referencing a node not in the provided list are dropped.

    Args:
      nodes: ([Node]) Sequence of Nodes.
      edges: ([Edge]) Sequence of Edges.
    """
    self._nodes = set(nodes)
    self._edges = set(filter(
        lambda e: e.from_node in self._nodes and e.to_node in self._nodes,
        edges))
    assert all(isinstance(node, Node) for node in self._nodes)
    assert all(isinstance(edge, Edge) for edge in self._edges)
    self._in_edges = {n: [] for n in self._nodes}
    self._out_edges = {n: [] for n in self._nodes}
    for edge in self._edges:
      self._out_edges[edge.from_node].append(edge)
      self._in_edges[edge.to_node].append(edge)

  def OutEdges(self, node):
    """Returns a list of edges starting from a node.
    """
    return self._out_edges[node]

  def InEdges(self, node):
    """Returns a list of edges ending at a node."""
    return self._in_edges[node]

  def Nodes(self):
    """Returns the set of nodes of this graph."""
    return self._nodes

  def Edges(self):
    """Returns the set of edges of this graph."""
    return self._edges

  def RootNodes(self):
    """Returns an iterable of nodes that have no incoming edges."""
    return filter(lambda n: not self.InEdges(n), self._nodes)

  def UpdateEdge(self, edge, new_from_node, new_to_node):
    """Updates an edge.

    Args:
      edge:
      new_from_node:
      new_to_node:
    """
    assert edge in self._edges
    assert new_from_node in self._nodes
    assert new_to_node in self._nodes
    self._in_edges[edge.to_node].remove(edge)
    self._out_edges[edge.from_node].remove(edge)
    edge.from_node = new_from_node
    edge.to_node = new_to_node
    # TODO(lizeb): Check for duplicate edges?
    self._in_edges[edge.to_node].append(edge)
    self._out_edges[edge.from_node].append(edge)

  def TopologicalSort(self, roots=None):
    """Returns a list of nodes, in topological order.

      Args:
        roots: ([Node]) If set, the topological sort will only consider nodes
                        reachable from this list of sources.
    """
    sorted_nodes = []
    if roots is None:
      nodes_subset = self._nodes
    else:
      nodes_subset = self.ReachableNodes(roots)
    remaining_in_edges = {n: 0 for n in nodes_subset}
    for edge in self._edges:
      if edge.from_node in nodes_subset and edge.to_node in nodes_subset:
        remaining_in_edges[edge.to_node] += 1
    sources = [node for (node, count) in remaining_in_edges.items()
               if count == 0]
    while sources:
      node = sources.pop(0)
      sorted_nodes.append(node)
      for e in self.OutEdges(node):
        successor = e.to_node
        if successor not in nodes_subset:
          continue
        assert remaining_in_edges[successor] > 0
        remaining_in_edges[successor] -= 1
        if remaining_in_edges[successor] == 0:
          sources.append(successor)
    return sorted_nodes

  def ReachableNodes(self, roots, should_stop=lambda n: False):
    """Returns a list of nodes from a set of root nodes.

    Args:
      roots: ([Node]) List of roots to start from.
      should_stop: (callable) Returns True when a node should stop the
                   exploration and be skipped.
    """
    return self._ExploreFrom(
        roots, lambda n: (e.to_node for e in self.OutEdges(n)),
        should_stop=should_stop)

  def AncestorNodes(self, descendants):
    """Returns a set of nodes that are ancestors of a set of nodes.

    This is not quite the opposite of ReachableNodes, because (in a tree) it
    will not include |descendants|.

    Args:
      descendants: ([Node]) List of nodes to start from.

    """
    return set(self._ExploreFrom(
        descendants,
        lambda n: (e.from_node for e in self.InEdges(n)))) - set(descendants)

  def Cost(self, roots=None, path_list=None, costs_out=None):
    """Compute the cost of the graph.

    Args:
      roots: ([Node]) If set, only compute the cost of the paths reachable
             from this list of nodes.
      path_list: if not None, gets a list of nodes in the longest path.
      costs_out: if not None, gets a vector of node costs by node.

    Returns:
      Cost of the longest path.
    """
    if not self._nodes:
     return 0
    costs = {n: 0 for n in self._nodes}
    for node in self.TopologicalSort(roots):
      cost = 0
      if self.InEdges(node):
        cost = max([costs[e.from_node] + e.cost for e in self.InEdges(node)])
      costs[node] = cost + node.cost
    max_cost = max(costs.values())
    if costs_out is not None:
      del costs_out[:]
      costs_out.extend(costs)
    if path_list is not None:
      del path_list[:]
      node = (i for i in self._nodes if costs[i] == max_cost).next()
      path_list.append(node)
      while self.InEdges(node):
        predecessors = [e.from_node for e in self.InEdges(node)]
        node = reduce(
            lambda costliest_node, next_node:
            next_node if costs[next_node] > costs[costliest_node]
            else costliest_node, predecessors)
        path_list.insert(0, node)
    return max_cost

  def ToJsonDict(self):
    node_dicts = []
    node_to_index = {node: index for (index, node) in enumerate(self._nodes)}
    for (node, index) in node_to_index.items():
      node_dict = node.ToJsonDict()
      assert self.__GRAPH_NODE_INDEX not in node_dict
      node_dict.update({self.__GRAPH_NODE_INDEX: index})
      node_dicts.append(node_dict)
    edge_dicts = []
    for edge in self._edges:
      edge_dict = edge.ToJsonDict()
      assert self.__TO_NODE_INDEX not in edge_dict
      assert self.__FROM_NODE_INDEX not in edge_dict
      edge_dict.update({self.__TO_NODE_INDEX: node_to_index[edge.to_node],
                        self.__FROM_NODE_INDEX: node_to_index[edge.from_node]})
      edge_dicts.append(edge_dict)
    return {'nodes': node_dicts, 'edges': edge_dicts}

  @classmethod
  def FromJsonDict(cls, json_dict, node_class, edge_class):
    """Returns an instance from a dict.

    Note that the classes of the nodes and edges need to be specified here.
    This is done to reduce the likelihood of error.
    """
    index_to_node = {
        node_dict[cls.__GRAPH_NODE_INDEX]: node_class.FromJsonDict(node_dict)
        for node_dict in json_dict['nodes']}
    edges = []
    for edge_dict in json_dict['edges']:
      edge = edge_class.FromJsonDict(edge_dict)
      edge.from_node = index_to_node[edge_dict[cls.__FROM_NODE_INDEX]]
      edge.to_node = index_to_node[edge_dict[cls.__TO_NODE_INDEX]]
      edges.append(edge)
    result = DirectedGraph(index_to_node.values(), edges)
    return result

  def _ExploreFrom(self, initial, expand, should_stop=lambda n: False):
    """Explore from a set of nodes.

    Args:
      initial: ([Node]) List of nodes to start from.
      expand: (callable) Given a node, return an iterator of nodes to explore
        from that node.
      should_stop: (callable) Returns True when a node should stop the
                   exploration and be skipped.
    """
    visited = set()
    fifo = collections.deque([n for n in initial if not should_stop(n)])
    while fifo:
      node = fifo.pop()
      if should_stop(node):
        continue
      visited.add(node)
      for n in expand(node):
        if n not in visited and not should_stop(n):
          visited.add(n)
          fifo.appendleft(n)
    return list(visited)
