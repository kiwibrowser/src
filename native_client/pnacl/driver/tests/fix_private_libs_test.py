#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

""" Test that "FixPrivateLibs" function rearranges public and private
libraries in the right way. """

import unittest

from driver_env import env
import driver_test_utils
pnacl_ld = __import__("pnacl-ld")


class TestFixPrivateLibs(driver_test_utils.DriverTesterCommon):
  def setUp(self):
    super(TestFixPrivateLibs, self).setUp()
    driver_test_utils.ApplyTestEnvOverrides(env)
    env.set('USE_IRT', '1')

  def test_nop(self):
    """Test having no private libs at all -- nothing should happen.

    NOTE: The "user_libs" parameter can have flags as well as files
    and libraries. Basically it's just the commandline for the bitcode linker.
    Test that the flags and normal files aren't disturbed.
    """
    input_libs = ['--undefined=main',
                  'toolchain/linux_x86/pnacl_newlib_raw/lib/crti.bc',
                  'foo.bc',
                  '--start-group',
                  '-lnacl',
                  '-lc',
                  '--end-group']
    expected_libs = list(input_libs)
    output_libs = pnacl_ld.FixPrivateLibs(input_libs)
    self.assertEqual(expected_libs, output_libs)

  def test_pthread_no_nacl(self):
    """Test having pthread_private but not nacl_sys_private.

    pthread_private should replace pthread. However we still keep the
    original instance of pthread_private where it is, which shouldn't hurt.

    Also test that nacl stays the same, if nacl_sys_private was never
    requested by the user. We can't pull in nacl_sys_private spuriously,
    because chrome never builds nacl_sys_private w/ gyp.
    """
    input_libs = ['foo.bc',
                  '-lpthread_private',
                  '--start-group',
                  '-lpthread',
                  '-lnacl',
                  '-lc',
                  '--end-group']
    expected_libs = ['foo.bc',
                     '-lpthread_private',
                     '--start-group',
                     '-lpthread_private',
                     '-lnacl_sys_private',
                     '-lnacl',
                     '-lc',
                     '--end-group']
    output_libs = pnacl_ld.FixPrivateLibs(input_libs)
    self.assertEqual(expected_libs, output_libs)

  def test_nacl_private_no_pthread(self):
    """Test having nacl_sys_private but not pthread_private.

    Since pthread is added as an implicit lib for libc++, for non-IRT
    programs we need to swap pthread with pthread_private even if it
    wasn't explicitly asked for. Otherwise pthread will try to query the
    IRT and crash. Use nacl_sys_private as the signal for non-IRT
    programs.
    """
    input_libs = ['foo.bc',
                  '-lnacl_sys_private',
                  '--start-group',
                  '-lpthread',
                  '-lnacl',
                  '-lc',
                  '--end-group']
    expected_libs = ['foo.bc',
                     '-lnacl_sys_private',
                     '--start-group',
                     '-lpthread_private',
                     '-lnacl_sys_private',
                     '-lnacl',
                     '-lc',
                     '--end-group']
    output_libs = pnacl_ld.FixPrivateLibs(input_libs)
    self.assertEqual(expected_libs, output_libs)


  def test_pthread_and_nacl_1(self):
    """Test having both nacl_sys_private and pthread_private #1.

    In this case, yes we should touch nacl_sys_private.
    """
    input_libs = ['foo.bc',
                  '-lnacl_sys_private',
                  '-lpthread_private',
                  '--start-group',
                  '-lpthread',
                  '-lnacl',
                  '-lc',
                  '--end-group']
    expected_libs = ['foo.bc',
                     '-lnacl_sys_private',
                     '-lpthread_private',
                     '--start-group',
                     '-lpthread_private',
                     '-lnacl_sys_private',
                     '-lnacl',
                     '-lc',
                     '--end-group']
    output_libs = pnacl_ld.FixPrivateLibs(input_libs)
    self.assertEqual(expected_libs, output_libs)

  def test_pthread_and_nacl_2(self):
    """Test having both nacl_sys_private and pthread_private #2.

    Try flipping the order of nacl and pthread to make sure the substitution
    still works.
    """
    input_libs = ['foo.bc',
                  '-lpthread_private',
                  '-lnacl_sys_private',
                  '--start-group',
                  '-lnacl',
                  '-lpthread',
                  '-lc',
                  '--end-group']
    expected_libs = ['foo.bc',
                     '-lpthread_private',
                     '-lnacl_sys_private',
                     '--start-group',
                     '-lnacl_sys_private',
                     '-lnacl',
                     '-lpthread_private',
                     '-lc',
                     '--end-group']
    output_libs = pnacl_ld.FixPrivateLibs(input_libs)
    self.assertEqual(expected_libs, output_libs)
