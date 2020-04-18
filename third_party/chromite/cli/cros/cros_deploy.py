# -*- coding: utf-8 -*-
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""cros deploy: Deploy the packages onto the target device."""

from __future__ import print_function

from chromite.cli import command
from chromite.cli import deploy
from chromite.lib import commandline
from chromite.lib import cros_logging as logging


@command.CommandDecorator('deploy')
class DeployCommand(command.CliCommand):
  """Deploy the requested packages to the target device.

  This command assumes the requested packages are already built in the
  chroot. This command needs to run inside the chroot for inspecting
  the installed packages.

  Note: If the rootfs on your device is read-only, this command
  remounts it as read-write. If the rootfs verification is enabled on
  your device, this command disables it.
  """

  EPILOG = """
To deploy packages:
  cros deploy device power_manager cherrypy
  cros deploy device /path/to/package

To uninstall packages:
  cros deploy --unmerge cherrypy

For more information of cros build usage:
  cros build -h
"""

  @classmethod
  def AddParser(cls, parser):
    """Add a parser."""
    super(cls, DeployCommand).AddParser(parser)
    cls.AddDeviceArgument(parser)
    parser.add_argument(
        'packages', help='Packages to install. You can specify '
        '[category/]package[:slot] or the path to the binary package. '
        'Use @installed to update all installed packages (requires --update).',
        nargs='+')
    parser.add_argument(
        '--board',
        help='The board to use. By default it is automatically detected. You '
        'can override the detected board with this option.')
    parser.add_argument(
        '--no-strip', dest='strip', action='store_false', default=True,
        help='Do not run strip_package to filter out preset paths in the '
        'package. Stripping removes debug symbol files and reduces the size '
        'of the package significantly. Defaults to always strip.')
    parser.add_argument(
        '--unmerge', dest='emerge', action='store_false', default=True,
        help='Unmerge requested packages.')
    parser.add_argument(
        '--root', default='/',
        help="Package installation root, e.g. '/' or '/usr/local'"
        " (default: '%(default)s').")
    parser.add_argument(
        '--no-clean-binpkg', dest='clean_binpkg', action='store_false',
        default=True, help='Do not clean outdated binary packages. '
        ' Defaults to always clean.')
    parser.add_argument(
        '--emerge-args', default=None,
        help='Extra arguments to pass to emerge.')
    parser.add_argument(
        '--private-key', type='path', default=None,
        help='SSH identify file (private key).')
    parser.add_argument(
        '--no-ping', dest='ping', action='store_false', default=True,
        help='Do not ping the device before attempting to connect to it.')
    parser.add_argument(
        '--dry-run', '-n', action='store_true',
        help='Output deployment plan but do not deploy anything.')

    advanced = parser.add_argument_group('Advanced options')
    advanced.add_argument(
        '--force', action='store_true',
        help='Ignore sanity checks, just do it.')
    # TODO(garnold) Make deep and check installed the default behavior.
    advanced.add_argument(
        '--update', action='store_true',
        help='Check installed versions on target (emerge only).')
    advanced.add_argument(
        '--deep', action='store_true',
        help='Install dependencies. Implies --update.')
    advanced.add_argument(
        '--deep-rev', action='store_true',
        help='Install reverse dependencies. Implies --deep.')

  def Run(self):
    """Run cros deploy."""
    commandline.RunInsideChroot(self)
    self.options.Freeze()
    deploy.Deploy(
        self.options.device,
        self.options.packages,
        board=self.options.board,
        emerge=self.options.emerge,
        update=self.options.update,
        deep=self.options.deep,
        deep_rev=self.options.deep_rev,
        clean_binpkg=self.options.clean_binpkg,
        root=self.options.root,
        strip=self.options.strip,
        emerge_args=self.options.emerge_args,
        ssh_private_key=self.options.private_key,
        ping=self.options.ping,
        force=self.options.force,
        dry_run=self.options.dry_run)
    logging.info('cros deploy completed successfully.')
