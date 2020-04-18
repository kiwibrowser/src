# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# TODO: Support cleaning /tmp and /build/*/tmp.
# TODO: Support running `eclean -q packages` on / and the sysroots.
# TODO: Support cleaning sysroots as a destructive option.

"""Clean up working files in a Chromium OS checkout.

If unsure, just use the --safe flag to clean out various objects.
"""

from __future__ import print_function

import glob
import os

from chromite.lib import constants
from chromite.cli import command
from chromite.cli import flash
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import osutils


@command.CommandDecorator('clean')
class CleanCommand(command.CliCommand):
  """Clean up working files from the build."""

  @classmethod
  def AddParser(cls, parser):
    """Add parser arguments."""
    super(CleanCommand, cls).AddParser(parser)

    parser.add_argument(
        '--safe', default=False, action='store_true',
        help='Clean up files that are automatically created.')
    parser.add_argument(
        '-n', '--dry-run', default=False, action='store_true',
        help='Show which paths would be cleaned up.')

    group = parser.add_argument_group(
        'Cache Selection (Advanced)',
        description='Clean out specific caches (--safe does all of these).')
    group.add_argument(
        '--cache', default=False, action='store_true',
        help='Clean up our shared cache dir.')
    group.add_argument(
        '--chromite', default=False, action='store_true',
        help='Clean up chromite working directories.')
    group.add_argument(
        '--deploy', default=False, action='store_true',
        help='Clean files cached by cros deploy.')
    group.add_argument(
        '--flash', default=False, action='store_true',
        help='Clean files cached by cros flash.')
    group.add_argument(
        '--images', default=False, action='store_true',
        help='Clean up locally generated images.')
    group.add_argument(
        '--incrementals', default=False, action='store_true',
        help='Clean up incremental package objects.')
    group.add_argument(
        '--logs', default=False, action='store_true',
        help='Clean up various build log files.')
    group.add_argument(
        '--workdirs', default=False, action='store_true',
        help='Clean up build various package build directories.')

    group = parser.add_argument_group(
        'Unrecoverable Options (Dangerous)',
        description='Clean out objects that cannot be recovered easily.')
    group.add_argument(
        '--clobber', default=False, action='store_true',
        help='Delete all non-source objects.')
    group.add_argument(
        '--chroot', default=False, action='store_true',
        help='Delete build chroot (affects all boards).')
    group.add_argument(
        '--board', action='append', help='Delete board(s) build root(s).')
    group.add_argument(
        '--autotest', default=False, action='store_true',
        help='Delete build_externals packages.')

  def __init__(self, options):
    """Initializes cros clean."""
    command.CliCommand.__init__(self, options)

  def Run(self):
    """Perfrom the cros clean command."""

    # If no option is set, default to "--safe"
    if not (self.options.safe or
            self.options.clobber or
            self.options.board or
            self.options.chroot or
            self.options.cache or
            self.options.deploy or
            self.options.flash or
            self.options.images or
            self.options.autotest or
            self.options.incrementals):
      self.options.safe = True

    if self.options.clobber:
      self.options.chroot = True
      self.options.autotest = True
      self.options.safe = True

    if self.options.safe:
      self.options.cache = True
      self.options.chromite = True
      self.options.deploy = True
      self.options.flash = True
      self.options.images = True
      self.options.incrementals = True
      self.options.logs = True
      self.options.workdirs = True

    self.options.Freeze()

    chroot_dir = os.path.join(constants.SOURCE_ROOT,
                              constants.DEFAULT_CHROOT_DIR)

    cros_build_lib.AssertOutsideChroot()

    def Clean(path):
      """Helper wrapper for the dry-run checks"""
      if self.options.dry_run:
        logging.notice('would have cleaned: %s', path)
      else:
        osutils.RmDir(path, ignore_missing=True, sudo=True)

    def CleanNoBindMount(path):
      # This test is a convenience for developers that bind mount these dirs.
      if not os.path.ismount(path):
        Clean(path)
      else:
        logging.debug('Ignoring bind mounted dir: %s',
                      self.options.path)

    # Delete this first since many of the caches below live in the chroot.
    if self.options.chroot:
      logging.debug('Remove the chroot.')
      if self.options.dry_run:
        logging.notice('would have cleaned: %s', chroot_dir)
      else:
        cros_build_lib.RunCommand(['cros_sdk', '--delete'])

    if self.options.board:
      for b in self.options.board:
        logging.debug('Clean up the %s build root.', b)
        Clean(os.path.join(chroot_dir, 'build', b))

    if self.options.cache:
      logging.debug('Clean the common cache')
      CleanNoBindMount(self.options.cache_dir)

    if self.options.chromite:
      logging.debug('Clean chromite workdirs')
      Clean(os.path.join(constants.CHROMITE_DIR, 'venv', 'venv'))
      Clean(os.path.join(constants.CHROMITE_DIR, 'venv', '.venv_lock'))

    if self.options.deploy:
      logging.debug('Clean up the cros deploy cache.')
      for subdir in ('custom-packages', 'gmerge-packages'):
        for d in glob.glob(os.path.join(chroot_dir, 'build', '*', subdir)):
          Clean(d)

    if self.options.flash:
      logging.debug('Clean up the cros flash cache.')
      Clean(flash.DEVSERVER_STATIC_DIR)

    if self.options.images:
      logging.debug('Clean the images cache.')
      cache_dir = os.path.join(constants.SOURCE_ROOT, 'src', 'build')
      CleanNoBindMount(cache_dir)

    if self.options.incrementals:
      logging.debug('Clean package incremental objects')
      Clean(os.path.join(chroot_dir, 'var', 'cache', 'portage'))
      for d in glob.glob(os.path.join(chroot_dir, 'build', '*', 'var', 'cache',
                                      'portage')):
        Clean(d)

    if self.options.logs:
      logging.debug('Clean log files')
      Clean(os.path.join(chroot_dir, 'var', 'log'))
      for d in glob.glob(os.path.join(chroot_dir, 'build', '*', 'tmp',
                                      'portage', 'logs')):
        Clean(d)

    if self.options.workdirs:
      logging.debug('Clean package workdirs')
      Clean(os.path.join(chroot_dir, 'var', 'tmp', 'portage'))
      Clean(os.path.join(constants.CHROMITE_DIR, 'venv', 'venv'))
      for d in glob.glob(os.path.join(chroot_dir, 'build', '*', 'tmp',
                                      'portage')):
        Clean(d)

    if self.options.autotest:
      logging.debug('Clean build_externals')
      packages_dir = os.path.join(
          constants.SOURCE_ROOT, 'src', 'third_party', 'autotest', 'files',
          'site-packages')
      Clean(packages_dir)
