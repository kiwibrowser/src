# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This script moves ebuilds between 'stable' and 'live' states.

By default 'stable' ebuilds point at and build from source at the
last known good commit. Moving an ebuild to 'live' (via cros_workon start)
is intended to support development. The current source tip is fetched,
source modified and built using the unstable 'live' (9999) ebuild.
"""

from __future__ import print_function

from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import terminal
from chromite.lib import workon_helper


def GetParser():
  """Get a CLI parser."""
  parser = commandline.ArgumentParser(description=__doc__)
  parser.add_argument('--board', default=cros_build_lib.GetDefaultBoard(),
                      help='The board to set package keywords for.')
  parser.add_argument('--host', default=False, action='store_true',
                      help='Uses the host instead of board')
  parser.add_argument('--remote', default='',
                      help='For non-workon projects, the git remote to use.')
  parser.add_argument('--revision', default='',
                      help='Use to override the manifest defined default '
                           'revision used for a project')
  parser.add_argument('--command', default='git status', dest='iterate_command',
                      help='The command to be run by forall.')
  parser.add_argument('--workon_only', default=False, action='store_true',
                      help='Apply to packages that have a workon ebuild only')
  parser.add_argument('--all', default=False, action='store_true',
                      help='Apply to all possible packages for the '
                           'given command (overrides workon_only)')

  commands = [
      ('start', 'Moves an ebuild to live (intended to support development)'),
      ('stop', 'Moves an ebuild to stable (use last known good)'),
      ('info', 'Print package name, repo name, and source directory.'),
      ('list', 'List of live ebuilds (workon ebuilds if --all)'),
      ('list-all', 'List all of the live ebuilds for all setup boards'),
      ('iterate', 'For each ebuild, cd to the source dir and run a command'),
  ]
  command_parsers = parser.add_subparsers(dest='command', title='commands')
  for command, description in commands:
    sub_parser = command_parsers.add_parser(command, description=description,
                                            help=description)
    sub_parser.add_argument('packages', nargs='*',
                            help='The packages to run command against.')

  return parser


def main(argv):
  parser = GetParser()
  options = parser.parse_args(argv)
  options.Freeze()

  if options.command == 'list-all':
    board_to_packages = workon_helper.ListAllWorkedOnAtoms()
    color = terminal.Color()
    for board in sorted(board_to_packages):
      print(color.Start(color.GREEN) + board + ':' + color.Stop())
      for package in board_to_packages[board]:
        print('    ' + package)
      print('')
    return 0

  # TODO(wiley): Assert that we're not running as root.
  cros_build_lib.AssertInsideChroot()

  if options.host:
    friendly_name = 'host'
    sysroot = '/'
  elif options.board:
    friendly_name = options.board
    sysroot = cros_build_lib.GetSysroot(board=options.board)
  else:
    cros_build_lib.Die('You must specify either --host, --board')

  helper = workon_helper.WorkonHelper(sysroot, friendly_name)
  try:
    if options.command == 'start':
      helper.StartWorkingOnPackages(options.packages, use_all=options.all,
                                    use_workon_only=options.workon_only)
    elif options.command == 'stop':
      helper.StopWorkingOnPackages(options.packages, use_all=options.all,
                                   use_workon_only=options.workon_only)
    elif options.command == 'info':
      triples = helper.GetPackageInfo(options.packages, use_all=options.all,
                                      use_workon_only=options.workon_only)
      for package, repos, paths in triples:
        print(package, ','.join(repos), ','.join(paths))
    elif options.command == 'list':
      packages = helper.ListAtoms(
          use_all=options.all, use_workon_only=options.workon_only)
      if packages:
        print('\n'.join(packages))
    elif options.command == 'iterate':
      helper.RunCommandInPackages(options.packages, options.iterate_command,
                                  use_all=options.all,
                                  use_workon_only=options.workon_only)
  except workon_helper.WorkonError as e:
    cros_build_lib.Die(e.message)

  return 0
