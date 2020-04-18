#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re
import unittest

from driver_env import env
import driver_test_utils
import driver_tools
import filetype


class TestFiletypeCache(driver_test_utils.DriverTesterCommon):
  def setUp(self):
    super(TestFiletypeCache, self).setUp()
    driver_test_utils.ApplyTestEnvOverrides(env)

  def getFakeLLAndBitcodeFile(self):
    with self.getTemp(suffix='.ll', close=False) as t:
      t.write('''
define i32 @main() {
  ret i32 0
}
''')
      t.close()
      with self.getTemp(suffix='.bc') as b:
        driver_tools.RunDriver('pnacl-as', [t.name, '-o', b.name])
        return t, b

  def test_ll_bc_filetypes(self):
    if not driver_test_utils.CanRunHost():
      return
    ll, bc = self.getFakeLLAndBitcodeFile()
    self.assertTrue(filetype.FileType(ll.name) == 'll')
    self.assertTrue(filetype.FileType(bc.name) == 'po')
    self.assertFalse(filetype.IsLLVMBitcode(ll.name))
    self.assertTrue(filetype.IsLLVMBitcode(bc.name))

  def test_inplace_finalize(self):
    if not driver_test_utils.CanRunHost():
      return
    ll, bc = self.getFakeLLAndBitcodeFile()
    self.assertTrue(filetype.FileType(bc.name) == 'po')
    self.assertTrue(filetype.IsLLVMBitcode(bc.name))
    self.assertFalse(filetype.IsPNaClBitcode(bc.name))
    driver_tools.RunDriver('pnacl-finalize', [bc.name])
    self.assertFalse(filetype.IsLLVMBitcode(bc.name))
    self.assertTrue(filetype.IsPNaClBitcode(bc.name))
    self.assertTrue(filetype.FileType(bc.name) == 'pexe')

  def getBCWithDebug(self):
    with self.getTemp(suffix='.c', close=False) as t:
      t.write('''
int __attribute__((noinline)) baz(int x, int y) {
  return x + y;
}

int foo(int a, int b) {
  return baz(a, b);
}
''')
      t.close()
      with self.getTemp(suffix='.bc') as b:
        # Compile w/ optimization to avoid allocas from local
        # variables/parameters, since this isn't running the ABI
        # simplification passes.
        driver_tools.RunDriver(
            'pnacl-clang', [t.name, '-o', b.name, '-c', '-g', '-O1'])
        return b

  def test_finalize_keep_syms(self):
    """Test that finalize is still able to create a pexe w/ -no-strip-syms."""
    if not driver_test_utils.CanRunHost():
      return
    bc = self.getBCWithDebug()
    self.assertTrue(filetype.FileType(bc.name) == 'po')
    self.assertTrue(filetype.IsLLVMBitcode(bc.name))
    self.assertFalse(filetype.IsPNaClBitcode(bc.name))
    driver_tools.RunDriver('pnacl-finalize', [bc.name, '--no-strip-syms'])
    self.assertFalse(filetype.IsLLVMBitcode(bc.name))
    self.assertTrue(filetype.IsPNaClBitcode(bc.name))
    self.assertTrue(filetype.FileType(bc.name) == 'pexe')
    # Use pnacl-dis instead of llvm-nm, since llvm-nm won't know how to
    # handle finalized bitcode for now:
    # https://code.google.com/p/nativeclient/issues/detail?id=3993
    with self.getTemp(suffix='.ll') as temp_ll:
      driver_tools.RunDriver(
        'pnacl-dis', [bc.name, '-o', temp_ll.name])
      with open(temp_ll.name, 'r') as dis_file:
        file_contents = dis_file.read()
        self.assertTrue(re.search(r'define .*@baz', file_contents))
        self.assertTrue(re.search(r'define .*@foo', file_contents))
