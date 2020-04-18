# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import signal
import time
import unittest

import common_util


class SerializeAttributesTestCase(unittest.TestCase):
  class Foo(object):
    def __init__(self, foo_fighters, whisky_bar):
      # Pylint doesn't like foo and bar, but I guess musical references are OK.
      self.foo_fighters = foo_fighters
      self.whisky_bar = whisky_bar

  def testSerialization(self):
    foo_fighters = self.Foo('1', 2)
    json_dict = common_util.SerializeAttributesToJsonDict(
        {}, foo_fighters, ['foo_fighters', 'whisky_bar'])
    self.assertDictEqual({'foo_fighters': '1', 'whisky_bar': 2}, json_dict)
    # Partial update
    json_dict = common_util.SerializeAttributesToJsonDict(
        {'baz': 42}, foo_fighters, ['whisky_bar'])
    self.assertDictEqual({'baz': 42, 'whisky_bar': 2}, json_dict)
    # Non-existing attribute.
    with self.assertRaises(AttributeError):
      json_dict = common_util.SerializeAttributesToJsonDict(
          {}, foo_fighters, ['foo_fighters', 'whisky_bar', 'baz'])

  def testDeserialization(self):
    foo_fighters = self.Foo('hello', 'world')
    json_dict = {'foo_fighters': 12, 'whisky_bar': 42}
    # Partial.
    foo_fighters = common_util.DeserializeAttributesFromJsonDict(
        json_dict, foo_fighters, ['foo_fighters'])
    self.assertEqual(12, foo_fighters.foo_fighters)
    self.assertEqual('world', foo_fighters.whisky_bar)
    # Complete.
    foo_fighters = common_util.DeserializeAttributesFromJsonDict(
        json_dict, foo_fighters, ['foo_fighters', 'whisky_bar'])
    self.assertEqual(42, foo_fighters.whisky_bar)
    # Non-existing attribute.
    with self.assertRaises(AttributeError):
      json_dict['baz'] = 'bad'
      foo_fighters = common_util.DeserializeAttributesFromJsonDict(
          json_dict, foo_fighters, ['foo_fighters', 'whisky_bar', 'baz'])


class TimeoutScopeTestCase(unittest.TestCase):
  def testTimeoutRaise(self):
    self.assertEquals(0, signal.alarm(0))

    with self.assertRaisesRegexp(common_util.TimeoutError, 'hello'):
      with common_util.TimeoutScope(seconds=1, error_name='hello'):
        signal.pause()
        self.fail()
    self.assertEquals(0, signal.alarm(0))

    with self.assertRaisesRegexp(common_util.TimeoutError, 'world'):
      with common_util.TimeoutScope(seconds=1, error_name='world'):
        time.sleep(2)
    self.assertEquals(0, signal.alarm(0))

  def testCollisionDetection(self):
    ONE_YEAR = 365 * 24 * 60 * 60

    def _mock_callback(signum, frame):
      del signum, frame # unused.

    flag = False
    with self.assertRaises(common_util.TimeoutCollisionError):
      with common_util.TimeoutScope(seconds=ONE_YEAR, error_name=''):
        flag = True
        signal.signal(signal.SIGALRM, _mock_callback)
    self.assertTrue(flag)
    self.assertEquals(0, signal.alarm(0))

    flag = False
    with self.assertRaises(common_util.TimeoutCollisionError):
      with common_util.TimeoutScope(seconds=ONE_YEAR, error_name=''):
        flag = True
        with common_util.TimeoutScope(seconds=ONE_YEAR, error_name=''):
          self.fail()
    self.assertTrue(flag)
    self.assertEquals(0, signal.alarm(0))

    signal.alarm(ONE_YEAR)
    with self.assertRaises(common_util.TimeoutCollisionError):
      with common_util.TimeoutScope(seconds=ONE_YEAR, error_name=''):
        self.fail()
    self.assertEquals(0, signal.alarm(0))


if __name__ == '__main__':
  unittest.main()
