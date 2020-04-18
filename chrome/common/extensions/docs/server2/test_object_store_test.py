#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from test_object_store import TestObjectStore
import unittest

class TestObjectStoreTest(unittest.TestCase):
  def testEmpty(self):
    store = TestObjectStore('namespace')
    self.assertEqual(None, store.Get('hi').Get())
    self.assertEqual({}, store.GetMulti(['hi', 'lo']).Get())

  def testNonEmpty(self):
    store = TestObjectStore('namespace')
    store.Set('hi', 'bye')
    self.assertEqual('bye', store.Get('hi').Get())
    self.assertEqual({'hi': 'bye'}, store.GetMulti(['hi', 'lo']).Get())
    store.Set('hi', 'blah')
    self.assertEqual('blah', store.Get('hi').Get())
    self.assertEqual({'hi': 'blah'}, store.GetMulti(['hi', 'lo']).Get())
    store.Del('hi')
    self.assertEqual(None, store.Get('hi').Get())
    self.assertEqual({}, store.GetMulti(['hi', 'lo']).Get())

  def testCheckAndReset(self):
    store = TestObjectStore('namespace')
    store.Set('x', 'y')
    self.assertTrue(*store.CheckAndReset(set_count=1))
    store.Set('x', 'y')
    store.Set('x', 'y')
    self.assertTrue(*store.CheckAndReset(set_count=2))
    store.Set('x', 'y')
    store.Set('x', 'y')
    store.Get('x').Get()
    store.Get('x').Get()
    store.Get('x').Get()
    store.Del('x')
    self.assertTrue(*store.CheckAndReset(get_count=3, set_count=2, del_count=1))

if __name__ == '__main__':
  unittest.main()
