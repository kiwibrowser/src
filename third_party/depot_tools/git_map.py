#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Enhances `git log --graph` view with information on commit branches + tags that
point to them. Items are colorized as follows:

  * Cyan    - Currently checked out branch
  * Green   - Local branch
  * Red     - Remote branches
  * Magenta - Tags
  * White   - Merge Base Markers
  * Blue background - The currently checked out commit
"""

import sys

import subprocess2

from git_common import current_branch, branches, tags, get_config_list, GIT_EXE
from git_common import get_or_create_merge_base, root

from third_party import colorama

CYAN = colorama.Fore.CYAN
GREEN = colorama.Fore.GREEN
MAGENTA = colorama.Fore.MAGENTA
RED = colorama.Fore.RED
WHITE = colorama.Fore.WHITE

BLUEBAK = colorama.Back.BLUE

BRIGHT = colorama.Style.BRIGHT
RESET = colorama.Fore.RESET + colorama.Back.RESET + colorama.Style.RESET_ALL

# Git emits combined color
BRIGHT_RED = '\x1b[1;31m'


def print_help():
  names = {
    'Cyan': CYAN,
    'Green': GREEN,
    'Magenta': MAGENTA,
    'Red': RED,
    'White': WHITE,
    'Blue background': BLUEBAK,
  }
  msg = "usage: git map [-h] [<args>]\n"

  for line in __doc__.splitlines():
    for key in names.keys():
      if key in line:
        msg += line.replace('* ', '* ' + names[key])+RESET+'\n'
        break
    else:
      msg += line + '\n'
  sys.stdout.write(msg)


def main(argv):
  if '-h' in argv:
    print_help()
    return 0

  map_extra = get_config_list('depot_tools.map_extra')
  fmt = '%C(red bold)%h%x09%Creset%C(green)%d%Creset %C(yellow)%cd%Creset ~ %s'
  log_proc = subprocess2.Popen(
    [GIT_EXE, 'log', '--graph', '--branches', '--tags', root(),
     '--color=always', '--date=short', ('--pretty=format:' + fmt)
    ] + map_extra + argv,
    stdout=subprocess2.PIPE,
    shell=False)

  current = current_branch()
  all_branches = set(branches())
  merge_base_map = {b: get_or_create_merge_base(b) for b in all_branches}
  merge_base_map = {b: v for b, v in merge_base_map.iteritems() if v}
  if current in all_branches:
    all_branches.remove(current)
  all_tags = set(tags())
  try:
    for line in log_proc.stdout.xreadlines():
      if merge_base_map:
        commit = line[line.find(BRIGHT_RED)+len(BRIGHT_RED):line.find('\t')]
        base_for_branches = set()
        for branch, sha in merge_base_map.iteritems():
          if sha.startswith(commit):
            base_for_branches.add(branch)
        if base_for_branches:
          newline = '\r\n' if line.endswith('\r\n') else '\n'
          line = line.rstrip(newline)
          line += ''.join(
              (BRIGHT, WHITE, '    <(%s)' % (', '.join(base_for_branches)),
               RESET, newline))
          for b in base_for_branches:
            del merge_base_map[b]

      start = line.find(GREEN+' (')
      end   = line.find(')', start)
      if start != -1 and end != -1:
        start += len(GREEN) + 2
        branch_list = line[start:end].split(', ')
        branches_str = ''
        if branch_list:
          colored_branches = []
          head_marker = ''
          for b in branch_list:
            if b == "HEAD":
              head_marker = BLUEBAK+BRIGHT+'*'
              continue
            if b == current:
              colored_branches.append(CYAN+BRIGHT+b+RESET)
              current = None
            elif b in all_branches:
              colored_branches.append(GREEN+BRIGHT+b+RESET)
              all_branches.remove(b)
            elif b in all_tags:
              colored_branches.append(MAGENTA+BRIGHT+b+RESET)
            elif b.startswith('tag: '):
              colored_branches.append(MAGENTA+BRIGHT+b[5:]+RESET)
            else:
              colored_branches.append(RED+b)
            branches_str = '(%s) ' % ((GREEN+", ").join(colored_branches)+GREEN)
          line = "%s%s%s" % (line[:start-1], branches_str, line[end+5:])
          if head_marker:
            line = line.replace('*', head_marker, 1)
      sys.stdout.write(line)
  except (IOError, KeyboardInterrupt):
    pass
  finally:
    sys.stderr.close()
    sys.stdout.close()
  return 0


if __name__ == '__main__':
  try:
    sys.exit(main(sys.argv[1:]))
  except KeyboardInterrupt:
    sys.stderr.write('interrupted\n')
    sys.exit(1)
