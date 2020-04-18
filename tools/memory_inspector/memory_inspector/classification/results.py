# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module owns the logic for classifying and aggregating data in buckets.

This complements the structure defined in the rules module. Symmetrically, the
aggregated results are organized in a bucket tree, which structure is identical
to the one of the corresponding rule tree.
The logic for aggregation is the following:
- The client loads a "rule tree" defined by the end-user (e.g., in a file) which
  defines the final "shape" of the results.
- The rules define how to "match" a trace_record (e.g., a mmap line or a native
  allocation) given some of its properties (e.g. the mapped file or the prot.
  flags).
- The concrete classifier (which will use this module) knows how to count the
  values for each trace_record (e.g. [Dirty KB, Clean KB, RSS KB] for mmaps).
  Hence it decides the cardinality of the result nodes.
- The responsibility of this module is simply doing the math.

In the very essence this module adds up the counters of each node whereas the
trace_record being pushed in the tree (through the AddToMatchingNodes method)
matches a rule.
It just does this math in a hierarchical fashion following the shape the tree.

A typical result tree looks like this (each node has two values in the example):
                          +----------------------+
                          |        Total         |
                          |----------------------|
       +------------------+     (100, 1000)      +--------------------+
       |                  +----------+-----------+                    |
       |                             |                                |
 +-----v-----+                 +-----v-----+                   +------v----+
 |    Foo    |                 |    Bar    |                   |Total-other|
 |-----------|                 |-----------|                   |-----------|
 | (15, 100) |             +---+ (80, 200) +-----+             | (5, 700)  |
 +-----------+             |   +-----------+     |             +-----------+
                           |                     |
                    +------v------+       +------v-----+
                    | Bar::Coffee |       | Bar-other  |
                    |-------------|       |------------|
                    |  (30, 120)  |       |  (50, 80)  |
                    +-------------+       +------------+
"""

from memory_inspector.classification import rules


class AggreatedResults(object):
  """A tree of results, where each node is a bucket (root: 'Total' bucket)."""

  def __init__(self, rule_tree, keys):
    """Initializes the bucket tree using the structure of the rules tree.

    Each node of the bucket tree is initialized with a list of len(keys) zeros.
    """
    assert(isinstance(rule_tree, rules.Rule))
    assert(isinstance(keys, list))
    self.keys = keys
    self.total = AggreatedResults._MakeBucketNodeFromRule(rule_tree, len(keys))

  def AddToMatchingNodes(self, trace_record, values):
    """Adds the provided |values| to the nodes that match the |trace_record|.

    Tree traversal logic: at any level, one and only one node will match the
    |trace_record| (in the worst case it will be the catchall *-other rule).
    When a node is matched, the traversal continues in its children and no
    further siblings in the upper levels are visited anymore.
    This is to guarantee that at any level the values of one node are equal to
    the sum of the values of all its children.

    Args:
      trace_record: any kind of object which can be matched by the Match method
          of the Rule object.
      values: a list of int(s) which represent the value associated to the
          matched trace_record. The cardinality of the list must be equal to the
          cardinality of the initial keys.
    """
    assert(len(values) == len(self.keys))
    AggreatedResults._AddToMatchingNodes(
        trace_record, values, self.total, len(self.keys))

  @staticmethod
  def _AddToMatchingNodes(trace_record, values, bucket, num_keys):
    if not bucket.rule.Match(trace_record):
      return False
    for i in xrange(num_keys):
      bucket.values[i] += values[i]
    for child_bucket in bucket.children:
      if AggreatedResults._AddToMatchingNodes(
          trace_record, values, child_bucket, num_keys):
        break
    return True

  @staticmethod
  def _MakeBucketNodeFromRule(rule, num_keys):
    assert(isinstance(rule, rules.Rule))
    bucket = Bucket(rule, num_keys)
    for child_rule in rule.children:
      bucket.children.append(
          AggreatedResults._MakeBucketNodeFromRule(child_rule, num_keys))
    return bucket


class Bucket(object):
  """A bucket is a node in the results tree. """
  def __init__(self, rule, num_keys):
    self.rule = rule
    self.values = [0] * num_keys
    self.children = []

  @property
  def name(self):
    return self.rule.name