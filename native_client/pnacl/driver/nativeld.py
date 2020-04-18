#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
#
# This is a thin wrapper for native LD. This is not meant to be
# used by the user, but is called from pnacl-translate.
# This implements the native linking part of translation.
#
# All inputs must be native objects or linker scripts.
#
# --pnacl-sb will cause the sandboxed LD to be used.
# The bulk of this file is logic to invoke the sandboxed translator.

import os
import subprocess

from driver_tools import CheckTranslatorPrerequisites, GetArch, ParseArgs, \
    Run, UnrecognizedOption
from driver_env import env
from driver_log import Log
import driver_tools
import elftools
import ldtools
import pathtools


EXTRA_ENV = {
  'INPUTS'   : '',
  'OUTPUT'   : '',

  # The INPUTS file coming from the llc translation step
  'LLC_TRANSLATED_FILE' : '',
  # Number of separate modules used for multi-threaded translation. This should
  # have been set by pnacl-translate, but default to 0 for checking.
  'SPLIT_MODULE' : '0',
  'USE_STDLIB': '1',

  # Upstream gold has the segment gap built in, but the gap can be modified
  # when not using the IRT. The gap does need to be at least one bundle so the
  # halt sled can be added for the TCB in case the segment ends up being a
  # multiple of 64k.
  # --eh-frame-hdr asks the linker to generate an .eh_frame_hdr section,
  # which is a presorted list of registered frames. This section is
  # used by libgcc_eh/libgcc_s to avoid doing the sort during runtime.
  # http://www.airs.com/blog/archives/462
  #
  # BE CAREFUL: anything added to LD_FLAGS should be synchronized with
  # flags used by the in-browser translator.
  # See: binutils/gold/nacl_file.cc
  'LD_FLAGS'    : '-nostdlib ' +
                  # Only relevant for ARM where it suppresses a warning.
                  # Ignored for other archs.
                  '--no-fix-cortex-a8 ' +
                  '--eh-frame-hdr ' +
                  # Give an error if any TEXTRELs occur.
                  '-z text ' +
                  # Ensure we don't accidentally get READ_IMPLIES_EXEC
                  # behaviour when building Linux Non-SFI executables.
                  '-z noexecstack ' +
                  '--build-id ',

  'SEARCH_DIRS'        : '${SEARCH_DIRS_USER} ${SEARCH_DIRS_BUILTIN}',
  'SEARCH_DIRS_USER'   : '',
  'SEARCH_DIRS_BUILTIN': '${USE_STDLIB ? ${LIBS_NATIVE_ARCH}/}',

  # Note: this is only used in the unsandboxed case
  'RUN_LD' : '${LD} ${LD_FLAGS} ${inputs} -o ${output}'
}

def PassThrough(*args):
  env.append('LD_FLAGS', *args)

LDPatterns = [
  ( '-o(.+)',          "env.set('OUTPUT', pathtools.normalize($0))"),
  ( ('-o', '(.+)'),    "env.set('OUTPUT', pathtools.normalize($0))"),

  ( '-nostdlib',       "env.set('USE_STDLIB', '0')"),

  ( '-L(.+)',
    "env.append('SEARCH_DIRS_USER', pathtools.normalize($0))"),
  ( ('-L', '(.*)'),
    "env.append('SEARCH_DIRS_USER', pathtools.normalize($0))"),

  # Note: we do not yet support all the combinations of flags which affect
  # layout of the various sections and segments because the corner cases in gold
  # may not all be worked out yet. They can be added (and tested!) as needed.
  ( '(-static)',                  PassThrough),
  ( '(-pie)',                     PassThrough),

  ( ('(-Ttext=.*)'),              PassThrough),
  ( ('(-Trodata=.*)'),            PassThrough),
  ( ('(-Ttext-segment=.*)'),      PassThrough),
  ( ('(-Trodata-segment=.*)'),    PassThrough),
  ( ('(--rosegment-gap=.*)'),     PassThrough),
  ( ('(--section-start)', '(.+)'),PassThrough),
  ( ('(--section-start=.*)'),     PassThrough),
  ( ('(-e)','(.*)'),              PassThrough),
  ( '(--entry=.*)',               PassThrough),
  ( '(-M)',                       PassThrough),
  ( '(-t)',                       PassThrough),
  ( ('-y','(.*)'),                PassThrough),
  ( ('(-defsym)','(.*)'),         PassThrough),
  ( '(-defsym=.*)',               PassThrough),
  ( '-export-dynamic',            PassThrough),

  ( '(--print-gc-sections)',      PassThrough),
  ( '(--gc-sections)',            PassThrough),
  ( '(--unresolved-symbols=.*)',  PassThrough),
  ( '(--dynamic-linker=.*)',      PassThrough),
  ( '(-g)',                       PassThrough),
  ( '(--build-id(?:=.+)?)',       PassThrough),

  ( '-melf_nacl',            "env.set('ARCH', 'X8632')"),
  ( ('-m','elf_nacl'),       "env.set('ARCH', 'X8632')"),
  ( '-melf64_nacl',          "env.set('ARCH', 'X8664')"),
  ( ('-m','elf64_nacl'),     "env.set('ARCH', 'X8664')"),
  ( '-marmelf_nacl',         "env.set('ARCH', 'ARM')"),
  ( ('-m','armelf_nacl'),    "env.set('ARCH', 'ARM')"),
  ( '-mmipselelf_nacl',      "env.set('ARCH', 'MIPS32')"),
  ( ('-m','mipselelf_nacl'), "env.set('ARCH', 'MIPS32')"),

  # Inputs and options that need to be kept in order
  ( '(--no-as-needed)',    "env.append('INPUTS', $0)"),
  ( '(--as-needed)',       "env.append('INPUTS', $0)"),
  ( '(--start-group)',     "env.append('INPUTS', $0)"),
  ( '(--end-group)',       "env.append('INPUTS', $0)"),
  ( '(-Bstatic)',          "env.append('INPUTS', $0)"),
  ( '(-Bdynamic)',         "env.append('INPUTS', $0)"),
  # This is the file passed from llc during translation (used to be via shmem)
  ( ('--llc-translated-file=(.*)'), "env.append('INPUTS', $0)\n"
                                    "env.set('LLC_TRANSLATED_FILE', $0)"),
  ( '-split-module=([0-9]+)', "env.set('SPLIT_MODULE', $0)"),
  ( '(--(no-)?whole-archive)', "env.append('INPUTS', $0)"),

  ( '(-l.*)',              "env.append('INPUTS', $0)"),
  ( '(--undefined=.*)',    "env.append('INPUTS', $0)"),

  ( '(-.*)',               UnrecognizedOption),
  ( '(.*)',                "env.append('INPUTS', pathtools.normalize($0))"),
]

def RemoveInterpProgramHeader(filename):
  headers = elftools.GetELFAndProgramHeaders(filename)
  assert headers
  ehdr, phdrs = headers
  for i, phdr in enumerate(phdrs):
    if phdr.type == elftools.ProgramHeader.PT_INTERP:
      fp = open(filename, 'rb+')
      fp.seek(ehdr.phoff + ehdr.phentsize * i)
      # Zero this program header. Note PT_NULL is 0.
      fp.write('\0' * ehdr.phentsize)
      fp.close()

def main(argv):
  env.update(EXTRA_ENV)

  ParseArgs(argv, LDPatterns)

  GetArch(required=True)
  inputs = env.get('INPUTS')
  output = env.getone('OUTPUT')

  if output == '':
    output = pathtools.normalize('a.out')

  # As we will modify the output file in-place for non-SFI, we output
  # the file to a temporary file first and then rename it. Otherwise,
  # build systems such as make assume the output file is ready even
  # if the last build failed during the in-place update.
  tmp_output = output + '.tmp'

  # Expand all parameters
  # This resolves -lfoo into actual filenames,
  # and expands linker scripts into command-line arguments.
  inputs = ldtools.ExpandInputs(inputs,
                                env.get('SEARCH_DIRS'),
                                True,
                                ldtools.LibraryTypes.NATIVE)

  env.push()
  env.set('inputs', *inputs)
  env.set('output', tmp_output)

  if env.getbool('SANDBOXED'):
    RunLDSandboxed()
  else:
    Run('${RUN_LD}')

  if env.getbool('NONSFI_NACL'):
    # Remove PT_INTERP in non-SFI binaries as we never use host's
    # dynamic linker/loader.
    #
    # This is necessary otherwise we get a statically linked
    # executable that is not directly runnable by Linux, because Linux
    # tries to load the non-existent file that PT_INTERP points to.
    #
    # This is fairly hacky.  It would be better if the linker provided
    # an option for omitting PT_INTERP (e.g. "--dynamic-linker ''").
    RemoveInterpProgramHeader(tmp_output)
  if driver_tools.IsWindowsPython() and os.path.exists(output):
    # On Windows (but not on Unix), the os.rename() call would fail if the
    # output file already exists.
    os.remove(output)
  os.rename(tmp_output, output)
  env.pop()
  # only reached in case of no errors
  return 0

def IsFlag(arg):
  return arg.startswith('-')

def RunLDSandboxed():
  if not env.getbool('USE_STDLIB'):
    Log.Fatal('-nostdlib is not supported by the sandboxed translator')
  CheckTranslatorPrerequisites()
  # The "main" input file is the application's combined object file.
  all_inputs = env.get('inputs')

  main_input = env.getone('LLC_TRANSLATED_FILE')
  if not main_input:
    Log.Fatal("Sandboxed LD requires one shm input file")

  outfile = env.getone('output')

  modules = int(env.getone('SPLIT_MODULE'))
  assert modules >= 1
  first_mainfile = all_inputs.index(main_input)
  first_extra = all_inputs.index(main_input) + modules
  # Have a list of just the split module files.
  llc_outputs = all_inputs[first_mainfile:first_extra]
  # Have a list of everything else.
  other_inputs = all_inputs[:first_mainfile] + all_inputs[first_extra:]

  native_libs_dirname = pathtools.tosys(GetNativeLibsDirname(other_inputs))
  command = [driver_tools.SelLdrCommand(),
             '-a'] # Allow file access
  driver_tools.AddListToEnv(command, 'NACL_IRT_PNACL_TRANSLATOR_LINK_INPUT',
                            llc_outputs)
  command.extend([
      '-E', 'NACL_IRT_PNACL_TRANSLATOR_LINK_OUTPUT=%s ' % outfile,
      '-E', 'NACL_IRT_OPEN_RESOURCE_BASE=%s' % native_libs_dirname,
      '-E', 'NACL_IRT_OPEN_RESOURCE_REMAP=%s'
      % 'libpnacl_irt_shim.a:libpnacl_irt_shim_dummy.a',
      '--', '${LD_SB}'])
  Run(' '.join(command),
      # stdout/stderr will be automatically dumped
      # upon failure
      redirect_stderr=subprocess.PIPE,
      redirect_stdout=subprocess.PIPE)


def GetNativeLibsDirname(other_inputs):
  """Check that native libs have a common directory and return the directory."""
  dirname = None
  for f in other_inputs:
    if IsFlag(f):
      continue
    else:
      if not pathtools.exists(f):
        Log.Fatal("Unable to open '%s'", pathtools.touser(f))
      if dirname is None:
        dirname = pathtools.dirname(f)
      else:
        if dirname != pathtools.dirname(f):
          Log.Fatal('Need a common directory for native libs: %s != %s',
                    dirname, pathtools.dirname(f))
  if not dirname:
    Log.Fatal('No native libraries found')
  return dirname + '/'
