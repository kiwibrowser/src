# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import resource_sack
from test_utils import (MakeRequest,
                        TestDependencyGraph)


class ResourceSackTestCase(unittest.TestCase):
  def SimpleGraph(self, node_names):
    """Create a simple graph from a list of nodes."""
    requests = [MakeRequest(node_names[0], 'null')]
    for n in node_names[1:]:
      requests.append(MakeRequest(n, node_names[0]))
    return TestDependencyGraph(requests)

  def test_NodeMerge(self):
    g1 = TestDependencyGraph([
        MakeRequest(0, 'null'),
        MakeRequest(1, 0),
        MakeRequest(2, 0),
        MakeRequest(3, 1)])
    g2 = TestDependencyGraph([
        MakeRequest(0, 'null'),
        MakeRequest(1, 0),
        MakeRequest(2, 0),
        MakeRequest(4, 2)])
    sack = resource_sack.GraphSack()
    sack.ConsumeGraph(g1)
    sack.ConsumeGraph(g2)
    self.assertEqual(5, len(sack.bags))
    for bag in sack.bags:
      if bag.label not in ('3/', '4/'):
        self.assertEqual(2, bag.num_nodes)
      else:
        self.assertEqual(1, bag.num_nodes)

  def test_MultiParents(self):
    g1 = TestDependencyGraph([
        MakeRequest(0, 'null'),
        MakeRequest(2, 0)])
    g2 = TestDependencyGraph([
        MakeRequest(1, 'null'),
        MakeRequest(2, 1)])
    sack = resource_sack.GraphSack()
    sack.ConsumeGraph(g1)
    sack.ConsumeGraph(g2)
    self.assertEqual(3, len(sack.bags))
    labels = {bag.label: bag for bag in sack.bags}
    def Predecessors(label):
      bag = labels['%s/' % label]
      return [e.from_node
              for e in bag._sack._graph.InEdges(bag)]
    self.assertEqual(
        set(['0/', '1/']),
        set([bag.label for bag in Predecessors(2)]))
    self.assertFalse(Predecessors(0))
    self.assertFalse(Predecessors(1))

  def test_Shortname(self):
    root = MakeRequest(0, 'null')
    shortname = MakeRequest(1, 0)
    shortname.url = 'data:fake/content;' + 'lotsand' * 50 + 'lotsofdata'
    g1 = TestDependencyGraph([root, shortname])
    sack = resource_sack.GraphSack()
    sack.ConsumeGraph(g1)
    self.assertEqual(set(['0/', 'data:fake/content']),
                     set([bag.label for bag in sack.bags]))

  def test_Core(self):
    # We will use a core threshold of 0.5 to make it easier to define
    # graphs. Resources 0 and 1 are core and others are not. We check full names
    # and node counts as we output that for core set analysis. In subsequent
    # tests we just check labels to make the tests easier to read.
    graphs = [self.SimpleGraph([0, 1, 2]),
              self.SimpleGraph([0, 1, 3]),
              self.SimpleGraph([0, 1, 4]),
              self.SimpleGraph([0, 5])]
    sack = resource_sack.GraphSack()
    sack.CORE_THRESHOLD = 0.5
    for g in graphs:
      sack.ConsumeGraph(g)
    self.assertEqual(set([('http://0', 4), ('http://1', 3)]),
                     set((b.name, b.num_nodes) for b in sack.CoreSet()))

  def test_IntersectingCore(self):
    # Graph set A has core set {0, 1} and B {0, 2} so the final core set should
    # be {0}. Set C makes sure we restrict core computation to tags A and B.
    set_A = [self.SimpleGraph([0, 1, 2]),
             self.SimpleGraph([0, 1, 3])]
    set_B = [self.SimpleGraph([0, 2, 3]),
             self.SimpleGraph([0, 2, 1])]
    set_C = [self.SimpleGraph([2 * i + 4, 2 * i + 5]) for i in xrange(5)]
    sack = resource_sack.GraphSack()
    sack.CORE_THRESHOLD = 0.5
    for g in set_A + set_B + set_C:
      sack.ConsumeGraph(g)
    self.assertEqual(set(), sack.CoreSet())
    self.assertEqual(set(['0/', '1/']),
                     set(b.label for b in sack.CoreSet(set_A)))
    self.assertEqual(set(['0/', '2/']),
                     set(b.label for b in sack.CoreSet(set_B)))
    self.assertEqual(set(), sack.CoreSet(set_C))
    self.assertEqual(set(['0/']),
                     set(b.label for b in sack.CoreSet(set_A, set_B)))
    self.assertEqual(set(), sack.CoreSet(set_A, set_B, set_C))

  def test_Simililarity(self):
    self.assertAlmostEqual(
        0.5,
        resource_sack.GraphSack.CoreSimilarity(
            set([1, 2, 3]), set([1, 3, 4])))
    self.assertEqual(
        0, resource_sack.GraphSack.CoreSimilarity(set(), set()))


if __name__ == '__main__':
  unittest.main()
