#!/usr/bin/python
#
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

import sys
import os
import textwrap
from subprocess import Popen, PIPE

_OBJDUMP = 'mips-linux-gnu-objdump'


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
            'at middle of an pseudo-instruction that must be executed in full',
            0, 0],
        'kProblemPatternCrossesBundle': ['This instruction is part of a '
            'sequence that must execute in full, but it spans a bundle edge '
            '-- so an indirect branch may target it',
            1, 1],
        'kProblemBranchInvalidDest': ['This branch targets a location that is '
            'outside of the application\'s executable code, over 256 MB',
            0, 0],
        'kProblemUnsafeLoadStore': ['This load/store instruction is not '
            'preceded by a valid store mask instruction',
            1, 0],
        'kProblemUnsafeLoadStoreThreadPointer': ['This is not a valid '
            'load/store instruction for accessing thread pointer.',
            0, 0],
        'kProblemUnsafeJumpRegister': ['This indirect jump instruction is not '
            'preceded by a valid jump mask instruction',
            1, 0],
        'kProblemUnsafeDataWrite': ['This instruction affects a register that '
            'must contain a valid data-region address (sp), but is not '
            'followed by a valid store mask instruction',
            0, 1],
        'kProblemReadOnlyRegister': ['This instruction changes the contents of'
            ' read-only register',
            0, 0],
        'kProblemMisalignedCall': ['This linking branch/jump instruction is '
            'not at the bundle offset +8, so when its RA result is masked, the'
            ' caller will not return to the next instruction (start of next '
            'bundle)',
            0, 0],
        'kProblemUnalignedJumpToTrampoline':['The destination of this '
            'jump/branch instruction is in trampoline code section but '
            'address is not bundle aligned',
            0, 0],
        'kProblemDataRegInDelaySlot':['This instruction changes value of a '
            'stack pointer but is located in the delay slot of jump/branch',
            1, 0],
        'kProblemBranchInDelaySlot':['This instruction is a jump/branch '
            'instruction and it is located in a delay slot of the previous '
            'jump/branch instruction',
            1, 0],

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


def main(argv):
    if len(argv) != 2:
        sys.stderr.write('Error, provide one parameter to the script.\n')
        return 1
    if not os.path.exists(sys.argv[1]):
        sys.stderr.write('Error, input file does not exist.\n')
        return 1
    for line in sys.stdin:
        if line.startswith('ncval: '):
            line = line[7:].strip()
            _explain_problem(sys.argv[1], *_parse_report(line))
    return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv))
