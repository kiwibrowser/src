#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Collection of unit tests for 'infra.libs.memoize' library.
"""

import unittest

from infra_libs import memoize

class MemoTestCase(unittest.TestCase):

  def setUp(self):
    self.tagged = set()

  def tag(self, tag=None):
    self.assertNotIn(tag, self.tagged)
    self.tagged.add(tag)

  def assertTagged(self, *tags):
    if len(tags) == 0:
      tags = (None,)
    self.assertEqual(set(tags), self.tagged)

  def clearTagged(self):
    self.tagged.clear()


class FunctionTestCase(MemoTestCase):

  def testFuncNoArgs(self):
    @memoize.memo()
    def func():
      self.tag()
      return 'foo'

    for _ in xrange(10):
      self.assertEqual(func(), 'foo')
    self.assertTagged()

  def testFuncAllArgs(self):
    @memoize.memo()
    def func(a, b):
      self.tag((a, b))
      return a + b

    # Execute multiple rounds of two unique function executions.
    for _ in xrange(10):
      self.assertEqual(func(1, 2), 3)
      self.assertEqual(func(3, 4), 7)
    self.assertTagged(
        (1, 2),
        (3, 4),
    )

  def testFuncIgnoreArgs(self):
    @memoize.memo(ignore=('b'))
    def func(a, b):
      self.tag(a)
      return a + b

    # Execute multiple rounds of two unique function executions.
    for _ in xrange(10):
      self.assertEqual(func(1, 1), 2)
      self.assertEqual(func(1, 2), 2)
      self.assertEqual(func(2, 1), 3)
      self.assertEqual(func(2, 2), 3)
    self.assertTagged(
        1,
        2,
    )

  def testOldClassMethod(self):
    class Test:
      # Disable 'no __init__ method' warning | pylint: disable=W0232
      # pylint: disable=old-style-class

      @classmethod
      @memoize.memo()
      def func(cls, a):
        self.tag(a)
        return a

    # Execute multiple rounds of two unique function executions.
    for _ in xrange(10):
      self.assertEqual(Test.func(1), 1)
      self.assertEqual(Test.func(2), 2)
    self.assertTagged(
        1,
        2,
    )

  def testNewClassMethod(self):
    class Test(object):
      # Disable 'no __init__ method' warning | pylint: disable=W0232

      @classmethod
      @memoize.memo()
      def func(cls, a):
        self.tag(a)
        return a

    # Execute multiple rounds of two unique function executions.
    for _ in xrange(10):
      self.assertEqual(Test.func(1), 1)
      self.assertEqual(Test.func(2), 2)
    self.assertTagged(
        1,
        2,
    )

  def testOldClassStaticMethod(self):
    class Test:
      # Disable 'no __init__ method' warning | pylint: disable=W0232
      # pylint: disable=old-style-class

      @staticmethod
      @memoize.memo()
      def func(a):
        self.tag(a)
        return a

    # Execute multiple rounds of two unique function executions.
    for _ in xrange(10):
      self.assertEqual(Test.func(1), 1)
      self.assertEqual(Test.func(2), 2)
    self.assertTagged(
        1,
        2,
    )

  def testNewClassStaticMethod(self):
    class Test(object):
      # Disable 'no __init__ method' warning | pylint: disable=W0232

      @staticmethod
      @memoize.memo()
      def func(a):
        self.tag(a)
        return a

    # Execute multiple rounds of two unique function executions.
    for _ in xrange(10):
      self.assertEqual(Test.func(1), 1)
      self.assertEqual(Test.func(2), 2)
    self.assertTagged(
        1,
        2,
    )

  def testClearAllArgs(self):
    @memoize.memo()
    def func(a, b=10):
      self.tag((a, b))
      return a + b

    # First round
    self.assertEqual(func(1), 11)
    self.assertEqual(func(1, b=0), 1)
    self.assertTagged(
        (1, 10),
        (1, 0),
    )

    # Clear (1)
    self.clearTagged()
    func.memo_clear(1)

    self.assertEqual(func(1), 11)
    self.assertEqual(func(1, b=0), 1)
    self.assertTagged(
        (1, 10),
    )

    # Clear (1, b=0)
    self.clearTagged()
    func.memo_clear(1, b=0)

    self.assertEqual(func(1), 11)
    self.assertEqual(func(1, b=0), 1)
    self.assertTagged(
        (1, 0),
    )


class MemoInstanceMethodTestCase(MemoTestCase):

  class TestBaseOld:
    # pylint: disable=old-style-class
    def __init__(self, test_case, name):
      self.test_case = test_case
      self.name = name

    def __hash__(self):
      # Prevent this instance from being used as a memo key
      raise NotImplementedError()


  class TestBaseNew(object):
    def __init__(self, test_case, name):
      self.test_case = test_case
      self.name = name

    def __hash__(self):
      # Prevent this instance from being used as a memo key
      raise NotImplementedError()


  class TestHash(object):

    def __init__(self):
      self._counter = 0

    @memoize.memo()
    def __hash__(self):
      assert self._counter == 0
      self._counter += 1
      return self._counter


  def testOldClassNoArgs(self):
    class Test(self.TestBaseOld):
      # Disable 'hash not overridden' warning | pylint: disable=W0223

      @memoize.memo()
      def func(self):
        self.test_case.tag(self.name)
        return 'foo'

    t0 = Test(self, 't0')
    t1 = Test(self, 't1')
    for _ in xrange(10):
      self.assertEqual(t0.func(), 'foo')
      self.assertEqual(t1.func(), 'foo')
    self.assertTagged(
        't0',
        't1',
    )

  def testNewClassNoArgs(self):
    class Test(self.TestBaseNew):
      # Disable 'hash not overridden' warning | pylint: disable=W0223

      @memoize.memo()
      def func(self):
        self.test_case.tag(self.name)
        return 'foo'

    t0 = Test(self, 't0')
    t1 = Test(self, 't1')
    for _ in xrange(10):
      self.assertEqual(t0.func(), 'foo')
      self.assertEqual(t1.func(), 'foo')
    self.assertTagged(
        't0',
        't1',
    )

  def testOldClassArgs(self):
    class Test(self.TestBaseOld):
      # Disable 'hash not overridden' warning | pylint: disable=W0223

      @memoize.memo()
      def func(self, a, b):
        self.test_case.tag((self.name, a, b))
        return a + b

    t0 = Test(self, 't0')
    t1 = Test(self, 't1')
    for _ in xrange(10):
      self.assertEqual(t0.func(1, 2), 3)
      self.assertEqual(t0.func(1, 3), 4)
      self.assertEqual(t1.func(1, 2), 3)
      self.assertEqual(t1.func(1, 3), 4)
    self.assertTagged(
        ('t0', 1, 2),
        ('t0', 1, 3),
        ('t1', 1, 2),
        ('t1', 1, 3),
    )

  def testNewClassArgs(self):
    class Test(self.TestBaseNew):
      # Disable 'hash not overridden' warning | pylint: disable=W0223

      @memoize.memo()
      def func(self, a, b):
        self.test_case.tag((self.name, a, b))
        return a + b

    t0 = Test(self, 't0')
    t1 = Test(self, 't1')
    for _ in xrange(10):
      self.assertEqual(t0.func(1, 2), 3)
      self.assertEqual(t0.func(1, 3), 4)
      self.assertEqual(t1.func(1, 2), 3)
      self.assertEqual(t1.func(1, 3), 4)
    self.assertTagged(
        ('t0', 1, 2),
        ('t0', 1, 3),
        ('t1', 1, 2),
        ('t1', 1, 3),
    )

  def testNewClassDirectCall(self):
    class Test(self.TestBaseNew):
      # Disable 'hash not overridden' warning | pylint: disable=W0223

      @memoize.memo()
      def func(self, a, b):
        self.test_case.tag((self.name, a, b))
        return a + b

    t0 = Test(self, 't0')
    for _ in xrange(10):
      self.assertEqual(t0.func.__get__(t0)(1,2), 3)
    self.assertTagged(
        ('t0', 1, 2),
    )

  def testClear(self):
    class Test(self.TestBaseNew):
      # Disable 'hash not overridden' warning | pylint: disable=W0223

      @memoize.memo()
      def func(self, a):
        self.test_case.tag((self.name, a))
        return a

    # Call '10' and '20'
    t = Test(self, 'test')
    t.func(10)
    self.assertTagged(
        ('test', 10),
    )

    # Clear
    self.clearTagged()
    t.func.memo_clear(10)

    # Call '10'; it should be tagged
    t.func(10)
    self.assertTagged(
        ('test', 10),
    )


  def testOverrideHash(self):
    t = self.TestHash()
    self.assertEquals(hash(t), 1)
    self.assertEquals(hash(t), 1)


class MemoClassMethodTestCase(MemoTestCase):
  """Tests handling of the 'cls' and 'self' parameters"""

  class Test(object):

    def __init__(self, test_case, name):
      self.test_case = test_case
      self.name = name
      self._value = 0

    @memoize.memo(ignore=('tag',))
    def func(self, a, tag):
      self.test_case.tag(tag)
      return a

    @classmethod
    @memoize.memo(ignore=('test_case', 'tag'))
    def class_func(cls, test_case, memo_value, tag):
      test_case.tag(tag)
      return memo_value

    @property
    @memoize.memo()
    def prop(self):
      self.test_case.tag(self.name)
      return self._value

    @prop.setter
    def prop(self, value):
      self._value = value

    @prop.deleter
    def prop(self):
      self._value = 0


  class TestWithEquals(Test):

    def __hash__(self):
      return 7

    def __eq__(self, other):
      return type(other) == type(self)


  def testClassMethodNoEquals(self):
    self.assertEqual(self.Test.class_func(self, 1, 't0'), 1)
    self.assertEqual(self.Test.class_func(self, 1, 't1'), 1)
    self.assertEqual(self.Test.class_func(self, 2, 't2'), 2)

    self.Test.class_func.memo_clear(self.Test, self, 2, None)
    self.assertEqual(self.Test.class_func(self, 1, 't3'), 1)
    self.assertEqual(self.Test.class_func(self, 2, 't4'), 2)

    self.Test.class_func.memo_clear()
    self.assertEqual(self.Test.class_func(self, 1, 't5'), 1)

    self.assertTagged(
        't0',
        't2',
        't4',
        't5',
    )

  def testInstanceMethodNoEquals(self):
    t0 = self.Test(self, 't0')
    t1 = self.Test(self, 't1')

    self.assertEqual(t0.func(1, 't0.0'), 1)
    self.assertEqual(t1.func(1, 't1.0'), 1)

    t1.func.memo_clear(1, None)
    self.assertEqual(t0.func(1, 't0.1'), 1)
    self.assertEqual(t1.func(1, 't1.1'), 1)
    self.assertTagged(
        't0.0',
        't1.0',
        't1.1',
    )

  def testInstanceMethodWithEquals(self):
    t0 = self.TestWithEquals(self, 't0')
    t1 = self.TestWithEquals(self, 't1')

    self.assertEqual(hash(t0), 7)
    self.assertTrue(t0 == t1)
    self.assertEqual(t0.func(1, 't0.0'), 1)
    self.assertEqual(t1.func(1, 't1.0'), 1)

    t1.func.memo_clear(1, None)
    self.assertEqual(t0.func(1, 't0.1'), 1)
    self.assertEqual(t1.func(1, 't1.1'), 1)
    self.assertTagged(
        't0.0',
        't1.0',
        't1.1',
    )

  def testProperty(self):
    t0 = self.Test(self, 't0')
    t1 = self.Test(self, 't1')

    # The property can be set.
    t0.prop = 1024
    t1.prop = 1337
    del(t1.prop)
    for _ in xrange(10):
      self.assertEqual(t0.prop, 1024)
      self.assertEqual(t1.prop, 0)

    self.assertTagged(
        't0',
        't1',
    )


if __name__ == '__main__':
  unittest.main()
