#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from environment import GetAppVersion
from test_object_store import TestObjectStore
from object_store_creator import ObjectStoreCreator

class _FooClass(object):
  def __init__(self): pass

class ObjectStoreCreatorTest(unittest.TestCase):
  def setUp(self):
    self._creator = ObjectStoreCreator(start_empty=False,
                                       store_type=TestObjectStore,
                                       disable_wrappers=True)

  def testVanilla(self):
    store = self._creator.Create(_FooClass)
    self.assertEqual(
        'class=_FooClass&app_version=%s' % GetAppVersion(),
        store.namespace)
    self.assertFalse(store.start_empty)

  def testWithCategory(self):
    store = self._creator.Create(_FooClass, category='hi')
    self.assertEqual(
        'class=_FooClass&category=hi&app_version=%s' % GetAppVersion(),
        store.namespace)
    self.assertFalse(store.start_empty)

  def testWithoutAppVersion(self):
    store = self._creator.Create(_FooClass, app_version=None)
    self.assertEqual('class=_FooClass', store.namespace)
    self.assertFalse(store.start_empty)

  def testStartConfiguration(self):
    store = self._creator.Create(_FooClass, start_empty=True)
    self.assertTrue(store.start_empty)
    store = self._creator.Create(_FooClass, start_empty=False)
    self.assertFalse(store.start_empty)
    self.assertRaises(ValueError, ObjectStoreCreator)

  def testIllegalCharacters(self):
    self.assertRaises(ValueError,
                      self._creator.Create, _FooClass, app_version='1&2')
    self.assertRaises(ValueError,
                      self._creator.Create, _FooClass, category='a=&b')

if __name__ == '__main__':
  unittest.main()
