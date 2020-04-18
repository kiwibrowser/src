# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for the partial_mock test helper code."""

from __future__ import print_function

from chromite.lib import cros_test_lib
from chromite.lib import partial_mock


# pylint: disable=W0212


class ComparatorTest(cros_test_lib.TestCase):
  """Test Comparitor functionality."""
  TEST_KEY1 = 'monkey'
  TEST_KEY2 = 'foon'

  def testEquals(self):
    """__eq__, __ne__ functionality of Comparator classes."""
    for cls_name in ['In', 'Regex', 'ListRegex']:
      cls = getattr(partial_mock, cls_name)
      obj1 = cls(self.TEST_KEY1)
      obj2 = cls(self.TEST_KEY1)
      obj3 = cls(self.TEST_KEY2)
      self.assertEquals(obj1, obj2)
      self.assertFalse(obj1 == obj3)
      self.assertNotEquals(obj1, obj3)

  def testIgnoreEquals(self):
    """Verify __eq__ functionality for Ignore."""
    obj1 = partial_mock.Ignore()
    obj2 = partial_mock.Ignore()
    self.assertEquals(obj1, obj2)
    self.assertFalse(obj1 != obj2)

  def testListRegex(self):
    """Verify ListRegex match functionality."""
    obj = partial_mock.ListRegex('.*monkey.*')
    self.assertTrue(obj.Match(['the', 'small monkeys', 'jumped']))
    self.assertFalse(obj.Match(['the', 'jumped']))
    self.assertFalse(obj.Match(None))
    self.assertFalse(obj.Match(1))


class RecursiveCompareTest(cros_test_lib.TestCase):
  """Test recursive compare functionality."""

  LHS_DICT = {3: 1, 1: 2}
  RHS_DICT = {1: 2, 3: 1}
  LIST = [1, 2, 3, 4]
  TUPLE = (1, 2, 3, 4)

  def TrueHelper(self, lhs, rhs):
    self.assertTrue(partial_mock._RecursiveCompare(lhs, rhs))

  def FalseHelper(self, lhs, rhs):
    self.assertFalse(partial_mock._RecursiveCompare(lhs, rhs))

  def testIt(self):
    """Test basic equality cases."""
    self.TrueHelper(self.LHS_DICT, self.RHS_DICT)
    self.TrueHelper({3: self.LIST, 1: self.LHS_DICT},
                    {1: self.LHS_DICT, 3: self.LIST})
    self.FalseHelper({1: self.LHS_DICT, 3: self.LIST},
                     {1: self.LHS_DICT, 3: self.LIST + [5]})
    self.FalseHelper(self.LIST, self.TUPLE)

  def testUnicode(self):
    """Test recursively comparing unicode and non-unicode strings."""
    self.assertTrue(partial_mock._RecursiveCompare(['foo'], [u'foo']))


class ListContainsTest(cros_test_lib.TestCase):
  """Unittests for ListContains method."""

  L = range(10) + range(10) + [9]
  STRICTLY_TRUE_LISTS = [range(10), range(9, 10), range(3, 6), range(1), [],
                         [9, 9]]
  LOOSELY_TRUE_LISTS = [range(0, 10, 2), range(3, 6, 2), [1, 1]]
  FALSE_LISTS = [[1.5], [-1], [1, 1, 1], [10], [22], range(6, 11), range(-1, 5)]

  def testStrictContains(self):
    """Test ListContains with strict=True."""
    for x in self.STRICTLY_TRUE_LISTS:
      self.assertTrue(partial_mock.ListContains(x, self.L, strict=True))
    for x in self.LOOSELY_TRUE_LISTS + self.FALSE_LISTS:
      self.assertFalse(partial_mock.ListContains(x, self.L, strict=True))

  def testLooseContains(self):
    """Test ListContains with strict=False."""
    for x in self.STRICTLY_TRUE_LISTS + self.LOOSELY_TRUE_LISTS:
      self.assertTrue(partial_mock.ListContains(x, self.L))
    for x in self.FALSE_LISTS:
      self.assertFalse(partial_mock.ListContains(x, self.L))

  def testUnicode(self):
    """Test ListContains with unicode and non-unicode strings."""
    self.assertTrue(partial_mock.ListContains(['foo'], [u'foo']))


class HasStringTest(cros_test_lib.TestCase):
  """Unittests for HasString."""
  def testEqual(self):
    self.assertTrue(
        partial_mock.HasString('substring') ==
        'sentence with substring...')
    self.assertTrue(
        partial_mock.HasString('tr') == 'it should be true')
    self.assertTrue(
        partial_mock.HasString('') == 'match any string')

  def testUneuqal(self):
    self.assertFalse(
        partial_mock.HasString('not there') == 'typo no there')
    self.assertFalse(
        partial_mock.HasString('Uppercase matters') == 'uppercase matters')


class MockedCallResultsTest(cros_test_lib.TestCase):
  """Test MockedCallResults functionality."""

  ARGS = ('abc',)
  LIST_ARGS = ([1, 2, 3, 4],)
  KWARGS = {'test': 'ing'}
  NEW_ENTRY = {'new': 'entry'}

  def KwargsHelper(self, result, kwargs, strict=True):
    self.mr.AddResultForParams(self.ARGS, result, kwargs=kwargs,
                               strict=strict)

  def setUp(self):
    self.mr = partial_mock.MockedCallResults('SomeFunction')

  def testNoMock(self):
    """The call is not mocked."""
    self.assertRaises(AssertionError, self.mr.LookupResult, self.ARGS)

  def testArgReplacement(self):
    """Replacing mocks for args-only calls."""
    self.mr.AddResultForParams(self.ARGS, 1)
    self.mr.AddResultForParams(self.ARGS, 2)
    self.assertEquals(2, self.mr.LookupResult(self.ARGS))

  def testKwargsStrictReplacement(self):
    """Replacing strict kwargs mock with another strict mock."""
    self.KwargsHelper(1, self.KWARGS)
    self.KwargsHelper(2, self.KWARGS)
    self.assertEquals(2, self.mr.LookupResult(self.ARGS, kwargs=self.KWARGS))

  def testKwargsNonStrictReplacement(self):
    """Replacing strict kwargs mock with nonstrict mock."""
    self.KwargsHelper(1, self.KWARGS)
    self.KwargsHelper(2, self.KWARGS, strict=False)
    self.assertEquals(2, self.mr.LookupResult(self.ARGS, kwargs=self.KWARGS))

  def testListArgLookup(self):
    """Matching of arguments containing lists."""
    self.mr.AddResultForParams(self.LIST_ARGS, 1)
    self.mr.AddResultForParams(self.ARGS, 1)
    self.assertEquals(1, self.mr.LookupResult(self.LIST_ARGS))

  def testKwargsStrictLookup(self):
    """Strict lookup fails due to extra kwarg."""
    self.KwargsHelper(1, self.KWARGS)
    kwargs = self.NEW_ENTRY
    kwargs.update(self.KWARGS)
    self.assertRaises(AssertionError, self.mr.LookupResult, self.ARGS,
                      kwargs=kwargs)

  def testKwargsNonStrictLookup(self):
    """"Nonstrict lookup passes with extra kwarg."""
    self.KwargsHelper(1, self.KWARGS, strict=False)
    kwargs = self.NEW_ENTRY
    kwargs.update(self.KWARGS)
    self.assertEquals(1, self.mr.LookupResult(self.ARGS, kwargs=kwargs))

  def testIgnoreMatching(self):
    """Deep matching of Ignore objects."""
    ignore = partial_mock.Ignore()
    self.mr.AddResultForParams((ignore, ignore), 1, kwargs={'test': ignore})
    self.assertEquals(
        1, self.mr.LookupResult(('some', 'values'), {'test': 'bla'}))

  def testRegexMatching(self):
    """Regex matching."""
    self.mr.AddResultForParams((partial_mock.Regex('pre.ix'),), 1)
    self.mr.AddResultForParams((partial_mock.Regex('suffi.'),), 2)
    self.assertEquals(1, self.mr.LookupResult(('prefix',)))
    self.assertEquals(2, self.mr.LookupResult(('suffix',)))

  def testMultipleMatches(self):
    """Lookup matches mutilple results."""
    self.mr.AddResultForParams((partial_mock.Ignore(),), 1)
    self.mr.AddResultForParams((partial_mock.In('test'),), 2)
    self.assertRaises(AssertionError, self.mr.LookupResult, ('test',))

  def testDefaultResult(self):
    """Test default result matching."""
    self.mr.SetDefaultResult(1)
    self.mr.AddResultForParams((partial_mock.In('test'),), 2)
    self.assertEquals(1, self.mr.LookupResult(self.ARGS))
    self.assertEquals(2, self.mr.LookupResult(('test',)))

  def _ExampleHook(self, *args, **kwargs):
    """Example hook for testing."""
    self.assertEquals(args, self.LIST_ARGS)
    self.assertEquals(kwargs, self.KWARGS)
    return 2

  def testHook(self):
    """Return value of hook is used as the final result."""
    self.mr.AddResultForParams(self.ARGS, 1, side_effect=self._ExampleHook)
    self.assertEqual(
        2, self.mr.LookupResult(self.ARGS, hook_args=self.LIST_ARGS,
                                hook_kwargs=self.KWARGS))

  def testDefaultHook(self):
    """Verify default hooks are used."""
    self.mr.SetDefaultResult(1, self._ExampleHook)
    self.mr.AddResultForParams((partial_mock.In('test'),), 3)
    self.assertEqual(
        2, self.mr.LookupResult(self.ARGS, hook_args=self.LIST_ARGS,
                                hook_kwargs=self.KWARGS))
    self.assertEquals(3, self.mr.LookupResult(('test',)))

  class _DummyException(Exception):
    """A do-nothing exception class for test."""

  def testExceptionInstanceRaise(self):
    """Verify that exception is raised."""
    expected_msg = 'expected exception'
    self.mr.AddResultForParams(
        (partial_mock.In('test'),), 3,
        side_effect=self._DummyException(expected_msg))
    with self.assertRaisesRegexp(self._DummyException, expected_msg):
      self.mr.LookupResult(('test',))

  def testExceptionClassRaise(self):
    """Verify that exception is raised."""
    self.mr.AddResultForParams((partial_mock.In('test'),), 3,
                               side_effect=self._DummyException)
    with self.assertRaises(self._DummyException):
      self.mr.LookupResult(('test',))
