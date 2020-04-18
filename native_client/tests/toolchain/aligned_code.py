# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import subprocess
import sys
import re

def Main(args):
  nargs = len(args)
  outfile = None
  if nargs == 3:
    outfile = args[2]
  else:
    assert(nargs == 2)
  objdump = args[0]
  obj_file = args[1]

  regex = re.compile(r"""\s* LOAD \s+ off \s+ (0x[0-9a-f]+) \s+
                         vaddr \s+ (0x[0-9a-f]+) \s+""",
                     re.VERBOSE)
  regex2 = re.compile(r"""\s* filesz \s+ (0x[0-9a-f]+) \s+
                              memsz \s+ (0x[0-9a-f]+) \s+
                              flags \s+ r-x""",
                      re.VERBOSE)

  p_off = None
  p_vaddr = None
  p_filesz = None
  p_memsz = None

  objdump_args = [objdump, '-w', '-p', obj_file]
  proc = subprocess.Popen(objdump_args,
                          stdout=subprocess.PIPE,
                          bufsize=-1)
  for line in proc.stdout:
    match = regex.match(line)
    if match:
      line = proc.stdout.next()
      match2 = regex2.match(line)
      if match2:
        p_off = int(match.group(1), 16)
        p_vaddr = int(match.group(2), 16)
        p_filesz = int(match2.group(1), 16)
        p_memsz = int(match2.group(2), 16)
        break
  if proc.wait() != 0:
    print 'Command failed: %s' % objdump_args
    sys.exit(1)

  if p_off is None:
    print 'Failed to find executable segment in output of %s' % objdump_args
    sys.exit(1)

  failures = []
  def CheckMisaligned(value, which):
    if value % 0x10000 != 0:
      failures.append('misaligned %s %#x' % (which, value))

  CheckMisaligned(p_off, 'p_off')
  CheckMisaligned(p_vaddr, 'p_vaddr')
  CheckMisaligned(p_filesz, 'p_filesz')
  CheckMisaligned(p_memsz, 'p_memsz')

  if failures:
    failures = '\n'.join(failures)
    print failures
    sys.exit(1)

  if outfile is not None:
    outf = open(outfile, 'w')
    outf.write('segments in %s OK\n' % obj_file)
    outf.close()


if __name__ == '__main__':
  Main(sys.argv[1:])
