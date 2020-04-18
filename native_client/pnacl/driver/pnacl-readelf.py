#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from driver_env import env
from driver_log import Log
import driver_tools
import filetype

EXTRA_ENV = {
  'INPUTS':     '',
  'FLAGS':      '',
}

PATTERNS = [
  ( '(-.*)',    "env.append('FLAGS', $0)"),
  ( '(.*)',     "env.append('INPUTS', pathtools.normalize($0))"),
]

def main(argv):
  env.update(EXTRA_ENV)
  driver_tools.ParseArgs(argv, PATTERNS)
  inputs = env.get('INPUTS')
  if len(inputs) == 0:
    Log.Fatal("No input files given")

  for infile in inputs:
    driver_tools.CheckPathLength(infile)
    env.push()
    env.set('input', infile)
    if filetype.IsLLVMBitcode(infile):
      # Hack to support newlib build.
      # Newlib determines whether the toolchain supports .init_array, etc., by
      # compiling a small test and looking for a specific section tidbit using
      # "readelf -S". Since pnacl compiles to bitcode, readelf isn't available.
      # (there is a line: "if ${READELF} -S conftest | grep -e INIT_ARRAY"
      # in newlib's configure file).
      # TODO(sehr): we may want to implement a whole readelf on bitcode.
      flags = env.get('FLAGS')
      if len(flags) == 1 and flags[0] == '-S':
        print 'INIT_ARRAY'
        return 0
      Log.Fatal('Cannot handle pnacl-readelf %s' % str(argv))
      return 1
    driver_tools.Run('"${READELF}" ${FLAGS} ${input}')
    env.pop()

  # only reached in case of no errors
  return 0

def get_help(unused_argv):
  return """
Usage: %s <option(s)> elf-file(s)
 Display information about the contents of ELF format files
 Options are:
  -a --all               Equivalent to: -h -l -S -s -r -d -V -A -I
  -h --file-header       Display the ELF file header
  -l --program-headers   Display the program headers
     --segments          An alias for --program-headers
  -S --section-headers   Display the sections' header
     --sections          An alias for --section-headers
  -g --section-groups    Display the section groups
  -t --section-details   Display the section details
  -e --headers           Equivalent to: -h -l -S
  -s --syms              Display the symbol table
      --symbols          An alias for --syms
  -n --notes             Display the core notes (if present)
  -r --relocs            Display the relocations (if present)
  -u --unwind            Display the unwind info (if present)
  -d --dynamic           Display the dynamic section (if present)
  -V --version-info      Display the version sections (if present)
  -A --arch-specific     Display architecture specific information (if any).
  -c --archive-index     Display the symbol/file index in an archive
  -D --use-dynamic       Use the dynamic section info when displaying symbols
  -x --hex-dump=<number|name>
                         Dump the contents of section <number|name> as bytes
  -p --string-dump=<number|name>
                         Dump the contents of section <number|name> as strings
  -R --relocated-dump=<number|name>
                         Dump the contents of section <number|name> as relocated bytes
  -w[lLiaprmfFsoR] or
  --debug-dump[=rawline,=decodedline,=info,=abbrev,=pubnames,=aranges,=macro,=frames,=str,=loc,=Ranges]
                         Display the contents of DWARF2 debug sections
  -I --histogram         Display histogram of bucket list lengths
  -W --wide              Allow output width to exceed 80 characters
  @<file>                Read options from <file>
  -H --help              Display this information
  -v --version           Display the version number of readelf

""" % env.getone('SCRIPT_NAME')
