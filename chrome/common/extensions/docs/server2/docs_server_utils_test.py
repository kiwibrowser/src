#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from docs_server_utils import StringIdentity


class DocsServerUtilsTest(unittest.TestCase):
  '''Tests for methods in docs_server_utils.
  '''

  # TODO(kalman): There are a bunch of methods in docs_server_utils.
  # Test them all.

  def testStringIdentity(self):
    # The important part really is that these are all different.
    self.assertEqual('C+7Hteo/', StringIdentity('foo'))
    self.assertEqual('Ys23Ag/5', StringIdentity('bar'))
    self.assertEqual('T5FOBOjX', StringIdentity('foo', 'bar'))
    self.assertEqual('K7XzI1GD', StringIdentity('bar', 'foo'))
    self.assertEqual('CXypceHn', StringIdentity('foo', 'bar', 'baz'))
    self.assertEqual('gGo0GTF6', StringIdentity('foo', 'baz', 'bar'))


if __name__ == '__main__':
  unittest.main()
