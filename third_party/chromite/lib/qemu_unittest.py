# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test for chromite qemu logic"""

from __future__ import print_function

import glob
import os

from chromite.lib import cros_test_lib
from chromite.lib import qemu


class QemuTests(cros_test_lib.TestCase):
  """Verify Qemu logic works"""

  def testArchDetect(self):
    """Verify we correctly probe each arch"""
    test_dir = os.path.join(os.path.realpath(os.path.dirname(__file__)),
                            'datafiles')
    test_files = os.path.join(test_dir, 'arch.*.elf')

    for test in glob.glob(test_files):
      test_file = os.path.basename(test)
      exp_arch = test_file.split('.')[1]

      arch = qemu.Qemu.DetectArch(test_file, test_dir)
      if arch is None:
        # See if we have a mask for it.
        # pylint: disable=W0212
        self.assertNotIn(exp_arch, qemu.Qemu._MAGIC_MASK.keys(),
                         msg='ELF "%s" did not match "%s", but should have' %
                         (test, exp_arch))
      else:
        self.assertEqual(arch, exp_arch)

  def testRegisterStr(self):
    """Verify the binfmt register string doesn't exceed kernel limits"""
    # pylint: disable=W0212
    for arch in qemu.Qemu._MAGIC_MASK.keys():
      name = 'qemu-%s' % arch
      interp = '/build/bin/%s' % name
      register = qemu.Qemu.GetRegisterBinfmtStr(arch, name, interp)
      self.assertGreaterEqual(256, len(register),
                              msg='arch "%s" has too long of a register string:'
                                  ' %i: %r' % (arch, len(register), register))
