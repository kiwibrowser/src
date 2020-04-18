#!/usr/bin/python

# Copyright 2010 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import getopt
import sys

import elf

USAGE = """Usage: elf_checker [options]* [filename]+

This tools verifies alignment attributes of segments in elf binaries.
If no attribute is specified via '-a' each segment in the binary is dumped.
Otherwise, all options must be specified.

Options:
   -a (vaddr|paddr|memsz|filesz):  select the segement attribute to be checked
   -s <name>:                      only consider segments with this name
   -f <flag>:                      only consider segments with this flag value
   -d <divisor>                    divide segment attribute by this number
   -r <remainder>                  check division for this remainder
"""


def Fatal(mesg):
  print >>sys.stderr, mesg
  sys.exit(1)


def DumpSegments(elf_obj):
  for n, s in enumerate(elf_obj.PhdrList()):
    print n, str(s)


def CheckSegments(elf_obj, attribute, segment, flags, divisor, remainder):
  def ismatch(s):
    return s.TypeName() == segment and s.flags == flags

  filtered_segments = [p for p in elf_obj.PhdrList() if ismatch(p)]
  if not filtered_segments:
    Fatal('no matching segments found')

  for s in filtered_segments:
    print 'found: ', str(s)
    val = s.__dict__[attribute]
    result = val % divisor
    if result != remainder:
      Fatal("%x %% %x == %x (expected %x)" % (val, divisor, result, remainder))


def main(argv):
  try:
    opts, args = getopt.getopt(argv[1:], 'a:s:d:r:f:')
  except getopt.error, e:
    Fatal(str(e) + '\n' + USAGE)

  flags = None
  remainder = None
  divisor = None
  attribute = None
  segment = None

  for opt, val in opts:
    if opt == '-r':
      remainder = int(val, 0)
    elif opt == '-s':
      segment = val
    elif opt == '-d':
      divisor = int(val, 0)
    elif opt == '-a':
      attribute = val
    elif opt == '-f':
      flags = int(val, 0)
    else:
      assert False

  if not args:
    Fatal('no files specified')

  if attribute is not None:
    if attribute not in ['vaddr', 'paddr', 'memsz', 'filesz']:
      Fatal('unknown attribute: %s' % attribute)
    if segment is None:
      Fatal('you must specify a segment name via -s')
    if flags is None:
      Fatal('you must specify segment flags via -f')
    if divisor is None:
      Fatal('you must specify a divisor via -d')
    if remainder is None:
      Fatal('you must specify a remainder via -r')

  for filename in args:
    data = open(filename).read()
    elf_obj = elf.Elf(data)
    if attribute is None:
      DumpSegments(elf_obj)
    else:
      CheckSegments(elf_obj, attribute, segment, flags, divisor, remainder)

  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv))
