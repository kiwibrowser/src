#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests of the pnacl driver.

This tests the driver_env functionality.
"""

import unittest

import driver_env

class TestDriverEnv(unittest.TestCase):

  def test_EnvSetMany(self):
    myenv = driver_env.Environment()
    self.assertFalse(myenv.has('foo_str'))
    self.assertFalse(myenv.has('foo_unicode'))
    self.assertFalse(myenv.has('foo_list'))
    myenv.setmany(foo_str='string',
                  foo_unicode=u'unicode',
                  foo_list=['a', 'b', 'c'])
    self.assertEqual('string', myenv.getone('foo_str'))
    self.assertEqual(u'unicode', myenv.getone('foo_unicode'))
    self.assertEqual(['a', 'b', 'c'], myenv.get('foo_list'))


if __name__ == '__main__':
  unittest.main()
