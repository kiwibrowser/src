#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""End to end test of toolchain_build."""

import os
import sys
import unittest

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import pynacl.file_tools
import pynacl.working_directory

import command
import toolchain_main


INITIAL_README_TEXT = 'foo'
LATER_README_TEXT = 'bar'


def GetTestPackages(change_readme):

  def ShouldChangeReadme(_):
    return change_readme

  return {
      'newlib': {
          'type': 'source',
          'commands': [
              command.WriteData(INITIAL_README_TEXT, '%(output)s/README'),
              command.WriteData(LATER_README_TEXT, '%(output)s/README',
                                run_cond=ShouldChangeReadme),
          ],
      },
      'newlib_build': {
          'type': 'build',
          'dependencies': ['newlib'],
          'commands': [
              command.Copy('%(newlib)s/README', '%(output)s/test1'),
          ],
      },
  }


def GetTestPackageTargets():
  return {
      'newlib_test': {
           'newlib_out': ['newlib_build'],
      },
  }


class TestToolchainBuild(unittest.TestCase):

  def test_BuildAfterChange(self):
    # Test that changes trigger a rebuild immediately.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      source_dir = os.path.join(work_dir, 'src')
      output_dir = os.path.join(work_dir, 'out')
      cache_dir = os.path.join(work_dir, 'cache')
      readme_out = os.path.join(output_dir, 'newlib_build_install', 'test1')
      args = ['--source', source_dir,
              '--output', output_dir,
              '--cache', cache_dir,
              '--no-use-remote-cache',
              '--sync']
      if '-v' in sys.argv or '--verbose' in sys.argv:
        args += ['--verbose', '--emit-signatures=-', '--no-annotator']
      else:
        args += ['--quiet']
      packages = GetTestPackages(change_readme=False)
      package_targets = GetTestPackageTargets()
      # Build once (so things are in place).
      tb = toolchain_main.PackageBuilder(
          packages, package_targets, args)
      tb.Main()
      self.assertEqual(
          INITIAL_README_TEXT,
          pynacl.file_tools.ReadFile(readme_out))
      # Build again (so the the cache is hit).
      tb = toolchain_main.PackageBuilder(
          packages, package_targets, args)
      tb.Main()
      self.assertEqual(
          INITIAL_README_TEXT,
          pynacl.file_tools.ReadFile(readme_out))
      # Build again after changing README.
      packages = GetTestPackages(change_readme=True)
      tb = toolchain_main.PackageBuilder(
          packages, package_targets, args)
      tb.Main()
      self.assertEqual(
          LATER_README_TEXT,
          pynacl.file_tools.ReadFile(readme_out))


if __name__ == '__main__':
  unittest.main()
