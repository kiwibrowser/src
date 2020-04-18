# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to generate a sysroot tarball.

Script that generates a tarball containing changes that are needed to create a
complete sysroot from extracted prebuilt packages.
"""

from __future__ import print_function

import os

from chromite.lib import constants
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import osutils
from chromite.lib import portage_util
from chromite.lib import sysroot_lib

_CREATE_BATCH_CMD = ('rsync',)
_CREATE_BATCH_EXCLUDE = ('--exclude=/tmp/', '--exclude=/var/cache/',
                         '--exclude=/usr/local/autotest/packages**',
                         '--exclude=/packages/', '--exclude=**.pyc',
                         '--exclude=**.pyo')
# rsync is used in archive mode with no --times.
# --checksum is used to ensure 100% accuracy.
# --delete is used to account for files that may be deleted during emerge.
# Short version: rsync -rplgoDc --delete
_CREATE_BATCH_ARGS = ('--recursive', '--links', '--perms', '--group',
                      '--owner', '--devices', '--specials', '--checksum',
                      '--delete')

# We want to ensure that we use only binary packages. However,
# build_packages will try to rebuild any unbuilt packages. Ignore those through
# --norebuild.
_BUILD_PKGS_CMD = (os.path.join(constants.CROSUTILS_DIR, 'build_packages'),
                   '--skip_chroot_upgrade', '--norebuild', '--usepkgonly')


def CreateBatchFile(build_dir, out_dir, batch_file):
  """Creates a batch file using rsync between build_dir and out_dir.

  This batch file can be applied to any directory identical to out_dir, to make
  it identical to build_dir.

  Args:
    build_dir: Directory to rsync from.
    out_dir: Directory to rsync to.
    batch_file: Batch file to be created.
  """
  cmd = list(_CREATE_BATCH_CMD)
  cmd.extend(list(_CREATE_BATCH_EXCLUDE))
  cmd.extend(list(_CREATE_BATCH_ARGS))
  cmd.extend(['--only-write-batch=' + batch_file, build_dir + '/', out_dir])
  cros_build_lib.SudoRunCommand(cmd)


def _ParseCommandLine(argv):
  """Parse args, and run environment-independent checks."""
  parser = commandline.ArgumentParser(description=__doc__)
  parser.add_argument('--board', required=True,
                      help='The board to generate the sysroot for.')
  parser.add_argument('--out-dir', type='path', required=True,
                      help='Directory to place the generated tarball.')
  parser.add_argument('--out-batch', default=constants.DELTA_SYSROOT_BATCH,
                      help=('The name to give to the batch file. Defaults to '
                            '%r.' % constants.DELTA_SYSROOT_BATCH))
  parser.add_argument('--out-file', default=constants.DELTA_SYSROOT_TAR,
                      help=('The name to give to the tarball. Defaults to %r.'
                            % constants.DELTA_SYSROOT_TAR))
  parser.add_argument('--skip-tests', action='store_false', default=True,
                      dest='build_tests',
                      help='If we should not build the autotests packages.')
  options = parser.parse_args(argv)

  return options


def FinishParsing(options):
  """Run environment dependent checks on parsed args."""
  target = os.path.join(options.out_dir, options.out_file)
  if os.path.exists(target):
    cros_build_lib.Die('Output file %r already exists.' % target)

  if not os.path.isdir(options.out_dir):
    cros_build_lib.Die(
        'Non-existent directory %r specified for --out-dir' % options.out_dir)


def GenerateSysroot(sysroot_path, board, build_tests, unpack_only=False):
  """Create a sysroot using only binary packages from local binhost.

  Args:
    sysroot_path: Where we want to place the sysroot.
    board: Board we want to build for.
    build_tests: If we should include autotest packages.
    unpack_only: If we only want to unpack the binary packages, and not build
                 them.
  """
  osutils.SafeMakedirs(sysroot_path)
  if not unpack_only:
    # Generate the sysroot configuration.
    sysroot = sysroot_lib.Sysroot(sysroot_path)
    sysroot.WriteConfig(sysroot.GenerateBoardConfiguration(board))
    cros_build_lib.RunCommand(
        [os.path.join(constants.CROSUTILS_DIR, 'install_toolchain'),
         '--noconfigure', '--sysroot', sysroot_path])
  cmd = list(_BUILD_PKGS_CMD)
  cmd.extend(['--board_root', sysroot_path, '--board', board])
  if unpack_only:
    cmd.append('--unpackonly')
  if not build_tests:
    cmd.append('--nowithautotest')
  env = {'USE': os.environ.get('USE', ''),
         'PORTAGE_BINHOST': 'file://%s' % portage_util.GetBinaryPackageDir(
             sysroot=cros_build_lib.GetSysroot(board))}
  cros_build_lib.RunCommand(cmd, extra_env=env)


def main(argv):
  """Generate the delta sysroot

  Create a tarball containing a sysroot that can be patched over extracted
  prebuilt package contents to create a complete sysroot.

  1. Unpack all packages for a board into an unpack_only sysroot directory.
  2. Emerge all packages for a board into a build sysroot directory.
  3. Create a batch file using:
    rsync -rplgoDc --delete --write-batch=<batch> <build_sys> <unpackonly_sys>
  4. Put the batch file inside a tarball.
  """
  options = _ParseCommandLine(argv)
  FinishParsing(options)

  cros_build_lib.AssertInsideChroot()

  with osutils.TempDir(set_global=False, sudo_rm=True) as tmp_dir:
    build_sysroot = os.path.join(tmp_dir, 'build-sys')
    unpackonly_sysroot = os.path.join(tmp_dir, 'tmp-sys')
    batch_filename = options.out_batch

    GenerateSysroot(unpackonly_sysroot, options.board, options.build_tests,
                    unpack_only=True)
    GenerateSysroot(build_sysroot, options.board, options.build_tests,
                    unpack_only=False)

    # Finally create batch file.
    CreateBatchFile(build_sysroot, unpackonly_sysroot,
                    os.path.join(tmp_dir, batch_filename))

    cros_build_lib.CreateTarball(
        os.path.join(options.out_dir, options.out_file), tmp_dir, sudo=True,
        inputs=[batch_filename])
