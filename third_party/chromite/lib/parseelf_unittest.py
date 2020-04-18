# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for the parseelf.py module."""

from __future__ import print_function

import os

from chromite.lib import cros_test_lib
from chromite.lib import osutils
from chromite.lib import parseelf
from chromite.lib import unittest_lib


class ELFParsingTest(cros_test_lib.TempDirTestCase):
  """Test the ELF parsing functions."""

  _ldpaths = {'interp': [], 'env': [], 'conf': []}

  def testIsLib(self):
    """Tests the 'is_lib' attribute is inferred correctly for libs."""
    unittest_lib.BuildELF(os.path.join(self.tempdir, 'liba.so'), ['func_a'])
    elf = parseelf.ParseELF(self.tempdir, 'liba.so', self._ldpaths)
    self.assertTrue('is_lib' in elf)
    self.assertTrue(elf['is_lib'])

  def testNotIsLib(self):
    """Tests the 'is_lib' attribute is inferred correctly for executables."""
    unittest_lib.BuildELF(os.path.join(self.tempdir, 'abc_main'),
                          executable=True)
    elf = parseelf.ParseELF(self.tempdir, 'abc_main', self._ldpaths)
    self.assertTrue('is_lib' in elf)
    self.assertFalse(elf['is_lib'])

  def testUnsupportedFiles(self):
    """Tests unsupported files are ignored."""
    osutils.WriteFile(os.path.join(self.tempdir, 'foo.so'), 'foo')
    self.assertEquals(None,
                      parseelf.ParseELF(self.tempdir, 'foo.so', self._ldpaths))

    osutils.WriteFile(os.path.join(self.tempdir, 'foo.so'), '\x7fELF-foo')
    self.assertEquals(None,
                      parseelf.ParseELF(self.tempdir, 'foo.so', self._ldpaths))

  def testParsedSymbols(self):
    """Tests the list of imported/exported symbols."""
    unittest_lib.BuildELF(os.path.join(self.tempdir, 'libabc.so'),
                          defined_symbols=['fa', 'fb', 'fc'])
    unittest_lib.BuildELF(os.path.join(self.tempdir, 'libxyz.so'),
                          defined_symbols=['fx', 'fy', 'fz'],
                          undefined_symbols=['fa', 'fb', 'fc'],
                          used_libs=['abc'])

    # Test without symbols.
    elf = parseelf.ParseELF(self.tempdir, 'libxyz.so', self._ldpaths,
                            parse_symbols=False)
    self.assertFalse('imp_sym' in elf)
    self.assertFalse('exp_sym' in elf)

    # Test with symbols by default.
    elf = parseelf.ParseELF(self.tempdir, 'libxyz.so', self._ldpaths,
                            parse_symbols=True)
    self.assertTrue('imp_sym' in elf)
    self.assertTrue('exp_sym' in elf)
    self.assertEquals(elf['imp_sym'], set(['fa', 'fb', 'fc']))
    self.assertIn('fx', elf['exp_sym'])
    self.assertIn('fy', elf['exp_sym'])
    self.assertIn('fz', elf['exp_sym'])

  def testLibDependencies(self):
    """Tests the list direct dependencies."""
    # Dependencies:
    #   u -> abc
    #   v -> abc
    #   prog -> u,v
    unittest_lib.BuildELF(os.path.join(self.tempdir, 'libabc.so'),
                          defined_symbols=['fa', 'fb', 'fc'])
    unittest_lib.BuildELF(os.path.join(self.tempdir, 'libu.so'),
                          defined_symbols=['fu'],
                          undefined_symbols=['fa'],
                          used_libs=['abc'])
    unittest_lib.BuildELF(os.path.join(self.tempdir, 'libv.so'),
                          defined_symbols=['fv'],
                          undefined_symbols=['fb'],
                          used_libs=['abc'])
    unittest_lib.BuildELF(os.path.join(self.tempdir, 'prog'),
                          undefined_symbols=['fu', 'fv'],
                          used_libs=['u', 'v'],
                          executable=True)

    elf_prog = parseelf.ParseELF(self.tempdir, 'prog', self._ldpaths)
    # Check the direct dependencies.
    self.assertTrue('libu.so' in elf_prog['needed'])
    self.assertTrue('libv.so' in elf_prog['needed'])
    self.assertFalse('libabc.so' in elf_prog['needed'])

  def testRelativeLibPaths(self):
    """Test that the paths reported by ParseELF are relative to root."""
    unittest_lib.BuildELF(os.path.join(self.tempdir, 'liba.so'), ['fa'])
    unittest_lib.BuildELF(os.path.join(self.tempdir, 'prog'),
                          undefined_symbols=['fa'], used_libs=['a'],
                          executable=True)
    elf = parseelf.ParseELF(self.tempdir, 'prog', self._ldpaths)
    for lib in elf['libs'].values():
      for path in ('realpath', 'path'):
        if lib[path] is None:
          continue
        self.assertFalse(lib[path].startswith('/'))
        self.assertFalse(lib[path].startswith(self.tempdir))
        # Linked lib paths should be relative to the working directory or is the
        # ld dynamic loader.
        self.assertTrue(lib[path] == elf['interp'] or
                        os.path.exists(os.path.join(self.tempdir, lib[path])))
