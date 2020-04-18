#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import driver_tools
import filetype
import ldtools
import multiprocessing
import os
from driver_env import env
from driver_log import Log
from driver_temps import TempFiles

import subprocess

EXTRA_ENV = {
  'TRANSLATE_PSO': '0',
  'PIC': '${NONSFI_NACL || TRANSLATE_PSO ? 1 : 0}',

  # Determine if we should build nexes compatible with the IRT
  'USE_IRT' : '1',

  # Use the IRT shim by default. This can be disabled with an explicit
  # flag (--noirtshim) or via -nostdlib.
  'USE_IRT_SHIM'  : '1',

  # To simulate the sandboxed translator better and avoid user surprises,
  # reject LLVM bitcode (non-finalized) by default, accepting only PNaCl
  # (finalized) bitcode. --allow-llvm-bitcode-input has to be passed
  # explicitly to override this.
  'ALLOW_LLVM_BITCODE_INPUT': '0',

  # Flags for nativeld
  'LD_FLAGS': '',

  'USE_STDLIB'     : '1',
  'USE_DEFAULTLIBS': '1',
  'FAST_TRANSLATION': '0',

  'INPUTS'        : '',
  'OUTPUT'        : '',
  'OUTPUT_TYPE'   : '',

  # Library Strings
  'LD_ARGS' : '${USE_STDLIB ? ${LD_ARGS_normal} : ${LD_ARGS_nostdlib}}',

  # Note: we always require a shim now, but the dummy shim is not doing
  # anything useful.
  # libpnacl_irt_shim.a is generated during the SDK packaging not
  # during the toolchain build and there are hacks in pnacl/driver/ldtools.py
  # and pnacl/driver/nativeld.py that will fall back to
  # libpnacl_irt_shim_dummy.a if libpnacl_irt_shim.a does not exist.
  'LD_ARGS_IRT_SHIM': '-l:libpnacl_irt_shim.a',
  'LD_ARGS_IRT_SHIM_DUMMY': '-l:libpnacl_irt_shim_dummy.a',
  # In addition to specifying the entry point, we also specify an undefined
  # reference to _start, which is called by the shim's entry function,
  # __pnacl_wrapper_start. _start normally comes from libnacl and will be in
  # the pexe, however for the IRT it comes from irt_entry.c and when linking it
  # using native object files, this reference is required to make sure it gets
  # pulled in from the archive.
  'LD_ARGS_ENTRY':
      '${TRANSLATE_PSO ? --entry=__pnacl_pso_root : '
      '  ${NONSFI_NACL && !USE_IRT '
      '    ? --entry=__pnacl_start_linux : --entry=__pnacl_start} '
      '  --undefined=_start}',

  'CRTBEGIN': '-l:crtbegin.o',
  'CRTEND': '-l:crtend.o',

  'LD_ARGS_nostdlib': '-nostdlib ${ld_inputs}',

  # These are just the dependencies in the native link.
  'LD_ARGS_normal':
    '${!TRANSLATE_PSO ? ${CRTBEGIN}} ' +
    '${ld_inputs} ' +
    '${USE_IRT_SHIM ? ${LD_ARGS_IRT_SHIM} : ${LD_ARGS_IRT_SHIM_DUMMY}} ' +
    '--start-group ' +
    '${USE_DEFAULTLIBS ? ${DEFAULTLIBS}} ' +
    '--end-group ' +
    '${CRTEND}',

  'DEFAULTLIBS': '-l:libgcc.a -l:libcrt_platform.a ',

  # BE CAREFUL: anything added here can introduce skew between
  # the pnacl-translate commandline tool and the in-browser translator.
  # See: llvm/tools/pnacl-llc/srpc_main.cpp and
  # Chromium's plugin/pnacl_translate_thread.cc
  'LLC_FLAGS_COMMON': '${PIC ? -relocation-model=pic} ' +
                      #  -force-tls-non-pic makes the code generator (llc)
                      # do the work that would otherwise be done by
                      # linker rewrites which are quite messy in the nacl
                      # case and hence have not been implemented in gold
                      '${PIC ? -force-tls-non-pic} ',

  # LLC flags which set the target and output type.
  'LLC_FLAGS_TARGET' : '-mtriple=${TRIPLE} -filetype=${outfiletype}',

  # Append additional non-default flags here.
  # BE CAREFUL: anything added here can introduce skew between
  # the pnacl-translate commandline tool and the in-browser translator.
  # See: llvm/tools/pnacl-llc/srpc_main.cpp and
  # Chromium's plugin/pnacl_translate_thread.cc
  'LLC_FLAGS_EXTRA' : '${FAST_TRANSLATION ? ${LLC_FLAGS_FAST}} ' +
                      '${#OPT_LEVEL ? -O${OPT_LEVEL}} ' +
                      '${OPT_LEVEL == 0 ? -disable-fp-elim}',
  # The output type for subzero.
  'SZ_FLAGS_TARGET': '--filetype=${outfiletype}',
  # Additional subzero flags that need to be specified to both the
  # host and the sandboxed pnacl-sz.
  'SZ_FLAGS_EXTRA' : '',

  # Opt level from command line (if any)
  'OPT_LEVEL' : '',

  # faster translation == slower code
  'LLC_FLAGS_FAST' : '-O0'
                     # This, surprisingly, makes a measurable difference
                     ' -tail-merge-threshold=20',

  'LLC_FLAGS': '${LLC_FLAGS_TARGET} ${LLC_FLAGS_COMMON} ${LLC_FLAGS_ARCH} ' +
               '${LLC_FLAGS_EXTRA}',

  # Note: this is only used in the unsandboxed case
  'RUN_LLC'       : '${LLVM_PNACL_LLC} ${LLC_FLAGS} ${LLC_MCPU} '
                    '${input} -o ${output} ',
  'RUN_SZ': '${LLVM_PNACL_SZ} ${SZ_FLAGS_ARCH} ${SZ_FLAGS_TARGET} '
            '${SZ_FLAGS_EXTRA} ${input} -o ${output}',
  # Whether to stream the bitcode from a single FD in unsandboxed mode
  # (otherwise it will use concurrent file reads when using multithreaded module
  # splitting)
  'STREAM_BITCODE' : '0',
  # Default to 'auto', which means unset by the user. In this case the driver
  # will use up to 4 modules if there are enough cores. If the user overrides,
  # use the number of modules as specified (which must be at least 1).
  'SPLIT_MODULE' : 'auto',
  # Module split scheduling. 'dynamic' will produce non-deterministic results
  # with faster compilation, whereas 'static' will still use multiple cores but
  # will be deterministic and slightly slower.
  'SPLIT_MODULE_SCHED' : '${SANDBOXED ? dynamic : static}',
  # Whether to (try to) use pnacl-sz for translation instead of pnacl-llc.
  'USE_SZ' : '0',
  # Whether an option has been specified that Subzero can't (yet) handle.
  'SZ_UNSUPPORTED' : '0',
  # Subzero equivalent of SPLIT_MODULE, i.e. default # of translation threads.
  'SZ_THREADS' : '0',
}


TranslatorPatterns = [
  ( '-o(.+)',          "env.set('OUTPUT', pathtools.normalize($0))"),
  ( ('-o', '(.+)'),    "env.set('OUTPUT', pathtools.normalize($0))"),

  ( '-pso',            "env.set('TRANSLATE_PSO', '1')"),

  ( '-S',              "env.set('OUTPUT_TYPE', 's')"), # Stop at .s
  ( '-c',              "env.set('OUTPUT_TYPE', 'o')"), # Stop at .o

  # Expose a very limited set of llc flags.
  # BE CAREFUL: anything added here can introduce skew between
  # the pnacl-translate commandline tool and the in-browser translator.
  # See: llvm/tools/pnacl-llc/srpc_main.cpp and
  # Chromium's plugin/pnacl_translate_thread.cc
  ( '(-sfi-.+)',        "env.append('LLC_FLAGS_EXTRA', $0)\n"
                        "env.set('SZ_UNSUPPORTED', '1')"),
  ( '(-mtls-use-call)', "env.append('LLC_FLAGS_EXTRA', $0)"),
  ( '(-force-align-stack)', "env.append('LLC_FLAGS_EXTRA', $0)\n"
                            "env.set('SZ_UNSUPPORTED', '1')"),
  # These flags are usually used for linktime dead code/data
  # removal but also help with reloc overflows on ARM
  ( '(-fdata-sections)',     "env.append('LLC_FLAGS_EXTRA', '-data-sections')\n"
                             "env.append('SZ_FLAGS_EXTRA', $0)"),
  ( '(-ffunction-sections)',
    "env.append('LLC_FLAGS_EXTRA', '-function-sections')\n"
    "env.append('SZ_FLAGS_EXTRA', $0)"),
  ( '(--gc-sections)',       "env.append('LD_FLAGS', $0)"),
  ( '(-mattr=.*)', "env.append('LLC_FLAGS_EXTRA', $0)\n"
                   "env.append('SZ_FLAGS_EXTRA', $0)"),
  ( '(-mcpu=.*)', "env.set('LLC_MCPU', '')\n"
                  "env.append('LLC_FLAGS_EXTRA', $0)"),
  ( '(-pnaclabi-verify=.*)', "env.append('LLC_FLAGS_EXTRA', $0)"),
  ( '(-pnaclabi-verify-fatal-errors=.*)', "env.append('LLC_FLAGS_EXTRA', $0)"),
  # Allow overriding the -O level.
  ( '-O([0-3])', "env.set('OPT_LEVEL', $0)"),

  # This adds arch specific flags to the llc invocation aimed at
  # improving translation speed at the expense of code quality.
  ( '-translate-fast',  "env.set('FAST_TRANSLATION', '1')"),
  # Allow Subzero.
  ( '--use-sz', "env.set('USE_SZ', '1')"),

  ( '-nostdlib',       "env.set('USE_STDLIB', '0')"),

  # Disables the default libraries.
  # This flag is needed for building libgcc_s.so.
  ( '-nodefaultlibs',  "env.set('USE_DEFAULTLIBS', '0')"),

  ( '--noirt',         "env.set('USE_IRT', '0')"),
  ( '--noirtshim',     "env.set('USE_IRT_SHIM', '0')"),

  ( '--allow-llvm-bitcode-input',
    "env.set('ALLOW_LLVM_BITCODE_INPUT', '1')\n"
    "env.set('SZ_UNSUPPORTED', '1')"),

  ( '-fPIC',           "env.set('PIC', '1')\n"
                       "env.set('SZ_UNSUPPORTED', '1')"),

  ( '(--build-id(?:=.+)?)',    "env.append('LD_FLAGS', $0)"),
  ( '-(split-module|threads)=([0-9]+|auto|seq)', "env.set('SPLIT_MODULE', $1)"),
  ( '-split-module-sched=(.*)', "env.set('SPLIT_MODULE_SCHED', $0)"),
  ( '-stream-bitcode', "env.set('STREAM_BITCODE', '1')"),

  # Treat general linker flags as inputs so they don't get re-ordered
  ( '-Wl,(.*)',        "env.append('INPUTS', *($0).split(','))"),

  ( '(-.*)',            driver_tools.UnrecognizedOption),
  ( '(.*)',            "env.append('INPUTS', pathtools.normalize($0))"),
]


def SetUpArch():
  base_arch = env.getone('BASE_ARCH')
  env.set('TARGET_OS', 'nacl')
  if base_arch.endswith('_LINUX'):
    base_arch = base_arch[:-len('_LINUX')]
    env.set('TARGET_OS', 'linux')
  elif base_arch.endswith('_MAC'):
    base_arch = base_arch[:-len('_MAC')]
    env.set('TARGET_OS', 'mac')

  if env.getbool('NONSFI_NACL'):
    triple_map = {
        'nacl':
            {'X8632': 'i686-linux-gnu',
             'ARM': 'armv7a-linux-gnueabihf'}}
  else:
    triple_map = {
        'nacl':
            {'X8632': 'i686-none-nacl-gnu',
             'X8664': 'x86_64-none-nacl-gnu',
             'ARM': 'armv7a-none-nacl-gnueabihf',
             'MIPS32': 'mipsel-none-nacl-gnu'},
        'linux':
            {'X8632': 'i686-linux-gnu',
             'X8664': 'x86_64-linux-gnux32'},
        'mac': {'X8632': 'i686-apple-darwin'}}
  env.set('TRIPLE', triple_map[env.getone('TARGET_OS')][base_arch])

  # CPU that is representative of baseline feature requirements for NaCl
  # and/or chrome.  We may want to make this more like "-mtune"
  # by specifying both "-mcpu=X" and "-mattr=+feat1,-feat2,...".
  # Note: this may be different from the in-browser translator, which may
  # do auto feature detection based on CPUID, but constrained by what is
  # accepted by NaCl validators.
  cpu_map = {
      'X8632': 'pentium4m',
      'X8664': 'x86-64',
      'ARM': 'cortex-a9',
      'MIPS32': 'mips32r2'}
  env.set('LLC_MCPU', '-mcpu=%s' % cpu_map[base_arch])

  llc_flags_map = {
      'ARM': ['-float-abi=hard', '-mattr=+neon'],
      'ARM_NONSFI': ['-float-abi=hard', '-arm-enable-dwarf-eh=1'],
      # To translate x86-32 binary, we set -malign-double option so that the
      # backend's datalayout matches the datalayout for "le32" used by the
      # frontend. The le32 datalayout uses 8-byte alignment for the types i64
      # and double. i386's datalayout usually uses only 4-byte alignment for
      # these types, but -malign-double changes that to 8-byte alignment.
      # This is only needed when translating LLVM IR that hasn't had PNaCl's IR
      # simplification passes applied to it.
      'X8632_NONSFI': ['-malign-double'],
      }
  env.set('LLC_FLAGS_ARCH', *llc_flags_map.get(env.getone('ARCH'), []))
  env.set('SZ_FLAGS_ARCH', '')
  # When linking against a host OS's libc (such as Linux glibc), don't
  # use %gs:0 to read the thread pointer because that won't be
  # compatible with the libc's use of %gs:0.  Similarly, Non-SFI Mode
  # currently offers no optimized path for reading the thread pointer.
  if env.getone('TARGET_OS') != 'nacl' or env.getbool('NONSFI_NACL'):
    env.append('LLC_FLAGS_ARCH', '-mtls-use-call')
  # For Subzero, determine -target, -sandbox, and -nonsfi options.
  is_sandbox = '0'
  is_nonsfi = '0'
  if env.getone('TARGET_OS') == 'nacl':
    if env.getbool('NONSFI_NACL'):
      is_nonsfi = '1'
    else:
      is_sandbox = '1'

  # Subzero arch setup below.
  if base_arch not in ('X8632', 'X8664', 'ARM'):
    env.set('SZ_UNSUPPORTED', '1')
    # Hard-fail on an unsupported architecture.
    if env.getbool('USE_SZ'):
      Log.Fatal('Unsupported architecture when using --sz: ' + base_arch)
    return

  sz_target = {
      'arm': 'arm32',
      'x8632': 'x8632',
      'x8664': 'x8664',
  }[base_arch.lower()]
  env.append('SZ_FLAGS_ARCH', '--sandbox=' + is_sandbox)
  env.append('SZ_FLAGS_ARCH', '--nonsfi=' + is_nonsfi)
  env.append('SZ_FLAGS_ARCH', '--target=' + sz_target)
  # This is a fine place to map OPT_LEVEL to the Subzero equivalent, with
  # default of -O2.
  sz_opt_map = {
    '0': '-Om1',
    '1': '-O2',
    '2': '-O2',
    }
  env.append('SZ_FLAGS_EXTRA', sz_opt_map.get(env.getone('OPT_LEVEL'), '-O2'))
  # At this point, the only Subzero options left to set are -o, -filetype, and
  # -threads.


def SetUpLinkOptions():
  if env.getbool('TRANSLATE_PSO'):
    # Using "-pie" rather than "-shared" has the effect of suppressing the
    # creation of a PLT and R_*_JUMP_SLOT relocations, which come from the
    # external symbol references that multi-threaded translation produces.
    env.append('LD_FLAGS', '-pie')
    return

  if env.getbool('NONSFI_NACL'):
    # "_begin" allows a PIE to find its load address in order to apply
    # dynamic relocations.
    env.append('LD_FLAGS', '-defsym=_begin=0')
    env.append('LD_FLAGS', '-pie')
  else:
    env.append('LD_FLAGS', '-static')
    # Give non-IRT builds 12MB of text before starting rodata instead of
    # the larger default gap. The gap cannot be too small (e.g., 0) because
    # sel_ldr requires space for adding a halt sled.
    if not env.getbool('USE_IRT'):
      env.append('LD_FLAGS', '--rosegment-gap=0xc00000')


def main(argv):
  env.update(EXTRA_ENV)
  driver_tools.ParseArgs(argv, TranslatorPatterns)
  driver_tools.GetArch(required = True)
  SetUpArch()
  SetUpLinkOptions()

  # Now commit to whether or not Subzero is used.
  use_sz = env.getbool('USE_SZ') and not env.getbool('SZ_UNSUPPORTED')

  inputs = env.get('INPUTS')
  output = env.getone('OUTPUT')

  if len(inputs) == 0:
    Log.Fatal("No input files")
  for path in inputs:
    driver_tools.CheckPathLength(path)

  if output == '':
    Log.Fatal("Please specify output file with -o")

  # Find the bitcode file on the command line.
  bcfiles = [f for f in inputs
             if not ldtools.IsFlag(f) and
               (filetype.IsPNaClBitcode(f)
                or filetype.IsLLVMBitcode(f)
                or filetype.FileType(f) == 'll')]
  if len(bcfiles) > 1:
    Log.Fatal('Expecting at most 1 bitcode file')
  elif len(bcfiles) == 1:
    bcfile = bcfiles[0]
  else:
    bcfile = None

  if ((env.getbool('ALLOW_LLVM_BITCODE_INPUT') or
       env.getone('TARGET_OS') != 'nacl' or
       env.getbool('USE_EMULATOR')) and
      env.getone('SPLIT_MODULE') == 'auto'):
    # When llvm input is allowed, the pexe may not be ABI-stable, so do not
    # split it.  Non-ABI-stable pexes may have symbol naming and visibility
    # issues that the current splitting scheme doesn't account for.
    #
    # For now, also do not enable multi-threaded translation when TARGET_OS !=
    # 'nacl', since in these cases we will be using the host toolchain's
    # linker.
    #
    # The x86->arm emulator is very flaky when threading is used, so don't
    # do module splitting when using it.
    env.set('SPLIT_MODULE', 'seq')
  # Do not set -streaming-bitcode for sandboxed mode, because it is already
  # in the default command line.
  if not env.getbool('SANDBOXED') and env.getbool('STREAM_BITCODE'):
    env.append('LLC_FLAGS_EXTRA', '-streaming-bitcode')

  if env.getone('SPLIT_MODULE') == 'seq':
    env.set('SPLIT_MODULE', '1')
    env.set('SZ_THREADS', '0')
  elif env.getone('SPLIT_MODULE') == 'auto':
    try:
      num_modules = min(4, multiprocessing.cpu_count())
    except NotImplementedError:
      num_modules = 2
    env.set('SPLIT_MODULE', str(num_modules))
    env.set('SZ_THREADS', str(num_modules))
  else:
    num_modules = int(env.getone('SPLIT_MODULE'))
    if num_modules < 1:
      Log.Fatal('Value given for -split-module must be > 0')
    env.set('SPLIT_MODULE', str(num_modules))
    env.set('SZ_THREADS', str(num_modules))

  modules = env.getone('SPLIT_MODULE')
  module_sched = env.getone('SPLIT_MODULE_SCHED')
  sz_threads = env.getone('SZ_THREADS')
  # TODO(dschuff,jvoung): No need to specify -split-module=X since the IPC
  # already has a parameter for the number of threads and modules.
  env.append('LLC_FLAGS_EXTRA', '-split-module=' + modules)
  env.append('LD_FLAGS', '-split-module=' + ('1' if use_sz else modules))
  env.append('LLC_FLAGS_EXTRA', '-split-module-sched=' + module_sched)
  # In sandboxed mode, the IPC already has a parameter for the number
  # of threads, so no need to specify that again via argv[].
  if not env.getbool('SANDBOXED'):
    env.append('SZ_FLAGS_EXTRA', '--threads=' + sz_threads)

  # If there's a bitcode file, translate it now.
  tng = driver_tools.TempNameGen(inputs + bcfiles, output)
  output_type = env.getone('OUTPUT_TYPE')
  if bcfile:
    sfile = None
    if output_type == 's':
      sfile = output

    ofile = None
    if output_type == 'o':
      ofile = output
    elif output_type != 's':
      ofile = tng.TempNameForInput(bcfile, 'o')

    if sfile:
      RunCompiler(bcfile, sfile, outfiletype='asm', use_sz=use_sz)
      if ofile:
        RunAS(sfile, ofile)
    else:
      RunCompiler(bcfile, ofile, outfiletype='obj', use_sz=use_sz)
  else:
    ofile = None

  # If we've been told to stop after translation, stop now.
  if output_type in ('o','s'):
    return 0

  if use_sz:
    # Reset SPLIT_MODULE to 1 to fall back to normal linking behavior.
    env.set('SPLIT_MODULE', '1')

  # Replace the bitcode file with __BITCODE__ in the input list
  if bcfile:
    inputs = ListReplace(inputs, bcfile, '__BITCODE__')
    env.set('INPUTS', *inputs)
  if int(env.getone('SPLIT_MODULE')) > 1:
    modules = int(env.getone('SPLIT_MODULE'))
    for i in range(1, modules):
      filename = ofile + '.module%d' % i
      TempFiles.add(filename)
      env.append('INPUTS', filename)

  if env.getone('TARGET_OS') != 'nacl':
    RunHostLD(ofile, output)
  else:
    RunLD(ofile, output)
  return 0

def RunAS(infile, outfile):
  driver_tools.RunDriver('pnacl-as', [infile, '-o', outfile])

def ListReplace(items, old, new):
  ret = []
  for k in items:
    if k == old:
      ret.append(new)
    else:
      ret.append(k)
  return ret

def RunLD(infile, outfile):
  inputs = env.get('INPUTS')
  if infile:
    # Put llc-translated-file at the beginning of the inputs so that it will
    # pull in all needed symbols from any native archives that may also be
    # in the input list. This is in case there are any mixed groups of bitcode
    # and native archives in the link (as is the case with irt_browser_lib)
    inputs.remove('__BITCODE__')
    inputs = ['--llc-translated-file=' + infile] + inputs
  env.set('ld_inputs', *inputs)
  args = env.get('LD_ARGS') + ['-o', outfile]
  if env.getbool('USE_STDLIB'):
    args += env.get('LD_ARGS_ENTRY')
  args += env.get('LD_FLAGS')
  driver_tools.RunDriver('nativeld', args)

def RunHostLD(infile, outfile):
  if env.getone('TARGET_OS') == 'linux':
    driver_tools.Run(['objcopy', '--redefine-sym', '_start=_user_start',
                      infile])
  lib_dir = (env.getone('BASE_LIB_NATIVE')
             + 'x86-32-%s/lib' % env.getone('TARGET_OS'))
  # TODO(stichnot): Consider making fully linked executable smaller by packaging
  # the .o files into an archive, and/or building the .o files with
  # -function-sections and linking with -gc-sections.
  args = ['gcc', '-m32', infile, '-o', outfile,
          os.path.join(lib_dir, 'unsandboxed_irt.o'),
          os.path.join(lib_dir, 'irt_random.o'),
          os.path.join(lib_dir, 'irt_query_list.o'),
          os.path.join(lib_dir, 'szrt.o'),
          os.path.join(lib_dir, 'szrt_ll.o'),
          '-lm', '-lpthread']
  if env.getone('TARGET_OS') == 'linux':
    args.append('-lrt')  # For clock_gettime()
  driver_tools.Run(args)

def RunCompiler(infile, outfile, outfiletype, use_sz):
  env.push()
  env.setmany(input=infile, output=outfile, outfiletype=outfiletype)
  if env.getbool('SANDBOXED'):
    RunSandboxedCompiler(use_sz)
  else:
    args = ["${RUN_SZ}" if use_sz else "${RUN_LLC}"]
    if filetype.IsPNaClBitcode(infile):
      args.append("-bitcode-format=pnacl")
    elif filetype.IsLLVMBitcode(infile):
      if not env.getbool('ALLOW_LLVM_BITCODE_INPUT'):
        Log.Fatal('Translator expects finalized PNaCl bitcode. '
                  'Pass --allow-llvm-bitcode-input to override.')
    driver_tools.Run(' '.join(args))
  env.pop()
  return 0

def GetObjectFiles(use_sz):
  if use_sz:
    # Subzero only creates one object file when using multiple threads.
    # Note that, with Subzero, specifying more object files here would
    # work, but it would be unnecessary: The IRT interface used by the
    # sandboxed translator would create the extra object files, but they
    # would be left as empty.
    obj_file_count = 1
  else:
    obj_file_count = int(env.getone('SPLIT_MODULE'))
  base_filename = env.getone('output')
  return ([base_filename] +
          ['%s.module%d' % (base_filename, number)
           for number in xrange(1, obj_file_count)])

def RunSandboxedCompiler(use_sz):
  driver_tools.CheckTranslatorPrerequisites()
  infile = env.getone('input')
  is_pnacl = filetype.IsPNaClBitcode(infile)
  if not is_pnacl and not env.getbool('ALLOW_LLVM_BITCODE_INPUT'):
    Log.Fatal('Translator expects finalized PNaCl bitcode. '
              'Pass --allow-llvm-bitcode-input to override.')
  threads = int(env.getone('SPLIT_MODULE'))
  command = [driver_tools.SelLdrCommand(),
             '-a', # Allow file access
             '-E NACL_IRT_PNACL_TRANSLATOR_COMPILE_INPUT=%s' % infile]
  driver_tools.AddListToEnv(command, 'NACL_IRT_PNACL_TRANSLATOR_COMPILE_OUTPUT',
                            GetObjectFiles(use_sz))
  driver_tools.AddListToEnv(command, 'NACL_IRT_PNACL_TRANSLATOR_COMPILE_ARG',
                            BuildOverrideCompilerCommandLine(is_pnacl, use_sz))
  command.extend(['-E NACL_IRT_PNACL_TRANSLATOR_COMPILE_THREADS=%d' % threads,
                  '--'])
  if use_sz:
    command.append('${PNACL_SZ_SB}')
  else:
    command.append('${LLC_SB}')
  driver_tools.Run(' '.join(command),
                   # stdout/stderr will be automatically dumped
                   # upon failure
                   redirect_stderr=subprocess.PIPE,
                   redirect_stdout=subprocess.PIPE)

def BuildOverrideCompilerCommandLine(is_pnacl, use_sz):
  if use_sz:
    # Subzero doesn't allow -mcpu=X tuning, only -mattr=X.
    extra_flags = env.get('SZ_FLAGS_EXTRA')
  else:
    extra_flags = env.get('LLC_FLAGS_EXTRA')
    # The mcpu is not part of the default flags, so append that too.
    mcpu = env.getone('LLC_MCPU')
    if mcpu:
      extra_flags.append(mcpu)
  if not is_pnacl:
    extra_flags.append('-bitcode-format=llvm')
  return extra_flags

def get_help(argv):
  return """
PNaCl bitcode to native code translator.

Usage: pnacl-translate [options] -arch <arch> <input> -o <output>

  <input>                 Input file (bitcode).
  -arch <arch>            Translate to <arch> (i686, x86-64, armv7).
                          Note: <arch> is a generic arch specifier.  This
                          generates code assuming certain baseline CPU features,
                          required by NaCl or Chrome. For finer control of
                          tuning and features, see -mattr, and -mcpu.
  -o <output>             Output file.

  The default output file type is .nexe, which assumes that the input file
  type is .pexe.  Native object files and assembly can also be generated
  with the -S and -c commandline flags.

ADVANCED OPTIONS:
  -mattr=<+feat1,-feat2>  Toggle specific cpu features on and off.
  -mcpu=<cpu-name>        Target a specific cpu type.  Tunes code as well as
                          turns cpu features on and off.
  -S                      Generate native assembly only.
  -c                      Generate native object file only.
  --use-sz                Use the Subzero fast translator.
  --pnacl-sb              Use the translator which runs inside the NaCl sandbox.
                          Applies to both pnacl-llc and pnacl-sz translators.
  -O[0-3]                 Change translation-time optimization level.
  -threads=<num>          Use <num> parallel threads for translation.
  -threads=auto           Automatically determine number of translation threads.
  -threads=seq            Use the minimal number of threads for translation.
"""
