# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from memory_inspector.classification import native_heap_classifier
from memory_inspector.core import native_heap
from memory_inspector.core import stacktrace
from memory_inspector.core import symbol


_TEST_RULES = """
[
{
  'name': 'content',
  'source_path': r'content/',
  'children': [
    {
      'name': 'browser',
      'stacktrace': r'content::browser',
      'source_path': r'content/browser',
    },
    {
      'name': 'renderer',
      'stacktrace': r'content::renderer',
    },
  ],
},
{
  'name': 'ashmem_in_skia',
  'stacktrace': [r'sk::', r'ashmem::'],
},
]
"""

_TEST_STACK_TRACES = [
    (3, [('stack_frame_0::foo()', '/ignored.c'),
         ('this_goes_under_totals_other', '/ignored.c')]),
    (5, [('foo', '/ignored.c'),
         ('content::browser::something()', '/content/browser/something.cc'),
         ('bar', '/ignored.c')]),
    (7, [('content::browser::something_else()', '/content/browser/else.cc')]),
    (11, [('content::browser::not_really()', '/content/subtle/something.cc'),
          ('foo', '/ignored.c')]),
    (13, [('foo', '/ignored.c'),
          ('content::renderer::something()', '/content/renderer/foo.c'),
          ('bar', '/ignored.c')]),
    (17, [('content::renderer::something_else()', '/content/renderer/foo.c')]),
    (19, [('content::renderer::something_else_2()', '/content/renderer/bar.c'),
          ('foo', '/ignored.c')]),
    (23, [('content::something_different', '/content/foo.c')]),
    (29, [('foo', '/ignored.c'),
          ('sk::something', '/skia/something.c'),
          ('not_ashsmem_goes_into_totals_other', '/ignored.c')]),
    (31, [('foo', '/ignored.c'),
          ('sk::something', '/skia/something.c'),
          ('foo::bar', '/ignored.c'),
          ('sk::foo::ashmem::alloc()', '/skia/ashmem.c')]),
    (37, [('foo', '/ignored.c'),
          ('sk::something', '/ignored.c'),
          ('sk::foo::ashmem::alloc()', '/ignored.c')]),
    (43, [('foo::ashmem::alloc()', '/ignored.c'),
          ('sk::foo', '/ignored.c'),
          ('wrong_order_goes_into_totals', '/ignored.c')])
]

_EXPECTED_RESULTS = {
    'Total':                         [238, 0],
    'Total::content':                [95, 0],
    'Total::content::browser':       [12, 0],  # 5 + 7.
    'Total::content::renderer':      [49, 0],  # 13 + 17 + 19.
    'Total::content::content-other': [34, 0],
    'Total::ashmem_in_skia':         [68, 0],  # 31 + 37.
    'Total::Total-other':            [75, 0],  # 3 + 29 + 43.
}

_HEURISTIC_TEST_STACK_TRACES = [
    (10, '/root/base1/foo/bar/file.cc'),  # Contrib: 0.13
    (20, '/root/base1/foo/baz/file.cc'),  # Contrib: 0.26
    (1, '/root/base1/foo/nah/file.cc'),   # Contrib: 0.01
    (3, '/root/base2/file.cc'),           # Contrib: 0.03
    (22, '/root/base2/subpath/file.cc'),  # Contrib: 0.28
    (18, '/root/base2/subpath2/file.cc'), # Contrib: 0.23
    (2, '/root/whatever/file.cc'),        # Contrib: 0.02
]

_HEURISTIC_EXPECTED_RESULTS = {
    'Total':                                        [76, 0],
    'Total::/root/':                                [76, 0],
    'Total::/root/::base1/foo/':                    [31, 0],  # 10 + 20 +1
    'Total::/root/::base1/foo/::bar/':              [10, 0],
    'Total::/root/::base1/foo/::baz/':              [20, 0],
    'Total::/root/::base1/foo/::base1/foo/-other':  [1, 0],
    'Total::/root/::base2/':                        [43, 0],  # 3 + 22 + 18
    'Total::/root/::base2/::subpath/':              [22, 0],
    'Total::/root/::base2/::subpath2/':             [18, 0],
    'Total::/root/::base2/::base2/-other':          [3, 0],
    'Total::/root/::/root/-other':                  [2, 0],
    'Total::Total-other':                           [0, 0],
}


class NativeHeapClassifierTest(unittest.TestCase):
  def testStandardRuleParsingAndProcessing(self):
    rule_tree = native_heap_classifier.LoadRules(_TEST_RULES)
    nheap = native_heap.NativeHeap()
    mock_addr = 0
    for test_entry in _TEST_STACK_TRACES:
      mock_strace = stacktrace.Stacktrace()
      for (mock_btstr, mock_source_path) in test_entry[1]:
        mock_addr += 4  # Addr is irrelevant, just keep it distinct.
        mock_frame = stacktrace.Frame(mock_addr)
        mock_frame.SetSymbolInfo(symbol.Symbol(mock_btstr, mock_source_path))
        mock_strace.Add(mock_frame)
      nheap.Add(native_heap.Allocation(
          size=test_entry[0], stack_trace=mock_strace))

    res = native_heap_classifier.Classify(nheap, rule_tree)
    self._CheckResult(res.total, '', _EXPECTED_RESULTS)

  def testInferHeuristicRules(self):
    nheap = native_heap.NativeHeap()
    mock_addr = 0
    for (mock_alloc_size, mock_source_path) in _HEURISTIC_TEST_STACK_TRACES:
      mock_strace = stacktrace.Stacktrace()
      mock_addr += 4  # Addr is irrelevant, just keep it distinct.
      mock_frame = stacktrace.Frame(mock_addr)
      mock_frame.SetSymbolInfo(symbol.Symbol(str(mock_addr), mock_source_path))
      for _ in xrange(10):  # Just repeat the same stack frame 10 times
        mock_strace.Add(mock_frame)
      nheap.Add(native_heap.Allocation(
          size=mock_alloc_size, stack_trace=mock_strace))

    rule_tree = native_heap_classifier.InferHeuristicRulesFromHeap(
        nheap, threshold=0.05)
    res = native_heap_classifier.Classify(nheap, rule_tree)
    self._CheckResult(res.total, '', _HEURISTIC_EXPECTED_RESULTS)

  def _CheckResult(self, node, prefix, expected_results):
    node_name = prefix + node.name
    self.assertIn(node_name, expected_results)
    self.assertEqual(node.values, expected_results[node_name])
    for child in node.children:
      self._CheckResult(child, node_name + '::', expected_results)