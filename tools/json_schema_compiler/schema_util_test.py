#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from schema_util import JsFunctionNameToClassName
from schema_util import StripNamespace
import unittest

class SchemaUtilTest(unittest.TestCase):
  def testStripNamespace(self):
    self.assertEquals('Bar', StripNamespace('foo.Bar'))
    self.assertEquals('Baz', StripNamespace('Baz'))

  def testJsFunctionNameToClassName(self):
    self.assertEquals('FooBar', JsFunctionNameToClassName('foo', 'bar'))
    self.assertEquals('FooBar',
                      JsFunctionNameToClassName('experimental.foo', 'bar'))
    self.assertEquals('FooBarBaz',
                      JsFunctionNameToClassName('foo.bar', 'baz'))
    self.assertEquals('FooBarBaz',
                      JsFunctionNameToClassName('experimental.foo.bar', 'baz'))

if __name__ == '__main__':
  unittest.main()
