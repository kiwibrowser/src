#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import traceback
import unittest


from future import All, Future, Race
from mock_function import MockFunction


class FutureTest(unittest.TestCase):
  def testNoValueOrDelegate(self):
    self.assertRaises(ValueError, Future)

  def testValue(self):
    future = Future(value=42)
    self.assertEqual(42, future.Get())
    self.assertEqual(42, future.Get())

  def testDelegateValue(self):
    called = [False,]
    def callback():
      self.assertFalse(called[0])
      called[0] = True
      return 42
    future = Future(callback=callback)
    self.assertEqual(42, future.Get())
    self.assertEqual(42, future.Get())

  def testErrorThrowingDelegate(self):
    class FunkyException(Exception):
      pass

    # Set up a chain of functions to test the stack trace.
    def qux():
      raise FunkyException()
    def baz():
      return qux()
    def bar():
      return baz()
    def foo():
      return bar()
    chain = [foo, bar, baz, qux]

    called = [False,]
    def callback():
      self.assertFalse(called[0])
      called[0] = True
      return foo()

    fail = self.fail
    assertTrue = self.assertTrue
    def assert_raises_full_stack(future, err):
      try:
        future.Get()
        fail('Did not raise %s' % err)
      except Exception as e:
        assertTrue(isinstance(e, err))
        stack = traceback.format_exc()
        assertTrue(all(stack.find(fn.__name__) != -1 for fn in chain))

    future = Future(callback=callback)
    assert_raises_full_stack(future, FunkyException)
    assert_raises_full_stack(future, FunkyException)

  def testAll(self):
    def callback_with_value(value):
      return MockFunction(lambda: value)

    # Test a single value.
    callback = callback_with_value(42)
    future = All((Future(callback=callback),))
    self.assertTrue(*callback.CheckAndReset(0))
    self.assertEqual([42], future.Get())
    self.assertTrue(*callback.CheckAndReset(1))

    # Test multiple callbacks.
    callbacks = (callback_with_value(1),
                 callback_with_value(2),
                 callback_with_value(3))
    future = All(Future(callback=callback) for callback in callbacks)
    for callback in callbacks:
      self.assertTrue(*callback.CheckAndReset(0))
    self.assertEqual([1, 2, 3], future.Get())
    for callback in callbacks:
      self.assertTrue(*callback.CheckAndReset(1))

    # Test throwing an error.
    def throws_error():
      raise ValueError()
    callbacks = (callback_with_value(1),
                 callback_with_value(2),
                 MockFunction(throws_error))

    future = All(Future(callback=callback) for callback in callbacks)
    for callback in callbacks:
      self.assertTrue(*callback.CheckAndReset(0))
    self.assertRaises(ValueError, future.Get)
    for callback in callbacks:
      # Can't check that the callbacks were actually run because in theory the
      # Futures can be resolved in any order.
      callback.CheckAndReset(0)

    # Test throwing an error with except_pass.
    future = All((Future(callback=callback) for callback in callbacks),
                 except_pass=ValueError)
    for callback in callbacks:
      self.assertTrue(*callback.CheckAndReset(0))
    self.assertEqual([1, 2, None], future.Get())

  def testRaceSuccess(self):
    callback = MockFunction(lambda: 42)

    # Test a single value.
    race = Race((Future(callback=callback),))
    self.assertTrue(*callback.CheckAndReset(0))
    self.assertEqual(42, race.Get())
    self.assertTrue(*callback.CheckAndReset(1))

    # Test multiple success values. Note that we could test different values
    # and check that the first returned, but this is just an implementation
    # detail of Race. When we have parallel Futures this might not always hold.
    race = Race((Future(callback=callback),
                 Future(callback=callback),
                 Future(callback=callback)))
    self.assertTrue(*callback.CheckAndReset(0))
    self.assertEqual(42, race.Get())
    # Can't assert the actual count here for the same reason as above.
    callback.CheckAndReset(99)

    # Test values with except_pass.
    def throws_error():
      raise ValueError()
    race = Race((Future(callback=callback),
                 Future(callback=throws_error)),
                 except_pass=(ValueError,))
    self.assertTrue(*callback.CheckAndReset(0))
    self.assertEqual(42, race.Get())
    self.assertTrue(*callback.CheckAndReset(1))

  def testRaceErrors(self):
    def throws_error():
      raise ValueError()

    # Test a single error.
    race = Race((Future(callback=throws_error),))
    self.assertRaises(ValueError, race.Get)

    # Test multiple errors. Can't use different error types for the same reason
    # as described in testRaceSuccess.
    race = Race((Future(callback=throws_error),
                 Future(callback=throws_error),
                 Future(callback=throws_error)))
    self.assertRaises(ValueError, race.Get)

    # Test values with except_pass.
    def throws_except_error():
      raise NotImplementedError()
    race = Race((Future(callback=throws_error),
                 Future(callback=throws_except_error)),
                 except_pass=(NotImplementedError,))
    self.assertRaises(ValueError, race.Get)

    race = Race((Future(callback=throws_error),
                 Future(callback=throws_error)),
                 except_pass=(ValueError,))
    self.assertRaises(ValueError, race.Get)

    # Test except_pass with default values.
    race = Race((Future(callback=throws_error),
                 Future(callback=throws_except_error)),
                 except_pass=(NotImplementedError,),
                 default=42)
    self.assertRaises(ValueError, race.Get)

    race = Race((Future(callback=throws_error),
                 Future(callback=throws_error)),
                 except_pass=(ValueError,),
                 default=42)
    self.assertEqual(42, race.Get())

  def testThen(self):
    def assertIs42(val):
      self.assertEqual(val, 42)
      return val

    then = Future(value=42).Then(assertIs42)
    # Shouldn't raise an error.
    self.assertEqual(42, then.Get())

    # Test raising an error.
    then = Future(value=41).Then(assertIs42)
    self.assertRaises(AssertionError, then.Get)

    # Test setting up an error handler.
    def handle(error):
      if isinstance(error, ValueError):
        return 'Caught'
      raise error

    def raiseValueError():
      raise ValueError

    def raiseException():
      raise Exception

    then = Future(callback=raiseValueError).Then(assertIs42, handle)
    self.assertEqual('Caught', then.Get())
    then = Future(callback=raiseException).Then(assertIs42, handle)
    self.assertRaises(Exception, then.Get)

    # Test chains of thens.
    addOne = lambda val: val + 1
    then = Future(value=40).Then(addOne).Then(addOne).Then(assertIs42)
    # Shouldn't raise an error.
    self.assertEqual(42, then.Get())

    # Test error in chain.
    then = Future(value=40).Then(addOne).Then(assertIs42).Then(addOne)
    self.assertRaises(AssertionError, then.Get)

    # Test handle error in chain.
    def raiseValueErrorWithVal(val):
      raise ValueError

    then = Future(value=40).Then(addOne).Then(raiseValueErrorWithVal).Then(
        addOne, handle).Then(lambda val: val + ' me')
    self.assertEquals(then.Get(), 'Caught me')

    # Test multiple handlers.
    def myHandle(error):
      if isinstance(error, AssertionError):
        return 10
      raise error

    then = Future(value=40).Then(assertIs42).Then(addOne, handle).Then(addOne,
                                                                       myHandle)
    self.assertEquals(then.Get(), 10)

  def testThenResolvesReturnedFutures(self):
    def returnsFortyTwo():
      return Future(value=42)
    def inc(x):
      return x + 1
    def incFuture(x):
      return Future(value=x + 1)

    self.assertEqual(43, returnsFortyTwo().Then(inc).Get())
    self.assertEqual(43, returnsFortyTwo().Then(incFuture).Get())
    self.assertEqual(44, returnsFortyTwo().Then(inc).Then(inc).Get())
    self.assertEqual(44, returnsFortyTwo().Then(inc).Then(incFuture).Get())
    self.assertEqual(44, returnsFortyTwo().Then(incFuture).Then(inc).Get())
    self.assertEqual(
        44, returnsFortyTwo().Then(incFuture).Then(incFuture).Get())

    # The same behaviour should apply to error handlers.
    def raisesSomething():
      def boom(): raise ValueError
      return Future(callback=boom)
    def shouldNotHappen(_):
      raise AssertionError()
    def oops(error):
      return 'oops'
    def oopsFuture(error):
      return Future(value='oops')

    self.assertEqual(
        'oops', raisesSomething().Then(shouldNotHappen, oops).Get())
    self.assertEqual(
        'oops', raisesSomething().Then(shouldNotHappen, oopsFuture).Get())
    self.assertEqual(
        'oops',
        raisesSomething().Then(shouldNotHappen, raisesSomething)
                         .Then(shouldNotHappen, oops).Get())
    self.assertEqual(
        'oops',
        raisesSomething().Then(shouldNotHappen, raisesSomething)
                         .Then(shouldNotHappen, oopsFuture).Get())


if __name__ == '__main__':
  unittest.main()
