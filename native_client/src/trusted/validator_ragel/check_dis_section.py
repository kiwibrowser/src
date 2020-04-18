#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import subprocess
import sys
import tempfile

import objdump_parser
import test_format


class RdfaTestRunner(test_format.TestRunner):

  SECTION_NAME = 'dis'

  def CommandLineOptions(self, parser):
    parser.add_option('--objdump',
                      help='Path to objdump')

  def GetSectionContent(self, options, sections):
    arch = {32: '-Mi386', 64: '-Mx86-64'}[options.bits]
    data = ''.join(test_format.ParseHex(sections['hex']))

    # TODO(shcherbina): get rid of custom prefix once
    # https://code.google.com/p/nativeclient/issues/detail?id=3631
    # is actually fixed.
    tmp = tempfile.NamedTemporaryFile(
        prefix='tmprdfa_', mode='wb', delete=False)
    try:
      tmp.write(data)
      tmp.close()

      objdump_proc = subprocess.Popen(
          [options.objdump,
           '-mi386', arch, '--target=binary',
           '--disassemble-all', '--disassemble-zeroes',
           '--insn-width=15',
           tmp.name],
          stdout=subprocess.PIPE,
          # On Windows, builds of binutils based on Cygwin end lines with
          # \n while builds of binutils based on MinGW end lines with \r\n.
          # The 'universal_newlines' feature makes this work with either one.
          universal_newlines=True)

      result = ''.join(objdump_parser.SkipHeader(objdump_proc.stdout))
      return_code = objdump_proc.wait()
      assert return_code == 0, 'error running objdump'

    finally:
      tmp.close()
      os.remove(tmp.name)

    return result


def main(argv):
  RdfaTestRunner().Run(argv)


if __name__ == '__main__':
  main(sys.argv[1:])
