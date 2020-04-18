# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re
import unittest

from memory_inspector.classification import results
from memory_inspector.classification import rules


class ResultsTest(unittest.TestCase):
  def runTest(self):
    rules_dict = [
      {
        'name': 'a*',
        'regex': '^a.*',
        'children': [
          {
            'name': 'az*',
            'regex': '^az.*'
          }
        ]
      },
      {
        'name': 'b*',
        'regex': '^b.*',
      },
    ]

    rule = rules.Load(str(rules_dict), MockRegexMatchingRule)
    result = results.AggreatedResults(rule, keys=['X', 'Y'])
    self.assertEqual(result.total.name, 'Total')
    self.assertEqual(len(result.total.children), 3)
    self.assertEqual(result.total.children[0].name, 'a*')
    self.assertEqual(result.total.children[1].name, 'b*')
    self.assertEqual(result.total.children[2].name, 'Total-other')
    self.assertEqual(result.total.children[0].children[0].name, 'az*')
    self.assertEqual(result.total.children[0].children[1].name, 'a*-other')

    result.AddToMatchingNodes('aa1', [1, 2])  # -> a*
    result.AddToMatchingNodes('aa2', [3, 4])  # -> a*
    result.AddToMatchingNodes('az', [5, 6])  # -> a*/az*
    result.AddToMatchingNodes('z1', [7, 8])  # -> T-other
    result.AddToMatchingNodes('b1', [9, 10])  # -> b*
    result.AddToMatchingNodes('b2', [11, 12])  # -> b*
    result.AddToMatchingNodes('z2', [13, 14])  # -> T-other

    self.assertEqual(result.total.values, [49, 56])
    self.assertEqual(result.total.children[0].values, [9, 12])
    self.assertEqual(result.total.children[1].values, [20, 22])
    self.assertEqual(result.total.children[0].children[0].values, [5, 6])
    self.assertEqual(result.total.children[0].children[1].values, [4, 6])
    self.assertEqual(result.total.children[2].values, [20, 22])


class MockRegexMatchingRule(rules.Rule):
  def __init__(self, name, filters):
    super(MockRegexMatchingRule, self).__init__(name)
    self._regex = filters['regex']

  def Match(self, s):
    return bool(re.match(self._regex, s))