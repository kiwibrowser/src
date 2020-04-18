#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from environment_wrappers import CreatePersistentObjectStore
import unittest

class PersistentObjectStoreTest(unittest.TestCase):
  '''Tests for PersistentObjectStore. These are all a bit contrived because
  ultimately it comes down to our use of the appengine datastore API, and we
  mock it out for tests anyway. Who knows whether it's correct.
  '''
  def testPersistence(self):
    # First object store.
    object_store = CreatePersistentObjectStore('test')
    object_store.Set('key', 'value')
    self.assertEqual('value', object_store.Get('key').Get())
    # Other object store should have it too.
    another_object_store = CreatePersistentObjectStore('test')
    self.assertEqual('value', another_object_store.Get('key').Get())
    # Setting in the other store should set in both.
    mapping = {'key2': 'value2', 'key3': 'value3'}
    another_object_store.SetMulti(mapping)
    self.assertEqual(mapping, object_store.GetMulti(mapping.keys()).Get())
    self.assertEqual(mapping,
                     another_object_store.GetMulti(mapping.keys()).Get())
    # And delete.
    object_store.DelMulti(mapping.keys())
    self.assertEqual({}, object_store.GetMulti(mapping.keys()).Get())
    self.assertEqual({}, another_object_store.GetMulti(mapping.keys()).Get())

  def testNamespaceIsolation(self):
    object_store = CreatePersistentObjectStore('test')
    another_object_store = CreatePersistentObjectStore('another')
    object_store.Set('key', 'value')
    self.assertEqual(None, another_object_store.Get('key').Get())

if __name__ == '__main__':
  unittest.main()
