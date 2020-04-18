#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests of the pnacl driver.

This tests that strip works on various kinds of files.
"""


from driver_env import env
import driver_log
import driver_test_utils
import driver_tools

import os
import tempfile
import unittest

class TestStrip(driver_test_utils.DriverTesterCommon):

  def setUp(self):
    super(TestStrip, self).setUp()
    driver_test_utils.ApplyTestEnvOverrides(env)

  def getSource(self, num):
    with self.getTemp(suffix='.c', close=False) as t:
      t.write('''
extern void puts(const char *);

void foo%d(void) {
  puts("Hello\\n");
}
''' % num)
      t.close()
      return t

  def generateObjWithDebug(self, src, is_native):
    s = '.bc'
    if is_native:
      s = '.o'
    obj = self.getTemp(suffix=s)
    args = ['-c', '-g', src.name, '-o', obj.name]
    if is_native:
      args += ['-arch', 'x86-32', '--pnacl-allow-translate']
    driver_tools.RunDriver('pnacl-clang', args)
    return obj

  def generateArchive(self, objs):
    a = self.getTemp(suffix='.a')
    # Archive file must be a valid archive (non-empty), or non-existent,
    # so remove it before running.
    os.remove(a.name)
    obj_names = [ obj.name for obj in objs]
    args = ['rcs', a.name] + obj_names
    driver_tools.RunDriver('pnacl-ar', args)
    return a

  def getFileSize(self, f):
    s = os.stat(f)
    return s.st_size

  def stripFileAndCheck(self, f):
    f_stripped = self.getTemp()
    driver_tools.RunDriver('pnacl-strip',
        ['--strip-all', f.name, '-o', f_stripped.name])
    self.assertTrue(self.getFileSize(f_stripped.name) <
                    self.getFileSize(f.name))
    driver_tools.RunDriver('pnacl-strip',
        ['--strip-debug', f.name, '-o', f_stripped.name])
    self.assertTrue(self.getFileSize(f_stripped.name) <
                    self.getFileSize(f.name))
    driver_tools.RunDriver('pnacl-strip',
        ['--strip-unneeded', f.name, '-o', f_stripped.name])
    self.assertTrue(self.getFileSize(f_stripped.name) <
                    self.getFileSize(f.name))

  # Individual tests.
  # NOTE: We do not test the bitcode archive case.
  # Bitcode archives cannot be stripped, because the strip tool does
  # not understand how to do that. We cannot test this e.g., with
  # assertRaises() because DriverExit() is called on an error
  # instead of raising an exception.

  def test_StripBitcodeObj(self):
    if driver_test_utils.CanRunHost():
      src = self.getSource(0)
      obj = self.generateObjWithDebug(src, is_native=False)
      self.stripFileAndCheck(obj)

  def test_StripNativeObj(self):
    if driver_test_utils.CanRunHost():
      src = self.getSource(0)
      obj = self.generateObjWithDebug(src, is_native=True)
      self.stripFileAndCheck(obj)

  def test_StripNativeArchive(self):
    if driver_test_utils.CanRunHost():
      src = self.getSource(0)
      obj = self.generateObjWithDebug(src, is_native=True)
      ar = self.generateArchive([obj])
      self.stripFileAndCheck(ar)
      src2 = self.getSource(1)
      obj2 = self.generateObjWithDebug(src2, is_native=True)
      ar2 = self.generateArchive([obj, obj2])
      self.stripFileAndCheck(ar2)

if __name__ == '__main__':
  unittest.main()
