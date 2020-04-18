# -*- coding: utf-8 -*-
# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Set up the environment for locally testing fuzz targets."""

from __future__ import print_function

import os

from chromite.lib import commandline
from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import osutils


def RunMountCommands(sysroot_path, unmount):
  """Mount/Unmount the proc & dev paths.

  Checks to see if path is mounted befor attempting to
  mount or unmount it.
  """

  for point in ['proc', 'dev']:
    mount_path = os.path.join(sysroot_path, point)
    if unmount:
      if osutils.IsMounted(mount_path):
        osutils.UmountDir(mount_path, cleanup=False)
    else:
      if not osutils.IsMounted(mount_path):
        # Not using osutils.Mount here, as not sure how to call
        # it to get the correct flags, etc.
        if point == 'proc':
          command = ['mount', '-t', 'proc', 'none', mount_path]
        else:
          command = ['mount', '-o', 'bind', '/dev', mount_path]
        cros_build_lib.SudoRunCommand(command)


def GetParser():
  parser = commandline.ArgumentParser(description=__doc__)

  parser.add_argument('--board',
                      required=True,
                      help='Board on which to run test.')
  parser.add_argument('--cleanup',
                      action='store_true',
                      help='Use this option after the testing is finished.'
                      ' It will undo the mount commands.')

  return parser


def main(argv):
  """Parse arguments, calls RunMountCommands & copies files."""

  cros_build_lib.AssertOutsideChroot()

  parser = GetParser()
  options = parser.parse_args(argv)

  chromeos_root = constants.SOURCE_ROOT
  chroot_path = os.path.join(chromeos_root, 'chroot')
  sysroot_path = os.path.join(chroot_path, 'build', options.board)

  RunMountCommands(sysroot_path, options.cleanup)

  # Do not copy the files if we are cleaning up.
  if not options.cleanup:
    src = os.path.join(chroot_path, 'usr', 'bin', 'asan_symbolize.py')
    dst = os.path.join(sysroot_path, 'usr', 'bin')
    cros_build_lib.SudoRunCommand(['cp', src, dst])

    # Create a directory for installing llvm-symbolizer and all of
    # its dependencies in the sysroot.
    install_path = os.path.join('usr', 'libexec', 'llvm-symbolizer')
    install_dir = os.path.join(sysroot_path, install_path)
    cmd = ['mkdir', '-p', install_dir]
    cros_build_lib.SudoRunCommand(cmd)

    # Now install llvm-symbolizer, with all its dependencies, in the
    # sysroot.
    llvm_symbolizer = os.path.join('usr', 'bin', 'llvm-symbolizer')

    lddtree_script = os.path.join(chromeos_root, 'chromite', 'bin', 'lddtree')
    cmd = [lddtree_script, '-v', '--generate-wrappers', '--root', chroot_path,
           '--copy-to-tree', install_dir, os.path.join('/', llvm_symbolizer)]
    cros_build_lib.SudoRunCommand(cmd)

    # Create a symlink to llvm-symbolizer in the sysroot.
    rel_path = os.path.relpath(install_dir, dst)
    link_path = os.path.join(rel_path, llvm_symbolizer)
    dest = os.path.join(sysroot_path, llvm_symbolizer)
    if not os.path.exists(dest):
      cmd = ['ln', '-s', link_path, '-t', dst]
      cros_build_lib.SudoRunCommand(cmd)
