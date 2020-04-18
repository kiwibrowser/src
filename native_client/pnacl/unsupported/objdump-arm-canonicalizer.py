#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A simple tool for making objdump's disassemble dumps
for arm more canonical.
If two binaries have been generated with an almost identical code
generator, we expect the delta of the canoncalized dumps to be small
as well.
"""
import sys
import re


# keeps track of offset within a function
count = 0

PREDICATES = ["eq", "ne",
              "cs", "cc", "hs", "lo", # cs == hs, cc == lo
              "mi", "pl",
              "vs", "vc",
              "hi", "ls",
              "ge", "lt",
              "gt", "le",
              "",
              ]
BRANCHES = set(["b" + p for p in PREDICATES])
CALLS = set(["bl" + p for p in PREDICATES])

for line in sys.stdin:
  tokens = line.split()
  if re.search(r">:$", line):
    # we encountered a function beginning
    print "@@@@@@@@@@@@@@@", tokens[1]
    count = 0
  elif re.search(r"^ +[0-9a-f]+:", line):
    # we encountered an instruction, first strip the instruction address
    line = line[8:]
    opcode = tokens[2]
    if opcode in BRANCHES:
      # Rewrite:
      #    20104:  3a00000a        bcc     20134 <recurse+0x74>
      #       44:  3a00000a        bcc      <recurse+0x74>
      fr = r"(\s+" + opcode + r"\s+)[0-9a-f]+"
      to = r"\1"
      line = re.sub(fr, to, line)
    elif opcode in CALLS:
      # Rewrite:
      #    2001c:  eb00527f        bl      34a20 <__register_frame_info>
      #       1c:                  bl       <__register_frame_info>
      fr = r"[0-9a-f]+(\s+" + opcode + r"\s+)[0-9a-f]+"
      to = r"        \1"
      line = re.sub(fr, to, line)
    # replace the address which was stripped out above by an offset
    print "%8x" % count, line,
    count += 4
  else:
    # pass thru everything which is neither function beginning or instruction
    print line,
