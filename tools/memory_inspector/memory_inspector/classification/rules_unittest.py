# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from memory_inspector.classification import rules


_TEST_RULE = """
[
{
  'name': '1',
  'mmap-file': r'/foo/1',
  'foo': 'bar',
  'children': [
     {
       'name': '1/1',
       'mmap-file': r'/foo/1/1',
       'children': []
     },
     {
       'name': '1/2',
       'mmap-file': r'/foo/1/2',
     },
  ]
},
{
  'name': '2',
  'mmap-file': r'/bar/2',
  'children': [
     {
       'name': '2/1',
       'mmap-file': r'/bar/2/1',
     },
     {
       'name': '2/2',
       'mmap-file': r'/bar/2/2',
       'children': [
         {
           'name': '2/2/1',
           'mmap-file': r'/bar/2/2/1',
         },
         {
           'name': '2/2/2',
           'mmap-file': r'/bar/2/2/2',
         },
       ]
     },
     {
       'name': '2/3',
       'mmap-file': r'/bar/3',
     },
  ]
},
]
"""


class RulesTest(unittest.TestCase):
  def runTest(self):
    rt = rules.Load(_TEST_RULE, MockRule)
    self.assertEqual(rt.name, 'Total')
    self.assertEqual(len(rt.children), 3)
    node1 = rt.children[0]
    node2 = rt.children[1]
    node3 = rt.children[2]

    # Check 1-st level leaves.
    self.assertEqual(node1.name, '1')
    self.assertEqual(node1.filters['mmap-file'], '/foo/1')
    self.assertEqual(node1.filters['foo'], 'bar')
    self.assertEqual(node2.name, '2')
    self.assertEqual(node2.filters['mmap-file'], '/bar/2')
    self.assertEqual(node3.name, 'Total-other')

    # Check 2-nd level leaves and their children.
    self.assertEqual(len(node1.children), 3)
    self.assertEqual(node1.children[0].name, '1/1')
    self.assertEqual(node1.children[1].name, '1/2')
    self.assertEqual(node1.children[2].name, '1-other')
    self.assertEqual(len(node2.children), 4)
    self.assertEqual(node2.children[0].name, '2/1')
    self.assertEqual(len(node2.children[0].children), 0)
    self.assertEqual(node2.children[1].name, '2/2')
    self.assertEqual(len(node2.children[1].children), 3)
    self.assertEqual(node2.children[2].name, '2/3')
    self.assertEqual(len(node2.children[2].children), 0)
    self.assertEqual(node2.children[3].name, '2-other')
    self.assertEqual(len(node2.children[3].children), 0)



class MockRule(rules.Rule):
  def __init__(self, name, filters):
    super(MockRule, self).__init__(name)
    self.filters = filters