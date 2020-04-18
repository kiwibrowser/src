#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from path_util import SplitParent, Split, Segment


class PathUtilTest(unittest.TestCase):

  def testSplitParent(self):
    self.assertEqual(('', 'hi'), SplitParent('hi'))
    self.assertEqual(('', 'hi/'), SplitParent('hi/'))
    self.assertEqual(('/', 'hi'), SplitParent('/hi'))
    self.assertEqual(('/', 'hi/'), SplitParent('/hi/'))
    self.assertEqual(('parent', 'hi'), SplitParent('parent/hi'))
    self.assertEqual(('parent', 'hi/'), SplitParent('parent/hi/'))
    self.assertEqual(('/parent', 'hi'), SplitParent('/parent/hi'))
    self.assertEqual(('/parent', 'hi/'), SplitParent('/parent/hi/'))
    self.assertEqual(('p1/p2', 'hi'), SplitParent('p1/p2/hi'))
    self.assertEqual(('p1/p2', 'hi/'), SplitParent('p1/p2/hi/'))
    self.assertEqual(('/p1/p2', 'hi'), SplitParent('/p1/p2/hi'))
    self.assertEqual(('/p1/p2', 'hi/'), SplitParent('/p1/p2/hi/'))

  def testSplit(self):
    self.assertEqual(['p1/', 'p2/', 'p3'], Split('p1/p2/p3'))
    self.assertEqual(['p1/', 'p2/', 'p3/'], Split('p1/p2/p3/'))
    self.assertEqual([''], Split(''))
    self.assertEqual(['p1/'], Split('p1/'))
    self.assertEqual(['p1'], Split('p1'))

  def testSegment(self):
    self.assertEqual([('', '')], list(Segment('')))
    self.assertEqual([('', 'hi'), ('hi', '')], list(Segment('hi')))
    self.assertEqual([('', 'p1/p2/hi'),
                      ('p1/', 'p2/hi'),
                      ('p1/p2/', 'hi'),
                      ('p1/p2/hi', '')],
                      list(Segment('p1/p2/hi')))
    self.assertEqual([('', 'foo/bar/baz.txt'),
                      ('foo/', 'bar/baz.txt'),
                      ('foo/bar/', 'baz.txt'),
                      ('foo/bar/baz.txt', '')],
                      list(Segment('foo/bar/baz.txt')))


if __name__ == '__main__':
  unittest.main()
