#!/usr/bin/python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests pnacl-compress.

This tests that pnacl-compress fixpoints, and is immutable after that.
"""

from driver_env import env
import driver_test_utils
import driver_tools
import pathtools

class TestPnaclCompress(driver_test_utils.DriverTesterCommon):
  def setUp(self):
    super(TestPnaclCompress, self).setUp()
    driver_test_utils.ApplyTestEnvOverrides(env)
    self.platform = driver_test_utils.GetPlatformToTest()

  def getFakePexe(self):
    with self.getTemp(suffix='.ll', close=False) as t:
      with self.getTemp(suffix='.pexe') as p:
        t.write('''
define i32 @foo() {
  %1 = add i32 1, 1
  %2 = add i32 %1, %1
  %3 = add i32 %2, %2
  %4 = add i32 %3, %3
  %5 = add i32 %4, %4
  %6 = add i32 %5, %5
  %7 = add i32 %6, %6
  %8 = add i32 %7, %7
  %9 = add i32 %8, %8
  %10 = add i32 %9, %9
  %11 = add i32 %10, %10
  %12 = add i32 %11, %11
  %13 = add i32 %12, %12
  %14 = add i32 %13, %13
  %15 = add i32 %14, %14
  %16 = add i32 %15, %15
  ret i32 %16
}

define i32 @main() {
  %1 = add i32 1, 1
  %2 = add i32 %1, %1
  %3 = add i32 %2, %2
  %4 = add i32 %3, %3
  %5 = add i32 %4, %4
  %6 = add i32 %5, %5
  %7 = add i32 %6, %6
  %8 = add i32 %7, %7
  %9 = add i32 %8, %8
  %10 = add i32 %9, %9
  %11 = add i32 %10, %10
  %12 = add i32 %11, %11
  %13 = add i32 %12, %12
  %14 = add i32 %13, %13
  %15 = add i32 %14, %14
  %16 = add i32 %15, %15
  %17 = call i32 @foo()
  %18 = add i32 %16, %17
  ret i32 %18
}
''')
        t.close()
        driver_tools.RunDriver('pnacl-as', [t.name, '-o', p.name])
        driver_tools.RunDriver('pnacl-finalize', [p.name])
        return p

  def test_multiple_compresses(self):
    pexe = self.getFakePexe()
    init_size = pathtools.getsize(pexe.name)
    driver_tools.RunDriver('pnacl-compress', [pexe.name])
    shrunk_size = pathtools.getsize(pexe.name)
    self.assertTrue(init_size >= shrunk_size)
    driver_tools.RunDriver('pnacl-compress', [pexe.name])
    self.assertTrue(pathtools.getsize(pexe.name) == shrunk_size)
