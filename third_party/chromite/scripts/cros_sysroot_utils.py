# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Collection of tools to create sysroots."""


from __future__ import print_function

import os
import sys

from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import sysroot_lib


def ParseArgs(argv):
  """Parse arguments.

  Args:
    argv: array of arguments passed to the script.
  """
  parser = commandline.ArgumentParser(description=__doc__)
  parser.set_defaults(out_file=None)
  subparser = parser.add_subparsers()
  wrapper = subparser.add_parser('create-wrappers')
  wrapper.add_argument('--sysroot', help='Path to the sysroot.', required=True)
  wrapper.add_argument('--friendlyname', help='Name to append to the commands.')
  wrapper.set_defaults(command='create-wrappers')

  config = subparser.add_parser('generate-config')
  target = config.add_mutually_exclusive_group(required=True)
  target.add_argument('--board', help='Board to generate the config for.')
  config.add_argument('--out-file', dest='out_file',
                      help='File to write into. If not specified, the '
                      'configuration will be printed to stdout.')
  config.add_argument('--sysroot', help='Path to the sysroot.', required=True)
  config.set_defaults(command='generate-config')

  makeconf = subparser.add_parser('generate-make-conf')
  makeconf.add_argument('--sysroot', help='Sysroot to use.')
  makeconf.add_argument('--out-file', dest='out_file',
                        help='File to write the configuration into. If not '
                        'specified, the configuration will be printed to '
                        'stdout.')
  makeconf.add_argument('--accepted-licenses',
                        help='List of accepted licenses.')
  makeconf.set_defaults(command='generate-make-conf')

  binhost = subparser.add_parser('generate-binhosts')
  binhost.add_argument('--sysroot', help='Sysroot to use.')
  binhost.add_argument('--out-file', dest='out_file',
                       help='File to write the configuration into. If not '
                       'specified, the configuration will be printed to '
                       'stdout.')
  binhost.add_argument('--chrome-only', dest='chrome_only', action='store_true',
                       help='Generate only the chrome binhost.')
  binhost.add_argument('--local-only', dest='local_only', action='store_true',
                       help='Use compatible local boards only.')
  binhost.set_defaults(command='generate-binhosts')

  options = parser.parse_args(argv)
  options.Freeze()
  return options


def main(argv):
  opts = ParseArgs(argv)
  if not cros_build_lib.IsInsideChroot():
    raise commandline.ChrootRequiredError()

  if os.geteuid() != 0:
    cros_build_lib.SudoRunCommand(sys.argv, print_cmd=False)
    return

  output = sys.stdout
  if opts.out_file:
    output = open(opts.out_file, 'w')

  sysroot = sysroot_lib.Sysroot(opts.sysroot)
  if opts.command == 'create-wrappers':
    sysroot.CreateAllWrappers(opts.friendlyname)
  elif opts.command == 'generate-config':
    config = sysroot.GenerateBoardConfig(opts.board)

    output.write('\n' + config)
  elif opts.command == 'generate-make-conf':
    output.write('\n' + sysroot.GenerateMakeConf(opts.accepted_licenses))
  elif opts.command == 'generate-binhosts':
    output.write('\n' + sysroot.GenerateBinhostConf(opts.chrome_only,
                                                    opts.local_only))
