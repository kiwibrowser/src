# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest
import collections

from infra_libs import infra_types


class TestFreeze(unittest.TestCase):
  def testDict(self):
    d = collections.OrderedDict()
    d['cat'] = 100
    d['dog'] = 0

    f = infra_types.freeze(d)
    self.assertEqual(d, f)
    self.assertIsInstance(f, infra_types.FrozenDict)
    self.assertEqual(
        hash(f),
        hash((0, ('cat', 100))) ^ hash((1, ('dog', 0)))
    )
    self.assertEqual(len(d), len(f))

    # Cover equality
    self.assertEqual(f, f)
    self.assertNotEqual(f, 'dog')
    self.assertNotEqual(f, {'bob': 'hat'})
    self.assertNotEqual(f, {'cat': 20, 'dog': 10})

  def testList(self):
    l = [1, 2, {'bob': 100}]
    f = infra_types.freeze(l)
    self.assertSequenceEqual(l, f)
    self.assertIsInstance(f, tuple)

  def testSet(self):
    s = {1, 2, infra_types.freeze({'bob': 100})}
    f = infra_types.freeze(s)
    self.assertEqual(s, f)
    self.assertIsInstance(f, frozenset)


class TestThaw(unittest.TestCase):
  def testDict(self):
    d = {
        'cat': 100,
        'dog': 0,
    }
    f = infra_types.freeze(d)
    t = infra_types.thaw(f)
    self.assertEqual(d, f)
    self.assertEqual(t, f)
    self.assertEqual(d, t)
    self.assertIsInstance(t, collections.OrderedDict)

  def testOrderedDictRetainsOrder(self):
    d = collections.OrderedDict()
    d['cat'] = 100
    d['dog'] = 0
    f = infra_types.freeze(d)
    t = infra_types.thaw(f)
    self.assertEqual(d, f)
    self.assertEqual(t, f)
    self.assertEqual(d, t)
    self.assertIsInstance(t, collections.OrderedDict)

  def testList(self):
    l = [1, 2, {'bob': 100}]
    f = infra_types.freeze(l)
    t = infra_types.thaw(f)
    self.assertSequenceEqual(l, f)
    self.assertSequenceEqual(f, t)
    self.assertSequenceEqual(l, t)
    self.assertIsInstance(t, list)

  def testSet(self):
    s = {1, 2, 'cat'}
    f = infra_types.freeze(s)
    t = infra_types.thaw(f)
    self.assertEqual(s, f)
    self.assertEqual(f, t)
    self.assertEqual(t, s)
    self.assertIsInstance(t, set)
