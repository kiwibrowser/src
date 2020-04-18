# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for factory.py."""

from __future__ import print_function

from chromite.lib import cros_test_lib
from chromite.lib import factory

def _GET_OBJECT():
  return object()

class FactoryTest(cros_test_lib.TestCase):
  """Test that ObjectFactory behaves as expected."""

  _OBJECT_NAME = 'Test Object Name'
  _OBJECT_TYPES = {
      't0' : _GET_OBJECT,
      't1' : factory.CachedFunctionCall(_GET_OBJECT),
      't3' : factory.CachedFunctionCall(_GET_OBJECT),
      't4' : None,
  }

  def _allowed_transitions(self, from_setup, to_setup):
    return from_setup == 't3' and to_setup == 't4'

  def setUp(self):
    self.of = factory.ObjectFactory(self._OBJECT_NAME, self._OBJECT_TYPES)
    self.of2 = factory.ObjectFactory(self._OBJECT_NAME, self._OBJECT_TYPES,
                                     self._allowed_transitions)

  def testGetInstance(self):
    self.of.Setup('t0')
    a = self.of.GetInstance()
    self.assertNotEqual(a, self.of.GetInstance())

  def testGetCachedInstance(self):
    self.of.Setup('t1')
    a = self.of.GetInstance()
    self.assertEqual(a, self.of.GetInstance())

  def testDuplicateSetupForbidden(self):
    self.of.Setup('t0')
    with self.assertRaises(factory.ObjectFactoryIllegalOperation):
      self.of.Setup('t0')

  def testNotSetup(self):
    with self.assertRaises(factory.ObjectFactoryIllegalOperation):
      self.of.GetInstance()

  def testUnknownSetupForbidden(self):
    with self.assertRaises(factory.ObjectFactoryIllegalOperation):
      self.of.Setup('unknown setup type')

  def testSetupWithInstanceForbidden(self):
    with self.assertRaises(factory.ObjectFactoryIllegalOperation):
      self.of.Setup('t0', None)

  def testSetupWithInstanceAllowed(self):
    self.of.Setup('t4', None)

  def testForbiddenTransition(self):
    self.of2.Setup('t0')
    with self.assertRaises(factory.ObjectFactoryIllegalOperation):
      self.of2.Setup('t1')

  def testAllowedTransition(self):
    self.of2.Setup('t3')
    a = self.of2.GetInstance()
    self.of2.Setup('t4', None)
    self.assertNotEqual(a, self.of2.GetInstance())
