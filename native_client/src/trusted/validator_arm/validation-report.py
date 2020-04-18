#!/usr/bin/python
#
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#


import sys
import textwrap
from subprocess import Popen, PIPE

_OBJDUMP = 'arm-linux-gnueabi-objdump'

def _objdump(binary, vaddr, ctx_before, ctx_after):
    args = [
        _OBJDUMP,
        '-d',
        '-G',
        binary,
        '--start-address=0x%08X' % (vaddr - (4 * ctx_before)),
        '--stop-address=0x%08X' % (vaddr + 4 + (4 * ctx_after))]
    highlight = ctx_before
    lines = 0
    for line in Popen(args, stdout=PIPE).stdout.read().split('\n'):
        if line.startswith(' '):
            if highlight == 0:
                print '--> ', line
            else:
                print '    ', line
            highlight -= 1
            lines += 1
    if not lines:
        print '    (not found)'

def _problem_info(code):
    return {
        'kProblemUnsafe': ['Instruction is unsafe', 0, 0],
        'kProblemBranchSplitsPattern': ['The destination of this branch is '
            'part of an instruction sequence that must be executed in full, '
            'or is inline data',
            0, 0],
        'kProblemPatternCrossesBundle': ['This instruction is part of a '
            'sequence that must execute in full, but it spans a bundle edge '
            '-- so an indirect branch may target it',
            1, 1],
        'kProblemBranchInvalidDest': ['This branch targets a location that is '
            'outside of the application\'s executable code, and is not a valid '
            'trampoline entry point', 0, 0],
        'kProblemUnsafeLoadStore': ['This store instruction is not preceded by '
            'a valid address mask instruction', 1, 0],
        'kProblemUnsafeBranch': ['This indirect branch instruction is not '
            'preceded by a valid address mask instruction', 1, 0],
        'kProblemUnsafeDataWrite': ['This instruction affects a register that '
            'must contain a valid data-region address, but is not followed by '
            'a valid address mask instruction', 0, 1],
        'kProblemReadOnlyRegister': ['This instruction changes the contents of '
            'a read-only register', 0, 0],
        'kProblemMisalignedCall': ['This linking branch instruction is not in '
            'the last slot of its bundle, so when its LR result is masked, the '
            'caller will not return to the next instruction', 0, 0],
    }[code]

def _safety_msg(val):
    return {
        0: 'UNKNOWN',  # Should not appear
        1: 'is undefined',
        2: 'has unpredictable effects',
        3: 'is deprecated',
        4: 'is forbidden',
        5: 'uses forbidden operands',
    }[val]

def _explain_problem(binary, vaddr, safety, code, ref_vaddr):
    msg, ctx_before, ctx_after = _problem_info(code)
    if safety == 6:
        msg = "At %08X: %s:" % (vaddr, msg)
    else:
        msg = ("At %08X: %s (%s):"
            % (vaddr, msg, _safety_msg(safety)))
    print '\n'.join(textwrap.wrap(msg, 70, subsequent_indent='  '))
    _objdump(binary, vaddr, ctx_before, ctx_after)
    if ref_vaddr:
        print "Destination address %08X:" % ref_vaddr
        _objdump(binary, ref_vaddr, 1, 1)

def _parse_report(line):
    vaddr_hex, safety, code, ref_vaddr_hex = line.split()
    return (int(vaddr_hex, 16), int(safety), code, int(ref_vaddr_hex, 16))


for line in sys.stdin:
    if line.startswith('ncval: '):
      line = line[7:].strip()
      _explain_problem(sys.argv[1], *_parse_report(line))
