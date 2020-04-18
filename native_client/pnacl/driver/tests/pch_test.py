#!/usr/bin/python
# Copyright 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import shutil
import sys
import tempfile
import unittest

from driver_env import env
import driver_test_utils
import driver_tools
import filetype

class TestPrecompiledHeaders(driver_test_utils.DriverTesterCommon):
  def setUp(self):
    super(TestPrecompiledHeaders, self).setUp()
    driver_test_utils.ApplyTestEnvOverrides(env)
    self.exit_backup = sys.exit
    sys.exit = driver_test_utils.FakeExit
    self.temp_dir = tempfile.mkdtemp()

  def tearDown(self):
    # Reset some internal state.
    sys.exit = self.exit_backup
    filetype.ClearFileTypeCaches()
    filetype.SetForcedFileType(None)
    shutil.rmtree(self.temp_dir)
    super(TestPrecompiledHeaders, self).tearDown()

  def runDriver(self, driver, flags):
    driver_tools.RunDriver(driver, flags)
    # Reset some internal state.
    filetype.ClearFileTypeCaches()
    filetype.SetForcedFileType(None)

  def getFirstTestHeader(self, suffix='.h'):
    filename = os.path.join(self.temp_dir, 'test_foo' + suffix)
    with open(filename, 'w') as t:
      t.write('''
#ifndef _PNACL_DRIVER_TESTS_TEST_HEADER_H_
#define _PNACL_DRIVER_TESTS_TEST_HEADER_H_
#ifdef __cplusplus
void foo_cxx(int x, int y);
#else
void bar_c(int x);
#endif
#endif''')
      return t

  def getSecondTestHeader(self, first_header, suffix='.h'):
    filename = os.path.join(self.temp_dir, 'test_bar' + suffix)
    with open(filename, 'w') as t:
      t.write('''
    #ifdef __cplusplus
    #include "{header}"
    #else
    #include "{header}"
    #endif
'''.format(header=first_header))
      return t

  def getTestSource(self, header_name, suffix='.c'):
    filename = os.path.join(self.temp_dir, 'test_source' + suffix)
    with open(filename, 'w') as t:
      t.write('''#include <%s>
#ifdef __cplusplus
void cxx_func() {
  foo_cxx(1, 2);
}
#else
void c_func() {
  bar_c(1);
}
#endif''' % header_name)
      return t

  def compileAndCheck(self, driver, header, source, is_cxx):
    source_base, _ = os.path.splitext(source)
    out = source_base + '.ll'
    self.runDriver(driver,
            ['-include', header, '-S', source, '-o', out])
    self.checkCompiledAs(out, is_cxx=is_cxx)

  def checkCompiledAs(self, out, is_cxx):
    with open(out, 'r') as t:
      if is_cxx:
        self.assertTrue('foo_cxx' in t.read())
      else:
        self.assertTrue('bar_c' in t.read())

  #---- Actual tests -----#

  def test_one_with_xlang(self):
    """Test compiling one PCH file with an explicit -x, but no -o"""
    h = self.getFirstTestHeader(suffix='.h')
    h2 = self.getSecondTestHeader(h.name, suffix='.h')
    self.runDriver('pnacl-clang',
            ['-x', 'c-header', '-I', self.temp_dir, h2.name])
    s = self.getTestSource(h2.name)
    self.compileAndCheck('pnacl-clang', h2.name, s.name, is_cxx=False)

  def test_one_override_output(self):
    """Test compiling one PCH file with an explicit -o <output>"""
    h = self.getFirstTestHeader(suffix='.h')
    h2 = self.getSecondTestHeader(h.name, suffix='.h')
    h2_out = h2.name + 'fancy.pch'
    self.assertFalse(os.path.exists(h2_out))
    self.runDriver('pnacl-clang',
            ['-x', 'c-header', '-I', self.temp_dir, h2.name,
             '-o', h2_out])
    self.assertTrue(os.path.exists(h2_out))
    s = self.getTestSource(h2.name)
    self.compileAndCheck('pnacl-clang', h2.name, s.name, is_cxx=False)

  def test_two_with_xlang(self):
    """Test compiling two PCH files with an explicit -x"""
    # Specifying multiple header files without something like -c, or -S
    # should be fine. It should not attempt to compile and link like what
    # would happen if there were .c or .cpp files without -c, or -S.
    h = self.getFirstTestHeader(suffix='.h')
    h2 = self.getSecondTestHeader(h.name, suffix='.h')
    self.runDriver('pnacl-clang',
            ['-x', 'c-header', '-I', self.temp_dir, h.name, h2.name])
    self.assertTrue(os.path.exists(driver_tools.DefaultPCHOutputName(h.name)))
    self.assertTrue(os.path.exists(driver_tools.DefaultPCHOutputName(h2.name)))
    s = self.getTestSource(h2.name)
    self.compileAndCheck('pnacl-clang', h2.name, s.name, is_cxx=False)

  def test_ignored_dash_mode(self):
    """Test that compiling PCH (essentially) ignores -c, -S, but not -E"""
    h = self.getFirstTestHeader(suffix='.h')
    self.runDriver('pnacl-clang',
            ['-c', '-x', 'c-header', '-I', self.temp_dir, h.name])
    h2 = self.getSecondTestHeader(h.name, suffix='.h')
    self.runDriver('pnacl-clang',
            ['-S', '-x', 'c-header', '-I', self.temp_dir, h2.name])
    s = self.getTestSource(h2.name)
    self.compileAndCheck('pnacl-clang', h2.name, s.name, is_cxx=False)

  def test_not_ignore_dash_E(self):
    """Test that compiling PCH does not ignore -E"""
    h = self.getFirstTestHeader(suffix='.h')
    self.runDriver('pnacl-clang',
            ['-E', '-x', 'c-header', '-I', self.temp_dir, h.name])
    self.assertFalse(os.path.exists(driver_tools.DefaultPCHOutputName(h.name)))

  def test_two_no_xlang(self):
    """Test compiling two PCH files without a -x (guess from suffix/driver)"""
    h = self.getFirstTestHeader(suffix='.h')
    h2 = self.getSecondTestHeader(h.name, suffix='.h')
    self.runDriver('pnacl-clang',
            ['-I', self.temp_dir, h.name, h2.name])
    s = self.getTestSource(h2.name)
    self.compileAndCheck('pnacl-clang', h2.name, s.name, is_cxx=False)

  def test_hpp_no_xlang_clangxx(self):
    """Test what happens when using clang++ without a -x <...> / .hpp file"""
    h = self.getFirstTestHeader(suffix='.hpp')
    h2 = self.getSecondTestHeader(h.name, suffix='.hpp')
    self.runDriver('pnacl-clang++',
            ['-I', self.temp_dir, h2.name])
    s = self.getTestSource(h2.name, suffix='.cc')
    self.compileAndCheck('pnacl-clang++', h2.name, s.name, is_cxx=True)

  def test_h_clangxx(self):
    """Test what happens when using clang++ but -x c++-header and .h file"""
    # We cannot test .h with clang++ and without a -x c++-header.
    # clang will actually say: "warning: treating 'c-header' input as
    # 'c++-header' when in C++ mode, this behavior is deprecated."
    # The way the pnacl drivers intercept -x and cache filetypes also
    # makes that difficult to distinguish between the -x c-header case and the
    # non-x-case.
    h = self.getFirstTestHeader(suffix='.h')
    h2 = self.getSecondTestHeader(h.name, suffix='.h')
    self.runDriver('pnacl-clang++',
            ['-x', 'c++-header', '-I', self.temp_dir, h2.name])
    s = self.getTestSource(h2.name, suffix='.cc')
    self.compileAndCheck('pnacl-clang++', h2.name, s.name, is_cxx=True)

  def test_hpp_no_xlang_clang(self):
    """Test what happens when using clang with no -x <...> / .hpp file"""
    h = self.getFirstTestHeader(suffix='.hpp')
    h2 = self.getSecondTestHeader(h.name, suffix='.hpp')
    self.runDriver('pnacl-clang',
            ['-I', self.temp_dir, h2.name])
    s = self.getTestSource(h2.name, suffix='.cc')
    self.compileAndCheck('pnacl-clang', h2.name, s.name, is_cxx=True)

  def test_mixed_headers_other(self):
    """Test that we throw an error when trying to mix doing PCH w/ compiling"""
    h = self.getFirstTestHeader(suffix='.hpp')
    h2 = self.getSecondTestHeader(h.name, suffix='.hpp')
    s = self.getTestSource(h2.name, suffix='.cc')
    self.assertRaises(driver_test_utils.DriverExitException,
            self.runDriver,
            'pnacl-clang++',
            ['-I', self.temp_dir, h2.name, '-c', s.name])

  def test_relocatable_header(self):
    """Tests that the driver accepts --relocatable-header."""
    h = self.getFirstTestHeader(suffix='.h')
    h2 = self.getSecondTestHeader(h.name, suffix='.h')
    driver_tools.RunDriver('pnacl-clang',
            ['-x', 'c-header', '--relocatable-pch',
             '-isysroot', self.temp_dir, h2.name])
