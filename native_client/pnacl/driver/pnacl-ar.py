#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import driver_tools
from driver_env import env

EXTRA_ENV = {
  'ARGS':    '',
  'COMMAND': ''
}
# just pass all args through to 'ARGS' and eventually to the underlying tool
PATTERNS = [ ( '(.*)',  "env.append('ARGS', $0)") ]

def main(argv):
  if len(argv) ==  0:
    print get_help(argv)
    return 1

  env.update(EXTRA_ENV)
  driver_tools.ParseArgs(argv, PATTERNS)

  # Note: --plugin must come after the command flags, but before the filename.
  #       (definitely confirmed that it cannot go before the command)
  #       for now assume command is just the very first args
  args = env.get('ARGS')
  command = args.pop(0)
  env.set('COMMAND', command)
  env.set('ARGS', *args)
  driver_tools.Run('"${AR}" ${COMMAND} --plugin=${GOLD_PLUGIN_SO} ${ARGS}')
  # only reached in case of no errors
  return 0


def get_help(unused_argv):
  return """
Usage: %s [-]{dmpqrstx}[abcDfilMNoPsSTuvV] [member-name] [count] archive-file file...
 commands:
  d            - delete file(s) from the archive
  m[ab]        - move file(s) in the archive
  p            - print file(s) found in the archive
  q[f]         - quick append file(s) to the archive
  r[ab][f][u]  - replace existing or insert new file(s) into the archive
  s            - act as ranlib
  t            - display contents of archive
  x[o]         - extract file(s) from the archive
 command specific modifiers:
  [a]          - put file(s) after [member-name]
  [b]          - put file(s) before [member-name] (same as [i])
  [D]          - use zero for timestamps and uids/gids
  [N]          - use instance [count] of name
  [f]          - truncate inserted file names
  [P]          - use full path names when matching
  [o]          - preserve original dates
  [u]          - only replace files that are newer than current archive contents
 generic modifiers:
  [c]          - do not warn if the library had to be created
  [s]          - create an archive index (cf. ranlib)
  [S]          - do not build a symbol table
  [T]          - make a thin archive
  [v]          - be verbose
  [V]          - display the version number
  @<file>      - read options from <file>
""" % env.getone('SCRIPT_NAME')
