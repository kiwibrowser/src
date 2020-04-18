# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test cros_generate_android_breakpad_symbols."""

from __future__ import print_function

import tempfile

from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import osutils
from chromite.scripts import cros_generate_android_breakpad_symbols


# Unitests often need access to internals of the thing they test.
# pylint: disable=protected-access

class AdjustOffsetsTest(cros_test_lib.TestCase):
  """Test breakpad symbol file offset adjustments."""

  def testFinaExpansionOffset(self):
    """Make sure we get the correct offset."""
    output = """
INFO: Relocations   : RELA
INFO: Packed           : 9544 bytes
INFO: Unpacked         : 66888 bytes
INFO: Relocations      : 2787 entries
INFO: Expansion     : 57344 bytes
"""
    cmd_result = cros_build_lib.CommandResult(
        cmd=None, error=None, output=output, returncode=0)

    result = cros_generate_android_breakpad_symbols.FindExpansionOffset(
        cmd_result)
    self.assertEqual(result, -57344)

  def testFinaExpansionOffsetNoUnpack(self):
    """Make sure we get an offset of zero, if the file wasn't unpacked."""

    cmd_result = cros_build_lib.CommandResult(
        cmd=None, error=None, output=None, returncode=1)

    result = cros_generate_android_breakpad_symbols.FindExpansionOffset(
        cmd_result)
    self.assertEqual(result, 0)

  def testFinaExpansionOffsetBadOutput(self):
    """Make sure we get an error without expected output."""

    cmd_result = cros_build_lib.CommandResult(
        cmd=None, error=None, output='foo', returncode=0)

    with self.assertRaises(
        cros_generate_android_breakpad_symbols.OffsetDiscoveryError):
      cros_generate_android_breakpad_symbols.FindExpansionOffset(cmd_result)

  def testAdjustLineSymbolOffset(self):
    """Test _AdjustLineSymbolOffset."""

    offset = 42
    line_expected = (
        ('', ''),
        ('\n', '\n'),
        ('noise\n', 'noise\n'),
        ('MODULE Linux arm64 E3D562057466309CED960047D474EBF00 libssl.so\n',
         'MODULE Linux arm64 E3D562057466309CED960047D474EBF00 libssl.so\n'),
        ('FUNC dcfc a8 0 dtls1_discard_fragment_body\n',
         'FUNC dd26 a8 0 dtls1_discard_fragment_body\n'),
        ('dd18 c 403 1\n',
         'dd42 c 403 1\n'),
        ('PUBLIC f9b0 0 DTLSv1_get_timeout\n',
         'PUBLIC f9da 0 DTLSv1_get_timeout\n'),
        ('STACK CFI INIT dcfc a8 .cfa: sp 0 + .ra: x30\n',
         'STACK CFI INIT dd26 a8 .cfa: sp 0 + .ra: x30\n'),
        ('STACK CFI dd04 .cfa: x29 336 +\n',
         'STACK CFI dd2e .cfa: x29 336 +\n'),
        ('0 c 403 1\n',
         '0 c 403 1\n'),
        ('FUNC 0 a8 0 dtls1_discard_fragment_body\n',
         'FUNC 0 a8 0 dtls1_discard_fragment_body\n'),
    )

    for line, expected in line_expected:
      result = cros_generate_android_breakpad_symbols._AdjustLineSymbolOffset(
          line, offset)
      self.assertEqual(result, expected)

  def testAdjustSymbolOffsetEmpty(self):
    """Test ability to adjust an empty file."""
    with tempfile.NamedTemporaryFile() as sym_file:
      osutils.WriteFile(sym_file.name, '')
      cros_generate_android_breakpad_symbols._AdjustSymbolOffset(
          sym_file.name, 42)
      self.assertEqual(osutils.ReadFile(sym_file.name), '')

  def testAdjustSymbolOffset(self):
    """Test ability to adjust an empty file."""
    unadjusted = """
MODULE Linux arm64 E3D562057466309CED960047D474EBF00 libssl.so
FILE 0 /testfile.h
FILE 1 /usr/testfile.c
dcfc 1c 403 1
dd18 c 403 1
dd30 10 405 1
dd4c 14 407 1
2d65c 8 1218 27
2d664 c 1226 27
FUNC dda4 140 0 dtls1_hm_fragment_new
dda4 10 151 1
PUBLIC 2c474 0 SSL_early_callback_ctx_extension_get
PUBLIC 2f0bc 0 SSL_get_shared_sigalgs
STACK CFI INIT dcfc a8 .cfa: sp 0 + .ra: x30
STACK CFI dd00 .cfa: sp 336 + .ra: .cfa -328 + ^ x29: .cfa -336 + ^
STACK CFI dd04 .cfa: x29 336 +
STACK CFI dd08 x23: .cfa -288 + ^ x24: .cfa -280 + ^
"""
    expected = """
MODULE Linux arm64 E3D562057466309CED960047D474EBF00 libssl.so
FILE 0 /testfile.h
FILE 1 /usr/testfile.c
dd26 1c 403 1
dd42 c 403 1
dd5a 10 405 1
dd76 14 407 1
2d686 8 1218 27
2d68e c 1226 27
FUNC ddce 140 0 dtls1_hm_fragment_new
ddce 10 151 1
PUBLIC 2c49e 0 SSL_early_callback_ctx_extension_get
PUBLIC 2f0e6 0 SSL_get_shared_sigalgs
STACK CFI INIT dd26 a8 .cfa: sp 0 + .ra: x30
STACK CFI dd2a .cfa: sp 336 + .ra: .cfa -328 + ^ x29: .cfa -336 + ^
STACK CFI dd2e .cfa: x29 336 +
STACK CFI dd32 x23: .cfa -288 + ^ x24: .cfa -280 + ^
"""
    with tempfile.NamedTemporaryFile() as sym_file:
      osutils.WriteFile(sym_file.name, unadjusted)
      cros_generate_android_breakpad_symbols._AdjustSymbolOffset(
          sym_file.name, 42)
      after = osutils.ReadFile(sym_file.name)
      self.assertEqual(after, expected)
