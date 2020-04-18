# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from memory_inspector.classification import mmap_classifier
from memory_inspector.core import memory_map


_TEST_RULES = """
[
{
  'name': 'anon',
  'mmap_file': r'^\[anon',
  'children': [
    {
      'name': 'jit',
      'mmap_prot': 'r-x',
    },
  ],
},
{
  'name': 'dev',
  'mmap_file': r'^/dev',
  'children': [
    {
      'name': 'gpu',
      'mmap_file': r'/gpu',
    },
  ],
},
{
  'name': 'lib',
  'mmap_file': r'.so$',
  'children': [
    {
      'name': 'data',
      'mmap_prot': 'rw',
    },
    {
      'name': 'text',
      'mmap_prot': 'r-x',
    },
  ],
},
]
"""

_TEST_MMAPS = [
#    START    END      PROT    FILE          P.Dirt  P.Clean  S.Dirt  S.Clean
    (0x00000, 0x03fff, 'rw--', '[anon]',     4096,   0,       4096,   0),
    (0x04000, 0x07fff, 'rw--', '/lib/1.so',  8192,   0,       0,      0),
    (0x08000, 0x0bfff, 'r-x-', '/lib/1.so',  4096,   8192,    0,      0),
    (0x0c000, 0x0ffff, 'rw--', '/lib/2.so',  0,      0,       4096,   8192),
    (0x10000, 0x13fff, 'r-x-', '/lib/2.so',  0,      12288,   0,      4096),
    (0x14000, 0x17fff, 'rw--', '/dev/gpu/1', 4096,   0,       0,      0),
    (0x18000, 0x1bfff, 'rw--', '/dev/gpu/2', 8192,   0,       4096,   0),
    (0x1c000, 0x1ffff, 'rw--', '/dev/foo',   0,      4096,    0,      8192),
    (0x20000, 0x23fff, 'r-x-', '[anon:jit]', 8192,   0,       4096,   0),
    (0x24000, 0x27fff, 'r---', 'OTHER',      0,      0,       8192,   0),
]

_EXPECTED_RESULTS = {
    'Total':                                [36864,  24576,  24576,  20480],
    'Total::anon':                          [12288,  0,      8192,   0],
    'Total::anon::jit':                     [8192,   0,      4096,   0],
    'Total::anon::anon-other':              [4096,   0,      4096,   0],
    'Total::dev':                           [12288,  4096,   4096,   8192],
    'Total::dev::gpu':                      [12288,  0,      4096,   0],
    'Total::dev::dev-other':                [0,      4096,   0,      8192],
    'Total::lib':                           [12288,  20480,  4096,   12288],
    'Total::lib::data':                     [8192,   0,      4096,   8192],
    'Total::lib::text':                     [4096,   20480,  0,      4096],
    'Total::lib::lib-other':                [0,      0,      0,      0],
    'Total::Total-other':                   [0,      0,      8192,   0],
}



class MmapClassifierTest(unittest.TestCase):
  def runTest(self):
    rule_tree = mmap_classifier.LoadRules(_TEST_RULES)
    mmap = memory_map.Map()
    for m in _TEST_MMAPS:
      mmap.Add(memory_map.MapEntry(
          m[0], m[1], m[2], m[3], 0, m[4], m[5], m[6], m[7]))

    res = mmap_classifier.Classify(mmap, rule_tree)

    def CheckResult(node, prefix):
      node_name = prefix + node.name
      self.assertIn(node_name, _EXPECTED_RESULTS)
      subtotal = node.values[0]
      values = node.values[1:]

      # First check that the subtotal matches clean + dirty + shared + priv.
      self.assertEqual(subtotal, values[0] + values[1] + values[2] + values[3])

      # Then check that the single values match the expectations.
      self.assertEqual(values, _EXPECTED_RESULTS[node_name])

      for child in node.children:
        CheckResult(child, node_name + '::')

    CheckResult(res.total, '')