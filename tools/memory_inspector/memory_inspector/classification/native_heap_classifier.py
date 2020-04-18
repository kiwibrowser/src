# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module classifies NativeHeap objects filtering their allocations.

The only filter currently available is 'stacktrace', which works as follows:

{'name': 'rule-1', 'stacktrace': 'foo' }
{'name': 'rule-2', 'stacktrace': ['foo', r'bar\s+baz']}
{'name': 'rule-3', 'source_path': 'sk.*allocator'}
{'name': 'rule-3', 'source_path': 'sk', 'stacktrace': 'SkAllocator'}


rule-1 will match any allocation that has 'foo' in one of its  stack frames.
rule-2 will match any allocation that has a stack frame matching 'foo' AND a
followed by a stack frame matching 'bar baz'. Note that order matters, so rule-2
will not match a stacktrace like ['bar baz', 'foo'].
rule-3 will match any allocation in which at least one of the source paths in
its stack frames matches the regex sk.*allocator.
rule-4 will match any allocation which satisfies both the conditions.

TODO(primiano): introduce more filters after the first prototype with UI, for
instance, filter by library file name or by allocation size.
"""

import collections
import posixpath
import re

from memory_inspector.classification import results
from memory_inspector.classification import rules
from memory_inspector.core import exceptions
from memory_inspector.core import native_heap


_RESULT_KEYS = ['bytes_allocated', 'bytes_resident']


def LoadRules(content):
  """Loads and parses a native-heap rule tree from a content (string).

  Returns:
    An instance of |rules.Rule|, which nodes are |_NHeapRule| instances.
  """
  return rules.Load(content, _NHeapRule)


def Classify(nativeheap, rule_tree):
  """Creates aggregated results of native heaps using the provided rules.

  Args:
    nativeheap: the heap dump being processed (a |NativeHeap| instance).
    rule_tree: the user-defined rules that define the filtering categories.

  Returns:
    An instance of |AggreatedResults|.
  """
  assert(isinstance(nativeheap, native_heap.NativeHeap))
  assert(isinstance(rule_tree, rules.Rule))

  res = results.AggreatedResults(rule_tree, _RESULT_KEYS)
  for allocation in nativeheap.allocations:
    res.AddToMatchingNodes(allocation,
                           [allocation.size, allocation.resident_size])
  return res


def InferHeuristicRulesFromHeap(nheap, max_depth=3, threshold=0.02):
  """Infers the rules tree from a symbolized heap snapshot.

  In lack of a specific set of rules, this method can be invoked to infer a
  meaningful rule tree starting from a heap snapshot. It will build a compact
  radix tree from the source paths of the stack traces, which height is at most
  |max_depth|, selecting only those nodes which contribute for at least
  |threshold| (1.0 = 100%) w.r.t. the total allocation of the heap snapshot.
  """
  assert(isinstance(nheap, native_heap.NativeHeap))

  def RadixTreeInsert(node, path):
    """Inserts a string (path) into a radix tree (a python recursive dict).

    e.g.: [/a/b/c, /a/b/d, /z/h] -> {'/a/b/': {'c': {}, 'd': {}}, '/z/h': {}}
    """

    def GetCommonPrefix(args):
      """Returns the common prefix between two paths (no partial paths).

      e.g.: /tmp/bar, /tmp/baz will return /tmp/ (and not /tmp/ba as the dumb
      posixpath.commonprefix implementation would do)
      """
      parts = posixpath.commonprefix(args).rpartition(posixpath.sep)[0]
      return parts + posixpath.sep if parts else ''

    for node_path in node.iterkeys():
      pfx = GetCommonPrefix([node_path, path])
      if not pfx:
        continue
      if len(pfx) < len(node_path):
        node[pfx] = {node_path[len(pfx):] : node[node_path]}
        del node[node_path]
      if len(path) > len(pfx):
        RadixTreeInsert(node[pfx], path[len(pfx):])
      return
    node[path] = {}  # No common prefix, create new child in current node.

  # Given an allocation of size N and its stack trace, heuristically determines
  # the source directory to be blamed for the N bytes.
  # The blamed_dir is the one which appears more times in the top 8 stack frames
  # (excluding the first 2, which usually are just the (m|c)alloc call sites).
  # At the end, this will generate a *leaderboard* (|blamed_dirs|) which
  # associates, to each source path directory, the number of bytes allocated.

  blamed_dirs = collections.Counter()  # '/s/path' : bytes_from_this_path (int)
  total_allocated = 0
  for alloc in nheap.allocations:
    dir_histogram = collections.Counter()
    for frame in alloc.stack_trace.frames[2:10]:
      # Compute a histogram (for each allocation) of the top source dirs.
      if not frame.symbol or not frame.symbol.source_info:
        continue
      src_file = frame.symbol.source_info[0].source_file_path
      src_dir = posixpath.dirname(src_file.replace('\\', '/')) + '/'
      dir_histogram.update([src_dir])
    if not dir_histogram:
      continue
    # Add the blamed dir to the leaderboard.
    blamed_dir = dir_histogram.most_common()[0][0]
    blamed_dirs.update({blamed_dir : alloc.size})
    total_allocated += alloc.size

  # Select only the top paths from the leaderboard which contribute for more
  # than |threshold| and make a radix tree out of them.
  radix_tree = {}
  for blamed_dir, alloc_size in blamed_dirs.most_common():
    if (1.0 * alloc_size / total_allocated) < threshold:
      break
    RadixTreeInsert(radix_tree, blamed_dir)

  # The final step consists in generating a rule tree from the radix tree. This
  # is a pretty straightforward tree-clone operation, they have the same shape.
  def GenRulesFromRadixTree(radix_tree_node, max_depth, parent_path=''):
    children = []
    if max_depth > 0:
      for node_path, node_children in radix_tree_node.iteritems():
        child_rule = {
            'name': node_path[-16:],
            'source_path': '^' + re.escape(parent_path + node_path),
            'children': GenRulesFromRadixTree(
                node_children, max_depth - 1, parent_path + node_path)}
        children += [child_rule]
    return children

  rules_tree = GenRulesFromRadixTree(radix_tree, max_depth)
  return LoadRules(str(rules_tree))


class _NHeapRule(rules.Rule):
  def __init__(self, name, filters):
    super(_NHeapRule, self).__init__(name)

    # The 'stacktrace' filter can be either a string (simple case, one regex) or
    # a list of strings (complex case, see doc in the header of this file).
    stacktrace_regexs = filters.get('stacktrace', [])
    if isinstance(stacktrace_regexs, basestring):
      stacktrace_regexs = [stacktrace_regexs]
    self._stacktrace_regexs = []
    for regex in stacktrace_regexs:
      try:
        self._stacktrace_regexs.append(re.compile(regex))
      except re.error, descr:
        raise exceptions.MemoryInspectorException(
            'Stacktrace regex error "%s" : %s' % (regex, descr))

    # The 'source_path' regex, instead, simply matches the source file path.
    self._path_regex = None
    path_regex = filters.get('source_path')
    if path_regex:
      try:
        self._path_regex = re.compile(path_regex)
      except re.error, descr:
        raise exceptions.MemoryInspectorException(
            'Path regex error "%s" : %s' % (path_regex, descr))

  def Match(self, allocation):
    # Match the source file path, if the 'source_path' filter is specified.
    if self._path_regex:
      path_matches = False
      for frame in allocation.stack_trace.frames:
        if frame.symbol and frame.symbol.source_info:
          if self._path_regex.search(
              frame.symbol.source_info[0].source_file_path):
            path_matches = True
            break
      if not path_matches:
        return False

    # Match the stack traces symbols, if the 'stacktrace' filter is specified.
    if not self._stacktrace_regexs:
      return True
    cur_regex_idx = 0
    cur_regex = self._stacktrace_regexs[0]
    for frame in allocation.stack_trace.frames:
      if frame.symbol and cur_regex.search(frame.symbol.name):
        # The current regex has been matched.
        if cur_regex_idx == len(self._stacktrace_regexs) - 1:
          return True  # All the provided regexs have been matched, we're happy.
        cur_regex_idx += 1
        cur_regex = self._stacktrace_regexs[cur_regex_idx]

    return False  # Not all the provided regexs have been matched.