#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import driver_tools
import filetype
import shutil
import pathtools
from driver_env import env
from driver_log import Log

EXTRA_ENV = {
  'INPUTS'             : '',
  'OUTPUT'             : '',
  'MODE'               : 'all',

  'OPT_FLAGS_all'      : '-disable-opt --strip',
  'OPT_FLAGS_debug'    : '-disable-opt --strip-debug',
  'OPT_FLAGS_unneeded' : '-disable-opt --strip',
  'STRIP_FLAGS_all'    : '-s',
  'STRIP_FLAGS_debug'  : '-S',
  'STRIP_FLAGS_unneeded': '--strip-unneeded',

  'OPT_FLAGS'          : '${OPT_FLAGS_%MODE%}',
  'STRIP_FLAGS'        : '${STRIP_FLAGS_%MODE%}',

  'RUN_OPT'            : '${LLVM_OPT} ${OPT_FLAGS} ${input} -o ${output}',
  'RUN_STRIP'          : '${STRIP} ${STRIP_FLAGS} ${input} -o ${output}',
}

StripPatterns = [
    ( ('-o','(.*)'),     "env.set('OUTPUT', pathtools.normalize($0))"),
    ( ('-o','(.*)'),     "env.set('OUTPUT', pathtools.normalize($0))"),

    ( '--strip-all',     "env.set('MODE', 'all')"),
    ( '-s',              "env.set('MODE', 'all')"),

    ( '--strip-debug',   "env.set('MODE', 'debug')"),
    ( '-S',              "env.set('MODE', 'debug')"),
    ( '-g',              "env.set('MODE', 'debug')"),
    ( '-d',              "env.set('MODE', 'debug')"),

    ( '--strip-unneeded',"env.set('MODE', 'unneeded')"),

    ( '(-p)',            "env.append('STRIP_FLAGS', $0)"),
    ( '(--info)',        "env.append('STRIP_FLAGS', $0)"),

    ( '(-.*)',           driver_tools.UnrecognizedOption),

    ( '(.*)',            "env.append('INPUTS', pathtools.normalize($0))"),
]

def main(argv):
  env.update(EXTRA_ENV)
  driver_tools.ParseArgs(argv, StripPatterns)
  inputs = env.get('INPUTS')
  output = env.getone('OUTPUT')
  for path in inputs + [output]:
    driver_tools.CheckPathLength(path)

  if len(inputs) > 1 and output != '':
    Log.Fatal('Cannot have -o with multiple inputs')

  if '--info' in env.get('STRIP_FLAGS'):
    code, _, _ = driver_tools.Run('${STRIP} ${STRIP_FLAGS}')
    return code

  for f in inputs:
    if output != '':
      f_output = output
    else:
      f_output = f
    if filetype.IsPNaClBitcode(f):
      # PNaCl-format bitcode has no symbols, i.e. it is already stripped.
      if f != f_output:
        shutil.copyfile(f, f_output)
    elif filetype.IsLLVMBitcode(f):
      driver_tools.RunWithEnv('${RUN_OPT}', input=f, output=f_output)
    elif filetype.IsELF(f) or filetype.IsNativeArchive(f):
      driver_tools.RunWithEnv('${RUN_STRIP}', input=f, output=f_output)
    elif filetype.IsBitcodeArchive(f):
      # The strip tool supports native archives, but it does not support the
      # LLVM gold plugin so cannot handle bitcode.  There is also no bitcode
      # tool like opt that support archives.
      Log.Fatal('%s: strip does not support bitcode archives',
                pathtools.touser(f))
    else:
      Log.Fatal('%s: File is neither ELF, nor bitcode', pathtools.touser(f))
  return 0


def get_help(unused_argv):
  script = env.getone('SCRIPT_NAME')
  return """Usage: %s <option(s)> in-file(s)
  Removes symbols and sections from bitcode or native code.

  The options are:
  -p --preserve-dates              Copy modified/access timestamps to the output
                                   (only native code for now)
  -s --strip-all                   Remove all symbol and relocation information
  -g -S -d --strip-debug           Remove all debugging symbols & sections
  --strip-unneeded                 Remove all symbols not needed by relocations
  -h --help                        Display this output
     --info                        List object formats & architectures supported
  -o <file>                        Place stripped output into <file>

%s: supported targets: bitcode, native code (see --info).
""" % (script, script)
