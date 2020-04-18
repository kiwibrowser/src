# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This implements the entry point for the `cros` CLI toolset.

This script is invoked by chromite/bin/cros, which sets up the
proper execution environment and calls this module's main() function.

In turn, this script looks for a subcommand based on how it was invoked. For
example, `cros lint` will use the cli/cros/cros_lint.py subcommand.

See cli/ for actual command implementations.
"""

from __future__ import print_function

from chromite.cli import command
from chromite.lib import commandline
from chromite.lib import cros_logging as logging


def GetOptions(my_commands):
  """Returns the parser to use for commandline parsing.

  Args:
    my_commands: A dictionary mapping subcommand names to classes.

  Returns:
    A commandline.ArgumentParser object.
  """
  parser = commandline.ArgumentParser(caching=True, default_log_level='notice')

  if my_commands:
    subparsers = parser.add_subparsers(title='Subcommands')
    for cmd_name in sorted(my_commands.iterkeys()):
      class_def = my_commands[cmd_name]
      epilog = getattr(class_def, 'EPILOG', None)
      sub_parser = subparsers.add_parser(
          cmd_name, description=class_def.__doc__, epilog=epilog,
          caching=class_def.use_caching_options,
          formatter_class=commandline.argparse.RawDescriptionHelpFormatter)
      class_def.AddParser(sub_parser)

  return parser


def _RunSubCommand(subcommand):
  """Helper function for testing purposes."""
  return subcommand.Run()


def main(argv):
  try:
    parser = GetOptions(command.ListCommands())
    # Cros currently does nothing without a subcmd. Print help if no args are
    # specified.
    if not argv:
      parser.print_help()
      return 1

    namespace = parser.parse_args(argv)
    subcommand = namespace.command_class(namespace)
    try:
      code = _RunSubCommand(subcommand)
    except (commandline.ChrootRequiredError, commandline.ExecRequiredError):
      # The higher levels want these passed back, so oblige.
      raise
    except Exception as e:
      code = 1
      logging.error('cros %s failed before completing.',
                    subcommand.command_name)
      if namespace.debug:
        raise
      else:
        logging.error(e)

    if code is not None:
      return code

    return 0
  except KeyboardInterrupt:
    logging.debug('Aborted due to keyboard interrupt.')
    return 1
