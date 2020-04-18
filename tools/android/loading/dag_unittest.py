# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys
import unittest

import dag

class DagTestCase(unittest.TestCase):

  def MakeDag(self, links):
    """Make a graph from a description of links.

    Args:
      links: A list of (index, (successor index...)) tuples. Index must equal
        the location of the tuple in the list and are provided to make it easier
        to read.

    Returns:
      A list of Nodes.
    """
    nodes = []
    for i in xrange(len(links)):
      assert i == links[i][0]
      nodes.append(dag.Node(i))
    for l in links:
      for s in l[1]:
        nodes[l[0]].AddSuccessor(nodes[s])
    return nodes

  def SortedIndicies(self, graph, node_filter=None):
    return [n.Index() for n in dag.TopologicalSort(graph, node_filter)]

  def SuccessorIndicies(self, node):
    return [c.Index() for c in node.SortedSuccessors()]

  def test_SimpleSorting(self):
    graph = self.MakeDag([(0, (1,2)),
                          (1, (3,)),
                          (2, ()),
                          (3, (4,)),
                          (4, ()),
                          (5, (6,)),
                          (6, ())])
    self.assertEqual(self.SuccessorIndicies(graph[0]), [1, 2])
    self.assertEqual(self.SuccessorIndicies(graph[1]), [3])
    self.assertEqual(self.SuccessorIndicies(graph[2]), [])
    self.assertEqual(self.SuccessorIndicies(graph[3]), [4])
    self.assertEqual(self.SuccessorIndicies(graph[4]), [])
    self.assertEqual(self.SuccessorIndicies(graph[5]), [6])
    self.assertEqual(self.SuccessorIndicies(graph[6]), [])
    self.assertEqual(self.SortedIndicies(graph), [0, 5, 1, 2, 6, 3, 4])

  def test_SortSiblingsAreGrouped(self):
    graph = self.MakeDag([(0, (1, 2, 3)),
                          (1, (4,)),
                          (2, (5, 6)),
                          (3, (7, 8)),
                          (4, ()),
                          (5, ()),
                          (6, ()),
                          (7, ()),
                          (8, ())])
    self.assertEqual(self.SortedIndicies(graph), [0, 1, 2, 3, 4, 5, 6, 7, 8])

  def test_FilteredSorting(self):
    # 0 is a filtered-out root, which means the subgraph containing 1, 2, 3 and
    # 4 should be ignored. 5 is an unfiltered root, and the subgraph containing
    # 6, 7, 8 and 10 should be sorted. 9 and 11 are filtered out, and should
    # exclude the unfiltred node 12.
    graph = self.MakeDag([(0, (1,)),
                          (1, (2, 3)),
                          (2, ()),
                          (3, (4,)),
                          (4, ()),
                          (5, (6, 7)),
                          (6, (11,)),
                          (7, (8,)),
                          (8, (9, 10)),
                          (9, ()),
                          (10, ()),
                          (11, (12,)),
                          (12, ())])
    self.assertEqual(self.SortedIndicies(
        graph, lambda n: n.Index() not in (0, 3, 9, 11)),
                     [5, 6, 7, 8, 10])


if __name__ == '__main__':
  unittest.main()
