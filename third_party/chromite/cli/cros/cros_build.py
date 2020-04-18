# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""cros build: Build the requested packages."""

from __future__ import print_function

from chromite.cli import command
from chromite.lib import chroot_util
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import operation
from chromite.lib import parallel
from chromite.lib import workon_helper


class BrilloBuildOperation(operation.ParallelEmergeOperation):
  """Wrapper around operation.ParallelEmergeOperation.

  Currently, this class is empty as the main component is just
  operation.ParallelEmergeOperation. However, self._CheckDependencies also
  produces output, then that output can be captured here.
  """


@command.CommandDecorator('build')
class BuildCommand(command.CliCommand):
  """Build the requested packages."""

  _BAD_DEPEND_MSG = '\nemerge detected broken ebuilds. See error message above.'
  EPILOG = """
To update specified package and all dependencies:
  cros build --board=lumpy power_manager
  cros build --host cros-devutils

To just build a single package:
  cros build --board=lumpy --no-deps power_manager
"""

  def __init__(self, options):
    super(BuildCommand, self).__init__(options)
    self.chroot_update = options.chroot_update and options.deps
    if options.chroot_update and not options.deps:
      logging.debug('Skipping chroot update due to --nodeps')
    self.build_pkgs = options.packages
    self.host = False
    self.board = None
    self.brick = None

    if self.options.host:
      self.host = True
    elif self.options.board:
      self.board = self.options.board
    else:
      # If nothing is explicitly set, use the default board.
      self.board = cros_build_lib.GetDefaultBoard()

    # Set sysroot and friendly name. The latter is None if building for host.
    self.sysroot = cros_build_lib.GetSysroot(self.board)

  @classmethod
  def AddParser(cls, parser):
    super(cls, BuildCommand).AddParser(parser)
    target = parser.add_mutually_exclusive_group()
    target.add_argument('--board', help='The board to build packages for.')
    target.add_argument('--host', help='Build packages for the chroot itself.',
                        default=False, action='store_true')
    parser.add_argument('--no-binary', help="Don't use binary packages.",
                        default=True, dest='binary', action='store_false')
    parser.add_argument('--init-only', action='store_true',
                        help="Initialize build environment but don't build "
                        "anything.")
    deps = parser.add_mutually_exclusive_group()
    deps.add_argument('--no-deps', help="Don't update dependencies.",
                      default=True, dest='deps', action='store_false')
    deps.add_argument('--rebuild-deps', default=False, action='store_true',
                      help='Automatically rebuild dependencies.')
    deps.add_argument('--test', default=False, action='store_true',
                      help='Build/run the package tests.')
    parser.add_argument('packages',
                        help='Packages to build. If no packages listed, uses '
                        'the current brick main package.',
                        nargs='*')

    # Advanced options.
    advanced = parser.add_argument_group('Advanced options')
    advanced.add_argument('--no-host-packages-update',
                          dest='host_packages_update', default=True,
                          action='store_false',
                          help="Don't update host packages during chroot "
                          "update.")
    advanced.add_argument('--no-chroot-update', default=True,
                          dest='chroot_update', action='store_false',
                          help="Don't update chroot at all.")
    advanced.add_argument('--no-enable-only-latest', default=True,
                          dest='enable_only_latest', action='store_false',
                          help="Don't enable packages with only live ebuilds.")
    advanced.add_argument('--jobs', default=None, type=int,
                          help='Maximum job count to run in parallel '
                          '(uses all available cores by default).')

    # Legacy options, for backward compatibiltiy.
    legacy = parser.add_argument_group('Options for backward compatibility')
    legacy.add_argument('--norebuild', default=True, dest='rebuild_deps',
                        action='store_false', help='Inverse of --rebuild-deps.')

  def _CheckDependencies(self):
    """Verify emerge dependencies.

    Verify all board packages can be emerged from scratch, without any
    backtracking. This ensures that no updates are skipped by Portage due to
    the fallback behavior enabled by the backtrack option, and helps catch
    cases where Portage skips an update due to a typo in the ebuild.

    Only print the output if this step fails or if we're in debug mode.
    """
    if self.options.deps and not self.host:
      cmd = chroot_util.GetEmergeCommand(sysroot=self.sysroot)
      cmd += ['-pe', '--backtrack=0'] + self.build_pkgs
      try:
        cros_build_lib.RunCommand(cmd, combine_stdout_stderr=True,
                                  debug_level=logging.DEBUG)
      except cros_build_lib.RunCommandError as ex:
        ex.msg += self._BAD_DEPEND_MSG
        raise

  def _Build(self):
    """Update the chroot, then merge the requested packages."""
    if self.chroot_update and self.host:
      chroot_util.UpdateChroot()

    chroot_util.Emerge(self.build_pkgs, self.sysroot,
                       with_deps=self.options.deps,
                       rebuild_deps=self.options.rebuild_deps,
                       use_binary=self.options.binary, jobs=self.options.jobs,
                       debug_output=(self.options.log_level.lower() == 'debug'))

  def _Test(self):
    """Update the chroot, then merge the requested packages."""
    # This ALWAYS runs after build, so we don't need to update the chroot.
    chroot_util.RunUnittests(
        self.sysroot, self.build_pkgs,
        verbose=(self.options.log_level.lower() == 'debug'))

  def Run(self):
    """Run cros build."""
    self.options.Freeze()

    if not self.host:
      if not (self.board or self.brick):
        cros_build_lib.Die('You did not specify a board/brick to build for. '
                           'You need to be in a brick directory or set '
                           '--board/--brick/--host')

      if self.brick and self.brick.legacy:
        cros_build_lib.Die('--brick should not be used with board names. Use '
                           '--board=%s instead.' % self.brick.config['name'])

    if self.board:
      chroot_args = ['--board', self.board]
    else:
      chroot_args = None

    commandline.RunInsideChroot(self, chroot_args=chroot_args)

    if not (self.build_pkgs or self.options.init_only):
      cros_build_lib.Die('No packages found, nothing to build.')

    # Set up the sysroots if not building for host.
    if self.brick or self.board:
      chroot_util.SetupBoard(
          brick=self.brick, board=self.board,
          update_chroot=self.chroot_update,
          update_host_packages=self.options.host_packages_update,
          use_binary=self.options.binary)

    if not self.options.init_only:
      # Preliminary: enable all packages that only have a live ebuild.
      if self.options.enable_only_latest:
        workon = workon_helper.WorkonHelper(self.sysroot)
        workon.StartWorkingOnPackages([], use_workon_only=True)

      if command.UseProgressBar():
        op = BrilloBuildOperation()
        op.Run(
            parallel.RunParallelSteps, [self._CheckDependencies, self._Build],
            log_level=logging.DEBUG)
        if self.options.test:
          self._Test()
      else:
        parallel.RunParallelSteps([self._CheckDependencies, self._Build])
        if self.options.test:
          self._Test()
      logging.notice('Build completed successfully.')
