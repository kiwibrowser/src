# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generates a sysroot tarball for building a specific package.

Meant for use after setup_board and build_packages have been run.
"""

from __future__ import print_function

import os

from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import commandline
from chromite.lib import osutils
from chromite.lib import sudo
from chromite.lib import sysroot_lib

DEFAULT_NAME = 'sysroot_%(package)s.tar.xz'
PACKAGE_SEPARATOR = '/'
SYSROOT = 'sysroot'


def ParseCommandLine(argv):
  """Parse args, and run environment-independent checks."""
  parser = commandline.ArgumentParser(description=__doc__)
  parser.add_argument('--board', required=True,
                      help=('The board to generate the sysroot for.'))
  parser.add_argument('--package', required=True,
                      help=('The packages to generate the sysroot for.'))
  parser.add_argument('--deps-only', action='store_true',
                      default=False,
                      help='Build dependencies only.')
  parser.add_argument('--out-dir', type='path', required=True,
                      help='Directory to place the generated tarball.')
  parser.add_argument('--out-file', default=DEFAULT_NAME,
                      help='The name to give to the tarball. '
                           'Defaults to %(default)s.')
  options = parser.parse_args(argv)

  options.out_file %= {
      'package': options.package.split()[0].replace(PACKAGE_SEPARATOR, '_'),
  }

  return options


class GenerateSysroot(object):
  """Wrapper for generation functionality."""

  PARALLEL_EMERGE = os.path.join(constants.CHROMITE_BIN_DIR, 'parallel_emerge')

  def __init__(self, sysroot, options):
    """Initialize

    Args:
      sysroot: Path to sysroot.
      options: Parsed options.
    """
    self.sysroot = sysroot
    self.options = options
    self.extra_env = {'ROOT': self.sysroot, 'USE': os.environ.get('USE', '')}

  def _Emerge(self, *args, **kwargs):
    """Emerge the given packages using parallel_emerge."""
    cmd = [self.PARALLEL_EMERGE, '--board=%s' % self.options.board,
           '--usepkgonly', '--noreplace'] + list(args)
    kwargs.setdefault('extra_env', self.extra_env)
    cros_build_lib.SudoRunCommand(cmd, **kwargs)

  def _InstallToolchain(self):
    # Create the sysroot's config.
    sysroot = sysroot_lib.Sysroot(self.sysroot)
    sysroot.WriteConfig(sysroot.GenerateBoardConfig(self.options.board))
    cros_build_lib.RunCommand(
        [os.path.join(constants.CROSUTILS_DIR, 'install_toolchain'),
         '--noconfigure', '--sysroot', self.sysroot])

  def _InstallKernelHeaders(self):
    self._Emerge('sys-kernel/linux-headers')

  def _InstallBuildDependencies(self):
    # Calculate buildtime deps that are not runtime deps.
    raw_sysroot = cros_build_lib.GetSysroot(board=self.options.board)
    packages = []
    if not self.options.deps_only:
      packages = self.options.package.split()
    else:
      for pkg in self.options.package.split():
        cmd = ['qdepends', '-q', '-C', pkg]
        output = cros_build_lib.RunCommand(
            cmd, extra_env={'ROOT': raw_sysroot}, capture_output=True).output

        if output.count('\n') > 1:
          raise AssertionError('Too many packages matched for given pattern')

        # qdepend outputs "package: deps", so only grab the deps.
        deps = output.partition(':')[2].split()
        packages.extend(deps)
    # Install the required packages.
    if packages:
      self._Emerge(*packages)

  def _CreateTarball(self):
    target = os.path.join(self.options.out_dir, self.options.out_file)
    cros_build_lib.CreateTarball(target, self.sysroot, sudo=True)

  def Perform(self):
    """Generate the sysroot."""
    self._InstallToolchain()
    self._InstallKernelHeaders()
    self._InstallBuildDependencies()
    self._CreateTarball()


def FinishParsing(options):
  """Run environment dependent checks on parsed args."""
  target = os.path.join(options.out_dir, options.out_file)
  if os.path.exists(target):
    cros_build_lib.Die('Output file %r already exists.' % target)

  if not os.path.isdir(options.out_dir):
    cros_build_lib.Die(
        'Non-existent directory %r specified for --out-dir' % options.out_dir)


def main(argv):
  options = ParseCommandLine(argv)
  FinishParsing(options)

  cros_build_lib.AssertInsideChroot()

  with sudo.SudoKeepAlive(ttyless_sudo=False):
    with osutils.TempDir(set_global=True, sudo_rm=True) as tempdir:
      sysroot = os.path.join(tempdir, SYSROOT)
      os.mkdir(sysroot)
      GenerateSysroot(sysroot, options).Perform()
