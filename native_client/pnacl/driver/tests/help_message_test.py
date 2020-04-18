#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests of the pnacl driver.

This tests that help messages are available in all of the drivers.
"""

from driver_env import env
import driver_test_utils
import driver_tools

pnacl_ar = __import__("pnacl-ar")
pnacl_as = __import__("pnacl-as")
pnacl_clang = __import__("pnacl-clang")
pnacl_dis = __import__("pnacl-dis")
pnacl_ld = __import__("pnacl-ld")
pnacl_opt = __import__("pnacl-opt")
pnacl_nm = __import__("pnacl-nm")
pnacl_ranlib = __import__("pnacl-ranlib")
pnacl_strip = __import__("pnacl-strip")
pnacl_translate = __import__("pnacl-translate")

import unittest


class TestHelpMessages(unittest.TestCase):

  def setUp(self):
    driver_test_utils.ApplyTestEnvOverrides(env)
    # get_help() currently expects an argv, even though most of the time
    # it doesn't do anything.  Sometimes it looks at argv to see if the
    # argv is "--help" or "--help-hidden" / "--help-full", but perhaps
    # that could be abstracted as a verbosity level at some point.
    self.default_argv = ['driver', '--help']

  def test_ARHelpMessage(self):
    # Some of the help messages use the 'SCRIPT_NAME' env var, so set that.
    # We probably don't want to depend on this global env across tests,
    # but that's what we have now and it's convenient.
    env.set('SCRIPT_NAME', 'pnacl-ar')
    help_str = pnacl_ar.get_help(self.default_argv)
    # Ideally we would be able to test the content better, but for now
    # this at least tests that no exception is raised, etc.
    self.assertNotEqual(help_str, driver_tools.HelpNotAvailable())
    # Use assertTrue x > 0 instead of assertGreater(x, 0) since some of
    # our bots don't have python 2.7.
    self.assertTrue(len(help_str) > 0)

  def test_ASHelpMessage(self):
    env.set('SCRIPT_NAME', 'pnacl-as')
    help_str = pnacl_as.get_help(self.default_argv)
    self.assertNotEqual(help_str, driver_tools.HelpNotAvailable())
    self.assertTrue(len(help_str) > 0)

  def test_ClangHelpMessage(self):
    env.set('SCRIPT_NAME', 'pnacl-clang')
    help_str = pnacl_clang.get_help(self.default_argv)
    self.assertNotEqual(help_str, driver_tools.HelpNotAvailable())
    self.assertTrue(len(help_str) > 0)
    # Clang also has a '--help-full'.
    if driver_test_utils.CanRunHost():
      help_full_str = pnacl_clang.get_help(['driver', '--help-full'])
      self.assertNotEqual(help_full_str, driver_tools.HelpNotAvailable())
      self.assertTrue(len(help_full_str) > 0)

  def test_DisHelpMessage(self):
    env.set('SCRIPT_NAME', 'pnacl-dis')
    help_str = pnacl_dis.get_help(self.default_argv)
    self.assertNotEqual(help_str, driver_tools.HelpNotAvailable())
    self.assertTrue(len(help_str) > 0)

  def test_BCLDHelpMessage(self):
    env.set('SCRIPT_NAME', 'pnacl-ld')
    help_str = pnacl_ld.get_help(self.default_argv)
    self.assertNotEqual(help_str, driver_tools.HelpNotAvailable())
    self.assertTrue(len(help_str) > 0)

  def test_OptHelpMessage(self):
    if driver_test_utils.CanRunHost():
      env.set('SCRIPT_NAME', 'pnacl-opt')
      help_str = pnacl_opt.get_help(self.default_argv)
      self.assertNotEqual(help_str, driver_tools.HelpNotAvailable())
      self.assertTrue(len(help_str) > 0)

  def test_NMHelpMessage(self):
    env.set('SCRIPT_NAME', 'pnacl-nm')
    help_str = pnacl_nm.get_help(self.default_argv)
    self.assertNotEqual(help_str, driver_tools.HelpNotAvailable())
    self.assertTrue(len(help_str) > 0)

  def test_RanlibHelpMessage(self):
    env.set('SCRIPT_NAME', 'pnacl-ranlib')
    help_str = pnacl_ranlib.get_help(self.default_argv)
    self.assertNotEqual(help_str, driver_tools.HelpNotAvailable())
    self.assertTrue(len(help_str) > 0)

  def test_StripHelpMessage(self):
    env.set('SCRIPT_NAME', 'pnacl-strip')
    help_str = pnacl_strip.get_help(self.default_argv)
    self.assertNotEqual(help_str, driver_tools.HelpNotAvailable())
    self.assertTrue(len(help_str) > 0)

  def test_TranslateHelpMessage(self):
    env.set('SCRIPT_NAME', 'pnacl-translate')
    help_str = pnacl_translate.get_help(self.default_argv)
    self.assertNotEqual(help_str, driver_tools.HelpNotAvailable())
    self.assertTrue(len(help_str) > 0)


if __name__ == '__main__':
  unittest.main()
