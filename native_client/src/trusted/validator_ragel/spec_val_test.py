#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import StringIO
import sys

import objdump_parser
import spec
import spec_val
import test_format


class SpecValTestRunner(test_format.TestRunner):

  SECTION_NAME = 'spec'

  def GetSectionContent(self, options, sections):
    spec_val_cls = {
        32: spec_val.Validator32,
        64: spec_val.Validator64}[options.bits]

    instructions = []
    for line in StringIO.StringIO(sections['dis']):
      insn = objdump_parser.ParseLine(line)
      insn = objdump_parser.CanonicalizeInstruction(insn)
      instructions.append(insn)

    messages = spec_val_cls().Validate(instructions)

    if messages == []:
      return 'SAFE\n'

    return ''.join(
        '%x: %s\n' % (offset, message) for offset, message in messages)


def main(argv):
  SpecValTestRunner().Run(argv)


if __name__ == '__main__':
  main(sys.argv[1:])
