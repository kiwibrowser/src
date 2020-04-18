#!/usr/bin/env python2

import argparse
import os
import sys

from utils import shellcmd

def main():
  """Run the specified command only if conditions are met.

     Two conditions are checked. First, the CONDITION must be true.
     Secondly, all NEED names must be in the set of HAVE names.
     If both conditions are met, the command defined by the remaining
     arguments is run in a shell.
  """
  argparser = argparse.ArgumentParser(
    description='    ' + main.__doc__,
    formatter_class=argparse.ArgumentDefaultsHelpFormatter)
  argparser.add_argument('--cond', choices={'true', 'false'} , required=False,
                         default='true', metavar='CONDITION',
                         help='Condition to test.')
  argparser.add_argument('--need', required=False, default=[],
                         action='append', metavar='NEED',
                         help='Needed name. May be repeated.')
  argparser.add_argument('--have', required=False, default=[],
                         action='append', metavar='HAVE',
                         help='Name you have. May be repeated.')
  argparser.add_argument('--echo-cmd', required=False,
                         action='store_true',
                         help='Trace the command before running.')
  argparser.add_argument('--command', nargs=argparse.REMAINDER,
                         help='Command to run if attributes found.')

  args = argparser.parse_args()

  # Quit early if no command to run.
  if not args.command:
    raise RuntimeError("No command argument(s) specified for ifatts")

  if args.cond == 'true' and set(args.need) <= set(args.have):
    stdout_result = shellcmd(args.command, echo=args.echo_cmd)
    if not args.echo_cmd:
      sys.stdout.write(stdout_result)

if __name__ == '__main__':
  main()
  sys.exit(0)
