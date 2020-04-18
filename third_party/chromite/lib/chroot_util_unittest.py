# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests the chroot_util module."""

from __future__ import print_function

import itertools

from chromite.lib import chroot_util
from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib

if cros_build_lib.IsInsideChroot():
  from chromite.scripts import cros_list_modified_packages

# pylint: disable=protected-access


class ChrootUtilTest(cros_test_lib.RunCommandTempDirTestCase):
  """Test class for the chroot_util functions."""

  def testEmerge(self):
    """Tests correct invocation of emerge."""
    packages = ['foo-app/bar', 'sys-baz/clap']
    self.PatchObject(cros_list_modified_packages, 'ListModifiedWorkonPackages',
                     return_value=[packages[0]])

    toolchain_packages = [
        'sys-devel/binutils',
        'sys-devel/gcc',
        'sys-kernel/linux-headers',
        'sys-libs/glibc',
        'sys-devel/gdb'
    ]
    self.PatchObject(chroot_util, '_GetToolchainPackages',
                     return_value=toolchain_packages)
    toolchain_package_list = ' '.join(toolchain_packages)

    input_values = [
        ['/', '/build/thesysrootname'],  # sysroot
        [True, False],  # with_deps
        [True, False],  # rebuild_deps
        [True, False],  # use_binary
        [0, 1, 2, 3],   # jobs
        [True, False],  # debug_output
    ]
    inputs = itertools.product(*input_values)
    for (sysroot, with_deps, rebuild_deps, use_binary,
         jobs, debug_output) in inputs:
      chroot_util.Emerge(packages, sysroot=sysroot, with_deps=with_deps,
                         rebuild_deps=rebuild_deps, use_binary=use_binary,
                         jobs=jobs, debug_output=debug_output)
      cmd = self.rc.call_args_list[-1][0][-1]
      self.assertEquals(sysroot != '/',
                        any(p.startswith('--sysroot') for p in cmd))
      self.assertEquals(with_deps, '--deep' in cmd)
      self.assertEquals(not with_deps, '--nodeps' in cmd)
      self.assertEquals(rebuild_deps, '--rebuild-if-unbuilt' in cmd)
      self.assertEquals(use_binary, '-g' in cmd)
      self.assertEquals(use_binary, '--with-bdeps=y' in cmd)
      self.assertEquals(use_binary and sysroot == '/',
                        '--useoldpkg-atoms=%s' % toolchain_package_list in cmd)
      self.assertEquals(bool(jobs), '--jobs=%d' % jobs in cmd)
      self.assertEquals(debug_output, '--show-output' in cmd)
