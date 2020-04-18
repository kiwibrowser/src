# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Support for directed acyclic graphs.

Used in the ResourceGraph model for chrome loading.
"""

class Node(object):
  """A node in a DAG.

  We do not enforce at a node level that a graph is a DAG. Methods like
  TopologicalSort will assume a DAG and may fail if that's not the case.

  Nodes are identified with an index that must be unique for a particular graph
  (it is used for hashing and equality). A graph is represented as a list of
  nodes, for example in the TopologicalSort class method. By convention a node's
  index is its position in this list, making it easy to store auxillary
  information.
  """
  def __init__(self, index):
    """Create a new node.

    Args:
      index: index of the node. We assume these indicies uniquely identify a
        node (and so use it for hashing and equality).
    """
    self._predecessors = set()
    self._successors = set()
    self._index = index

  def Predecessors(self):
    return self._predecessors

  def Successors(self):
    return self._successors

  def AddSuccessor(self, s):
    """Add a successor.

    Updates appropriate links. Any existing parents of s are unchanged; to move
    a node you must do a combination of RemoveSuccessor and AddSuccessor.

    Args:
      s: the node to add as a successor.
    """
    self._successors.add(s)
    s._predecessors.add(self)

  def RemoveSuccessor(self, s):
    """Removes a successor.

    Updates appropriate links.

    Args:
      s: the node to remove as a successor. Will raise a set exception if s is
         not an existing successor.
    """
    self._successors.remove(s)
    s._predecessors.remove(self)

  def SortedSuccessors(self):
    children = [c for c in self.Successors()]
    children.sort(key=lambda c: c.Index())
    return children

  def Index(self):
    return self._index

  def __eq__(self, o):
    return self.Index() == o.Index()

  def __hash__(self):
    return hash(self.Index())


def TopologicalSort(nodes, node_filter=None):
  """Topological sort.

  We use a BFS-like walk which ensures that sibling are always grouped
  together in the output. This is more convenient for some later analyses.

  Args:
    nodes: [Node, ...] Nodes to sort.
    node_filter: a filter Node->boolean to restrict the graph. A node passes the
      filter on a return value of True. Only the subgraph reachable from a root
      passing the filter is considered.

  Returns:
    A list of Nodes in topological order. Note that node indicies are
    unchanged; the original list nodes is not modified.
  """
  if node_filter is None:
    node_filter = lambda _: True
  sorted_nodes = []
  sources = []
  remaining_in_edges = {}
  for n in nodes:
    if n.Predecessors():
      remaining_in_edges[n] = len(n.Predecessors())
    elif node_filter(n):
      sources.append(n)
  while sources:
    n = sources.pop(0)
    assert node_filter(n)
    sorted_nodes.append(n)
    # We sort by index to get consistent sorts across runs/machines.
    for c in n.SortedSuccessors():
      assert remaining_in_edges[c] > 0
      if not node_filter(c):
        continue
      remaining_in_edges[c] -= 1
      if not remaining_in_edges[c]:
        sources.append(c)
  return sorted_nodes
