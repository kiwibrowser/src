# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test common module."""

from __future__ import print_function

from chromite.cros_bisect import common
from chromite.lib import cros_test_lib


class TestCommitInfo(cros_test_lib.TestCase):
  """Tests CommitInfo class."""

  def testEmpty(self):
    """Tests that empty CommitInfo's data members are initialized correctly."""
    info = common.CommitInfo()
    self.assertEqual(
        "CommitInfo(sha1='', title='', score=Score(values=[]), label='', "
        "timestamp=0)",
        repr(info))

  def testAssigned(self):
    """Tests that CommitInfo constrcutor sets up data members correctly."""
    info = common.CommitInfo(
        sha1='abcdef', title='test', score=common.Score(values=[1]),
        label='GOOD', timestamp=100)

    self.assertEqual(
        "CommitInfo(sha1='abcdef', title='test', score=Score(values=[1.0]), "
        "label='GOOD', timestamp=100)",
        repr(info))

  def testEqual(self):
    """Tests equality of two CommitInfo objects."""
    info1 = common.CommitInfo(
        sha1='abcdef', title='test', score=common.Score(values=[1, 2, 3]),
        label='GOOD', timestamp=100)
    info2 = common.CommitInfo(
        sha1='abcdef', title='test', score=common.Score(values=[1, 2, 3]),
        label='GOOD', timestamp=100)
    self.assertEqual(info1, info2)
    # In Python 2.7, __ne__() doesn't delegates to "not __eq__()" so that the
    # sanity check is necessary.
    self.assertFalse(info1 != info2)

  def testNotEqual(self):
    """Tests inequality of two CommitInfo objects."""
    info1 = common.CommitInfo(
        sha1='abcdef', title='test', score=common.Score(values=[1, 2, 3]),
        label='GOOD', timestamp=100)
    info2 = common.CommitInfo(
        sha1='abcdef', title='test', score=common.Score(values=[1, 2]),
        label='GOOD', timestamp=100)
    self.assertNotEqual(info1, info2)
    self.assertFalse(info1 == info2)

  def testBool(self):
    """Tests CommitInfo's boolean value conversion.

    Only default(empty) CommitInfo's boolean value is False.
    """
    info1 = common.CommitInfo()
    self.assertTrue(not info1)
    self.assertFalse(bool(info1))

    info2 = common.CommitInfo(title='something')
    self.assertTrue(bool(info2))
    self.assertFalse(not info2)


class TestScore(cros_test_lib.TestCase):
  """Tests Score class."""

  @staticmethod
  def IsEmpty(score):
    """Checks if a score object is empty.

    Args:
      score: Score object.

    Returns:
      True if score is empty (default / un-assigned).
    """
    return (
        'Score(values=[])' == repr(score) and
        'Score(values=[], mean=0.000, var=0.000, std=0.000)' == str(score) and
        0 == len(score))

  def testEmpty(self):
    """Tests that default Score object is empty."""
    score = common.Score()
    self.assertTrue(self.IsEmpty(score))

  def testScoreInit(self):
    """Tests that Score() sets up data member correctly."""
    score = common.Score([1, 2, 3])
    self.assertEqual('Score(values=[1.0, 2.0, 3.0])', repr(score))
    self.assertEqual(
        'Score(values=[1.0, 2.0, 3.0], mean=2.000, var=1.000, std=1.000)',
        str(score))
    self.assertEqual(3, len(score))

  def testScoreInitWrongType(self):
    """Tests that Init() can handles wrong input type by resetting itself."""
    self.assertTrue(self.IsEmpty(common.Score(['a', 'b'])))
    self.assertTrue(self.IsEmpty(common.Score([])))
    self.assertTrue(self.IsEmpty(common.Score(1)))

  def testScoreUpdate(self):
    """Tests that Update() sets up data member correctly."""
    score = common.Score([1, 2, 3])
    score.Update([2, 4, 6, 8])
    self.assertEqual('Score(values=[2.0, 4.0, 6.0, 8.0])', repr(score))
    self.assertEqual(
        'Score(values=[2.0, 4.0, 6.0, 8.0], mean=5.000, var=6.667, std=2.582)',
        str(score))
    self.assertEqual(4, len(score))

  def testScoreUpdateWrongType(self):
    """Tests that Update() can handles wrong input type by resetting itself."""
    score = common.Score([1, 2, 3])
    score.Update(['a', 'b'])
    self.assertTrue(self.IsEmpty(score))

  def testScoreUpdateEmpty(self):
    """Tests that Update() can handle empty input."""
    score = common.Score([1, 2, 3])
    score.Update([])
    self.assertTrue(self.IsEmpty(score))

  def testScoreUpdateNotAList(self):
    """Tests that Update() can handle wrong input type by resetting itself."""
    score = common.Score([1, 2, 3])
    score.Update(5)
    self.assertTrue(self.IsEmpty(score))

  def testEqual(self):
    """Tests equality of two Score objects."""
    score1 = common.Score([1, 2, 3])
    score2 = common.Score([1, 2, 3])
    self.assertEqual(score1, score2)
    self.assertTrue(score1 == score2)
    self.assertFalse(score1 != score2)

    score3 = common.Score([3, 2, 1])
    self.assertEqual(score1, score3)
    self.assertTrue(score1 == score3)
    self.assertFalse(score1 != score3)

    score4 = common.Score()
    score5 = common.Score([])
    self.assertEqual(score4, score5)
    self.assertTrue(score4 == score5)
    self.assertFalse(score4 != score5)

  def testNotEqual(self):
    """Tests inequality of two Score objects."""
    score1 = common.Score([1, 2])
    score2 = common.Score([1, 2, 3])
    self.assertNotEqual(score1, score2)
    self.assertTrue(score1 != score2)
    self.assertFalse(score1 == score2)

    score3 = common.Score([1, 3])
    self.assertNotEqual(score1, score3)
    self.assertTrue(score1 != score3)
    self.assertFalse(score1 == score3)

    score4 = common.Score()
    score5 = common.Score([0])
    self.assertNotEqual(score4, score5)
    self.assertTrue(score4 != score5)
    self.assertFalse(score4 == score5)

  def testBool(self):
    """Tests Score's boolean conversion.

    Only Score without value is treated as False.
    """
    score1 = common.Score()
    self.assertTrue(not score1)
    self.assertFalse(bool(score1))

    score2 = common.Score([0])
    self.assertTrue(bool(score2))
    self.assertFalse(not score2)


class ClassAOptionsChecker(common.OptionsChecker):
  """Used to test common.OptionsChecker."""
  REQUIRED_ARGS = ('a', )
  def __init__(self, options):
    super(ClassAOptionsChecker, self).__init__(options)


class ClassBOptionsChecker(ClassAOptionsChecker):
  """Used to test common.OptionsChecker."""
  REQUIRED_ARGS = ClassAOptionsChecker.REQUIRED_ARGS + ('b', )
  def __init__(self, options):
    super(ClassBOptionsChecker, self).__init__(options)


class TestOptionsChecker(cros_test_lib.TestCase):
  """Tests OptionsChecker class."""

  def testInit(self):
    """Tests constructor with OptionChecker."""
    options_e = cros_test_lib.EasyAttr()
    options_a = cros_test_lib.EasyAttr(a='a')
    options_b = cros_test_lib.EasyAttr(b='b')
    options_ab = cros_test_lib.EasyAttr(a='a', b='b')
    options_abc = cros_test_lib.EasyAttr(a='a', b='b', c='c')

    # Expect no exceptions.
    common.OptionsChecker(options_e)
    common.OptionsChecker(options_abc)
    ClassAOptionsChecker(options_a)
    ClassBOptionsChecker(options_ab)
    ClassBOptionsChecker(options_abc)

    # Missing 'a' argument.
    with self.assertRaises(common.MissingRequiredOptionsException) as cm:
      ClassAOptionsChecker(options_b)
    exception_message = cm.exception.message
    self.assertTrue('Missing command line' in exception_message)
    self.assertTrue('ClassAOptionsChecker' in exception_message)
    self.assertTrue("['a']" in exception_message)

    # Missing derived 'a' argument.
    with self.assertRaises(common.MissingRequiredOptionsException) as cm:
      ClassBOptionsChecker(options_b)
    exception_message = cm.exception.message
    self.assertTrue('Missing command line' in exception_message)
    self.assertTrue('ClassBOptionsChecker' in exception_message)
    self.assertTrue("['a']" in exception_message)

  def testSanityCheckOptions(self):
    """Like testInit, but just call SanityCheckOptions()."""
    options_e = cros_test_lib.EasyAttr()
    options_a = cros_test_lib.EasyAttr(a='a')
    options_b = cros_test_lib.EasyAttr(b='b')
    options_ab = cros_test_lib.EasyAttr(a='a', b='b')
    options_abc = cros_test_lib.EasyAttr(a='a', b='b', c='c')

    self.assertTrue(common.OptionsChecker.SanityCheckOptions(options_e))
    self.assertTrue(common.OptionsChecker.SanityCheckOptions(options_abc))
    self.assertTrue(ClassAOptionsChecker.SanityCheckOptions(options_a))
    self.assertTrue(ClassBOptionsChecker.SanityCheckOptions(options_ab))
    self.assertTrue(ClassBOptionsChecker.SanityCheckOptions(options_abc))

    # Missing 'a' argument.
    with self.assertRaises(common.MissingRequiredOptionsException) as cm:
      ClassAOptionsChecker.SanityCheckOptions(options_b)
    exception_message = cm.exception.message
    self.assertTrue('Missing command line' in exception_message)
    self.assertTrue('ClassAOptionsChecker' in exception_message)
    self.assertTrue("['a']" in exception_message)

    # Missing derived 'a' argument.
    with self.assertRaises(common.MissingRequiredOptionsException) as cm:
      ClassBOptionsChecker.SanityCheckOptions(options_b)
    exception_message = cm.exception.message
    self.assertTrue('Missing command line' in exception_message)
    self.assertTrue('ClassBOptionsChecker' in exception_message)
    self.assertTrue("['a']" in exception_message)
