#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# This code uses csmith to generate a large amount of .c files, change the entry
# point of each from main to entry_N and then create a new main.c file that
# would be linked to all of them. csmith needs to be built in CSMITH_HOME.
# This is used to generate large executables from csmith.
#
from __future__ import print_function
import os, fileinput, re, sys

CSMITH_HOME = os.environ['CSMITH_HOME']
CSMITH_EXE = CSMITH_HOME + '/src/csmith'
DEFAULT_N = 100

def shellrun(s):
  print(s)
  os.system(s)

def main(args):
  if len(args) > 0:
    N = int(args[0])
  else:
    N = DEFAULT_N

  n = 0
  while n < N:
    cobjname = '/tmp/entry_obj_{}.c'.format(n)
    entryname = 'entry_{}'.format(n)
    shellrun(CSMITH_EXE + ' --no-argc --max-funcs 40 --no-volatiles' +
                          ' --no-structs > ' + cobjname)

    # Retry if the file is too small
    if os.path.getsize(cobjname) < 400 * 1000:
      print('Redoing...')
    else:
      for line in fileinput.input(cobjname, inplace=True):
        fixed_line = re.sub('int main', 'int ' + entryname, line)
        sys.stdout.write(fixed_line)
      n += 1

  print('Creating main file')
  with open('/tmp/main.c', 'w') as of:
    for n in range(N):
      of.write('int entry_{}(void);\n'.format(n))
    of.write('int main(void) {\n')
    for n in range(N):
      of.write('  entry_{}();\n'.format(n))
    of.write('  return 0;\n}\n')

if __name__ == '__main__':
  main(sys.argv[1:])

