#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re
import subprocess

from driver_tools import AddHostBinarySearchPath, DefaultOutputName, \
    DefaultPCHOutputName, DriverChain, GetArch, ParseArgs, ParseTriple, \
    Run, RunDriver, RunWithEnv, TempNameGen, UnrecognizedOption
from driver_env import env
from driver_log import DriverOpen, Log
import filetype
import pathtools

EXTRA_ENV = {
  'ALLOW_TRANSLATE': '0',  # Allow bitcode translation before linking.
                           # It doesn't normally make sense to do this.

  'ALLOW_NATIVE'   : '0',  # Allow native objects (.S,.s,.o) to be in the
                           # linker line for .pexe generation.
                           # It doesn't normally make sense to do this.

  # CXX_EH_MODE specifies how to deal with C++ exception handling:
  #  * 'none':  Strips out use of C++ exception handling.
  #  * 'sjlj':  Enables the setjmp()+longjmp()-based implementation of
  #    C++ exception handling.
  'CXX_EH_MODE': 'none',

  'FORCE_INTERMEDIATE_LL': '0',
                          # Produce an intermediate .ll file
                          # Useful for debugging.
                          # NOTE: potentially different code paths and bugs
                          #       might be triggered by this
  'LANGUAGE'    : '',     # C or CXX (set by SetTool)
  'INCLUDE_CXX_HEADERS': '0', # This is set by RunCC.

  # Command-line options
  'GCC_MODE'    : '',     # '' (default), '-E', '-c', or '-S'
  'SHARED'      : '0',    # Identify if the target is a shared library.
  'STDINC'      : '1',    # Include standard headers (-nostdinc sets to 0)
  'STDINCCXX'   : '1',    # Include standard cxx headers (-nostdinc++ sets to 0)
  'USE_STDLIB'  : '1',    # Include standard libraries (-nostdlib sets to 0)
  'STDLIB'      : 'libc++',     # C++ Standard Library.
  'DEFAULTLIBS' : '1',    # Link with default libraries
  'DIAGNOSTIC'  : '0',    # Diagnostic flag detected
  'PIC'         : '0',    # Generate PIC
  'NEED_DASH_E' : '0',    # Used for stdin inputs, which must have an explicit
                          # type set (using -x) unless -E is specified.
  'VERBOSE'     : '0',    # Verbose (-v)
  'SHOW_VERSION': '0',    # Version (--version)

  'PTHREAD'     : '0',   # use pthreads?
  'INPUTS'      : '',    # Input files
  'OUTPUT'      : '',    # Output file
  'UNMATCHED'   : '',    # Unrecognized parameters

  'BIAS_NONE'   : '',
  'BIAS_ARM'    : '-D__arm__ -D__ARM_ARCH_7A__ -D__ARMEL__',
  'BIAS_MIPS32' : '-D__mips__',
  'BIAS_X8632'  : '-D__i386__ -D__i386 -D__i686 -D__i686__ -D__pentium4__',
  'BIAS_X8664'  : '-D__amd64__ -D__amd64 -D__x86_64__ -D__x86_64 -D__core2__',
  'BIAS_ARM_NONSFI': '${BIAS_ARM} -D__native_client_nonsfi__',
  'BIAS_X8632_NONSFI': '${BIAS_X8632} -D__native_client_nonsfi__',
  'FRONTEND_TRIPLE' : 'le32-unknown-nacl',

  'OPT_LEVEL'   : '',  # Default for most tools is 0, but we need to know
                       # if it's explicitly set or not when the driver
                       # is only used for linking + translating.
  'CC_FLAGS'    : '-O${#OPT_LEVEL ? ${OPT_LEVEL} : 0} ' +
                  '-fno-vectorize -fno-slp-vectorize ' +
                  '-fno-common ${PTHREAD ? -pthread} ' +
                  '-nostdinc ${BIAS_%BIAS%} ' +
                  '-fno-gnu-inline-asm ' +
                  '-target ${FRONTEND_TRIPLE} ' +
                  '${IS_CXX ? -fexceptions}',


  'ISYSTEM'        : '${ISYSTEM_USER} ${STDINC ? ${ISYSTEM_BUILTIN}}',

  'ISYSTEM_USER'   : '',  # System include directories specified by
                          # using the -isystem flag.

  'ISYSTEM_BUILTIN':
    '${BASE_USR}/usr/include ' +
    '${ISYSTEM_CLANG} ' +
    '${ISYSTEM_CXX} ' +
    '${BASE_USR}/include ' +
    '${BASE_SDK}/include ',

  'ISYSTEM_CLANG'  : '${BASE_LLVM}/lib/clang/${CLANG_VER}/include',

  'ISYSTEM_CXX' :
    '${INCLUDE_CXX_HEADERS && STDINCCXX ? ${ISYSTEM_CXX_include_paths}}',

  'ISYSTEM_CXX_include_paths' : '${BASE_USR}/include/c++/v1',

  # Only propagate opt level to linker if explicitly set, so that the
  # linker will know if an opt level was explicitly set or not.
  'LD_FLAGS' : '${#OPT_LEVEL ? -O${OPT_LEVEL}} ' +
               '${SHARED ? -shared : -static} ' +
               '${PIC ? -fPIC} ${@AddPrefix:-L:SEARCH_DIRS} ' +
               '--pnacl-exceptions=${CXX_EH_MODE}',

  'SEARCH_DIRS' : '', # Directories specified using -L

  # Library Strings
  'EMITMODE'    : '${!USE_STDLIB || SHARED ? nostdlib : static}',

  # This is setup so that LD_ARGS_xxx is evaluated lazily.
  'LD_ARGS' : '${LD_ARGS_%EMITMODE%}',

  # ${ld_inputs} signifies where to place the objects and libraries
  # provided on the command-line.
  'LD_ARGS_nostdlib': '-nostdlib ${ld_inputs}',

  'LD_ARGS_static':
    '-l:crt1.x -l:crti.bc -l:crtbegin.bc '
    '${CXX_EH_MODE==sjlj ? -l:sjlj_eh_redirect.bc : '
      '${CXX_EH_MODE==none ? -l:unwind_stubs.bc}} ' +
    '${ld_inputs} ' +
    '--start-group ${STDLIBS} --end-group',

  'LLVM_PASSES_TO_DISABLE': '',

  # Flags for translating to native .o files.
  'TRANSLATE_FLAGS' : '-O${#OPT_LEVEL ? ${OPT_LEVEL} : 0}',

  'STDLIBS'   : '${DEFAULTLIBS ? '
                '${LIBSTDCPP} ${LIBPTHREAD} ${LIBNACL} ${LIBC} '
                '${LIBGCC_BC} ${LIBPNACLMM}}',
  'LIBSTDCPP' : '${IS_CXX ? -lc++ -lm -lpthread }',
  # The few functions in the bitcode version of compiler-rt unfortunately
  # depend on libm. TODO(jvoung): try rewriting the compiler-rt functions
  # to be standalone.
  'LIBGCC_BC' : '-lgcc -lm',
  'LIBC'      : '-lc',
  'LIBNACL'   : '-lnacl',
  'LIBPNACLMM': '-lpnaclmm',
  # Enabled/disabled by -pthreads
  'LIBPTHREAD': '${PTHREAD ? -lpthread}',

  # IS_CXX is set by pnacl-clang and pnacl-clang++ programmatically
  'CC' : '${IS_CXX ? ${CLANGXX} : ${CLANG}}',
  'RUN_CC': '${CC} ${emit_llvm_flag} ${mode} ${CC_FLAGS} ' +
            '${@AddPrefix:-isystem :ISYSTEM} ' +
            '-x${typespec} ${infile} -o ${output}',
}

def AddLLVMPassDisableFlag(*args):
  env.append('LLVM_PASSES_TO_DISABLE', *args)
  env.append('LD_FLAGS', *args)

def AddLDFlag(*args):
  env.append('LD_FLAGS', *args)

def AddTranslatorFlag(*args):
  # pass translator args to ld in case we go all the way to .nexe
  env.append('LD_FLAGS', *['-Wt,' + a for a in args])
  # pass translator args to translator in case we go to .o
  env.append('TRANSLATE_FLAGS', *args)

def AddCCFlag(*args):
  env.append('CC_FLAGS', *args)

def AddDiagnosticFlag(*args):
  env.append('CC_FLAGS', *args)
  env.set('DIAGNOSTIC', '1')

def SetTarget(*args):
  arch = ParseTriple(args[0])
  env.set('FRONTEND_TRIPLE', args[0])
  AddLDFlag('--target=' + args[0])

def SetStdLib(*args):
  """Set the C++ Standard Library."""
  lib = args[0]
  if lib != 'libc++':
    Log.Fatal('Only libc++ is supported as standard library')

def IsPortable():
  return env.getone('FRONTEND_TRIPLE').startswith('le32-')

stdin_count = 0
def AddInputFileStdin():
  global stdin_count

  # When stdin is an input, -x or -E must be given.
  forced_type = filetype.GetForcedFileType()
  if not forced_type:
    # Only allowed if -E is specified.
    forced_type = 'c'
    env.set('NEED_DASH_E', '1')

  stdin_name = '__stdin%d__' % stdin_count
  env.append('INPUTS', stdin_name)
  filetype.ForceFileType(stdin_name, forced_type)
  stdin_count += 1

def IsStdinInput(f):
  return f.startswith('__stdin') and f.endswith('__')

def HandleDashX(arg):
  if arg == 'none':
    filetype.SetForcedFileType(None)
    return
  filetype.SetForcedFileType(filetype.GCCTypeToFileType(arg))

def AddVersionFlag(*args):
  env.set('SHOW_VERSION', '1')
  AddDiagnosticFlag(*args)

def AddBPrefix(prefix):
  """ Add a path to the list searched for host binaries and include dirs. """
  AddHostBinarySearchPath(prefix)
  prefix = pathtools.normalize(prefix)
  if pathtools.isdir(prefix) and not prefix.endswith('/'):
    prefix += '/'

  # Add prefix/ to the library search dir if it exists
  if pathtools.isdir(prefix):
    env.append('SEARCH_DIRS', prefix)

  # Add prefix/include to isystem if it exists
  include_dir = prefix + 'include'
  if pathtools.isdir(include_dir):
    env.append('ISYSTEM_USER', include_dir)

CustomPatterns = [
  ( '--driver=(.+)',                "env.set('CC', pathtools.normalize($0))\n"),
  ( '--pnacl-allow-native',         "env.set('ALLOW_NATIVE', '1')"),
  ( '--pnacl-allow-translate',      "env.set('ALLOW_TRANSLATE', '1')"),
  ( '--pnacl-frontend-triple=(.+)', SetTarget),
  ( ('-target','(.+)'),             SetTarget),
  ( ('--target=(.+)'),              SetTarget),
  ( '--pnacl-exceptions=(none|sjlj)', "env.set('CXX_EH_MODE', $0)"),
  ( '(--pnacl-allow-nexe-build-id)', AddLDFlag),
  ( '(--pnacl-disable-abi-check)',  AddLDFlag),
  ( '(--pnacl-disable-pass=.+)',    AddLLVMPassDisableFlag),
]

GCCPatterns = [
  ( '-o(.+)',          "env.set('OUTPUT', pathtools.normalize($0))"),
  ( ('-o', '(.+)'),    "env.set('OUTPUT', pathtools.normalize($0))"),

  ( '-E',              "env.set('GCC_MODE', '-E')"),
  ( '-S',              "env.set('GCC_MODE', '-S')"),
  ( '-c',              "env.set('GCC_MODE', '-c')"),

  ( '-nostdinc',       "env.set('STDINC', '0')"),
  ( '-nostdinc\+\+',   "env.set('STDINCCXX', '0')"),
  ( '-nostdlib',       "env.set('USE_STDLIB', '0')"),
  ( '-nodefaultlibs',  "env.set('DEFAULTLIBS', '0')"),

  ( '-?-stdlib=(.*)',      SetStdLib),
  ( ('-?-stdlib', '(.*)'), SetStdLib),

  # Flags to pass to native linker
  ( '(-Wn,.*)',        AddLDFlag),
  ( '-rdynamic', "env.append('LD_FLAGS', '-export-dynamic')"),

  # Flags to pass to pnacl-translate
  ( '-Wt,(.*)',               AddTranslatorFlag),
  ( ('-Xtranslator','(.*)'),  AddTranslatorFlag),

  # We don't care about -fPIC, but pnacl-ld and pnacl-translate do.
  ( '-fPIC',           "env.set('PIC', '1')"),

  # We must include -l, -Xlinker, and -Wl options into the INPUTS
  # in the order they appeared. This is the exactly behavior of gcc.
  # For example: gcc foo.c -Wl,--start-group -lx -ly -Wl,--end-group
  #
  ( '(-l.+)',             "env.append('INPUTS', $0)"),
  ( ('(-l)','(.+)'),      "env.append('INPUTS', $0+$1)"),
  ( ('-Xlinker','(.*)'),  "env.append('INPUTS', '-Xlinker=' + $0)"),
  ( '(-Wl,.*)',           "env.append('INPUTS', $0)"),
  ( '(-Bstatic)',         "env.append('INPUTS', $0)"),
  ( '(-Bdynamic)',        "env.append('INPUTS', $0)"),

  ( '-O([sz])',           "env.set('OPT_LEVEL', $0)\n"),
  ( '-O([0-3])',          "env.set('OPT_LEVEL', $0)\n"),
  ( '-O([0-9]+)',         "env.set('OPT_LEVEL', '3')\n"),
  ( '-O',                 "env.set('OPT_LEVEL', '1')\n"),

  ( ('-isystem', '(.*)'),
                       "env.append('ISYSTEM_USER', pathtools.normalize($0))"),
  ( '-isystem(.+)',
                       "env.append('ISYSTEM_USER', pathtools.normalize($0))"),
  ( ('-I', '(.+)'),    "env.append('CC_FLAGS', '-I'+pathtools.normalize($0))"),
  ( '-I(.+)',          "env.append('CC_FLAGS', '-I'+pathtools.normalize($0))"),
  # -I is passed through, so we allow -isysroot and pass it through as well.
  # However -L is intercepted and interpreted, so it would take more work
  # to handle -sysroot w/ libraries.
  ( ('-isysroot', '(.+)'),
        "env.append('CC_FLAGS', '-isysroot ' + pathtools.normalize($0))"),
  ( '-isysroot(.+)',
        "env.append('CC_FLAGS', '-isysroot ' + pathtools.normalize($0))"),

  # NOTE: the -iquote =DIR syntax (substitute = with sysroot) doesn't work.
  # Clang just says: ignoring nonexistent directory "=DIR"
  ( ('-iquote', '(.+)'),
    "env.append('CC_FLAGS', '-iquote', pathtools.normalize($0))"),
  ( ('-iquote(.+)'),
    "env.append('CC_FLAGS', '-iquote', pathtools.normalize($0))"),

  ( ('-idirafter', '(.+)'),
      "env.append('CC_FLAGS', '-idirafter'+pathtools.normalize($0))"),
  ( '-idirafter(.+)',
      "env.append('CC_FLAGS', '-idirafter'+pathtools.normalize($0))"),

  ( ('(-include)','(.+)'),    AddCCFlag),
  ( ('(-include.+)'),         AddCCFlag),
  ( '(--relocatable-pch)',    AddCCFlag),
  ( '(-g)',                   AddCCFlag),
  ( '(-W.*)',                 AddCCFlag),
  ( '(-w)',                   AddCCFlag),
  ( '(-std=.*)',              AddCCFlag),
  ( '(-ansi)',                AddCCFlag),
  ( ('(-D)','(.*)'),          AddCCFlag),
  ( '(-D.+)',                 AddCCFlag),
  ( ('(-U)','(.*)'),          AddCCFlag),
  ( '(-U.+)',                 AddCCFlag),
  ( '(-f.*)',                 AddCCFlag),
  ( '(-pedantic)',            AddCCFlag),
  ( '(-pedantic-errors)',     AddCCFlag),
  ( '(-g.*)',                 AddCCFlag),
  ( '(-v|--v)',               "env.append('CC_FLAGS', $0)\n"
                              "env.set('VERBOSE', '1')"),
  ( '(-pthreads?)',           "env.set('PTHREAD', '1')"),

  # No-op: accepted for compatibility in case build scripts pass it.
  ( '-static',                ""),

  ( ('-B','(.*)'),            AddBPrefix),
  ( ('-B(.+)'),               AddBPrefix),

  ( ('-L','(.+)'), "env.append('SEARCH_DIRS', pathtools.normalize($0))"),
  ( '-L(.+)',      "env.append('SEARCH_DIRS', pathtools.normalize($0))"),

  ( '(-Wp,.*)', AddCCFlag),
  ( '(-Xpreprocessor .*)', AddCCFlag),
  ( ('(-Xclang)', '(.*)'), AddCCFlag),

  # Accept and ignore default flags
  ( '-m32',                      ""),
  ( '-emit-llvm',                ""),

  ( '(-MG)',          AddCCFlag),
  ( '(-MMD)',         AddCCFlag),
  ( '(-MM?)',         "env.append('CC_FLAGS', $0)\n"
                      "env.set('GCC_MODE', '-E')"),
  ( '(-MP)',          AddCCFlag),
  ( ('(-MQ)','(.*)'), AddCCFlag),
  ( '(-MD)',          AddCCFlag),
  ( ('(-MT)','(.*)'), AddCCFlag),
  ( ('(-MF)','(.*)'), "env.append('CC_FLAGS', $0, pathtools.normalize($1))"),

  ( ('-x', '(.+)'),    HandleDashX),
  ( '-x(.+)',          HandleDashX),

  ( ('(-mllvm)', '(.+)'), AddCCFlag),

  # Ignore these gcc flags
  ( '(-msse)',                ""),
  ( '(-march=armv7-a)',       ""),
  ( '(-pipe)',                ""),

  ( '(-shared)',              "env.set('SHARED', '1')"),
  ( '(-s)',                   AddLDFlag),
  ( '(--strip-all)',          AddLDFlag),
  ( '(--strip-debug)',        AddLDFlag),

  # Ignore these assembler flags
  ( '(-Qy)',                  ""),
  ( ('(--traditional-format)', '.*'), ""),
  ( '(-gstabs)',              ""),
  ( '(--gstabs)',             ""),
  ( '(-gdwarf2)',             ""),
  ( '(--gdwarf2)',             ""),
  ( '(--fatal-warnings)',     ""),
  ( '(-meabi=.*)',            ""),
  ( '(-mfpu=.*)',             ""),

  ( '(-mfloat-abi=.+)',       AddCCFlag),

  # GCC diagnostic mode triggers
  ( '(-print-.*)',            AddDiagnosticFlag),
  ( '(--print.*)',            AddDiagnosticFlag),
  ( '(-dumpspecs)',           AddDiagnosticFlag),
  ( '(--version)',            AddVersionFlag),
  # These are preprocessor flags which should be passed to the frontend, but
  # should not prevent the usual -i flags (which DIAGNOSTIC mode does)
  ( '(-d[DIMNU])',            AddCCFlag),
  ( '(-d.*)',                 AddDiagnosticFlag),

  # Catch all other command-line arguments
  ( '(-.+)',              "env.append('UNMATCHED', $0)"),

  # Standard input
  ( '-',     AddInputFileStdin),

  # Input Files
  # Call ForceFileType for all input files at the time they are
  # parsed on the command-line. This ensures that the gcc "-x"
  # setting is correctly applied.
  ( '(.*)',  "env.append('INPUTS', pathtools.normalize($0))\n"
             "filetype.ForceFileType(pathtools.normalize($0))"),
]

def CheckSetup():
  if not env.has('IS_CXX'):
    Log.Fatal('"pnacl-driver" cannot be used directly. '
              'Use pnacl-clang or pnacl-clang++.')

def DriverOutputTypes(driver_flag, compiling_to_native):
  output_type_map = {
    ('-E', False) : 'pp',
    ('-E', True)  : 'pp',
    ('-c', False) : 'po',
    ('-c', True)  : 'o',
    ('-S', False) : 'll',
    ('-S', True)  : 's',
    ('',   False) : 'pexe',
    ('',   True)  : 'nexe',
  }
  return output_type_map[(driver_flag, compiling_to_native)]


def ReadDriverRevision():
  rev_file = env.getone('DRIVER_REV_FILE')
  nacl_ver = DriverOpen(rev_file, 'rb').readlines()[0]
  m = re.search(r'\[GIT\].*/native_client(?:\.git)?:\s*([0-9a-f]{40})',
                nacl_ver)
  if m:
    return m.group(1)
  # fail-fast: if the REV file exists but regex search failed,
  # we need to fix the regex to get nacl-version.
  if not m:
    Log.Fatal('Failed to parse REV file to get nacl-version.')


def main(argv):
  env.update(EXTRA_ENV)
  CheckSetup()
  ParseArgs(argv, CustomPatterns + GCCPatterns)

  # "configure", especially when run as part of a toolchain bootstrap
  # process, will invoke gcc with various diagnostic options and
  # parse the output. In these cases we do not alter the incoming
  # commandline. It is also important to not emit spurious messages.
  if env.getbool('DIAGNOSTIC'):
    if env.getbool('SHOW_VERSION'):
      code, stdout, stderr = Run(env.get('CC') + env.get('CC_FLAGS'),
                                 redirect_stdout=subprocess.PIPE)
      out = stdout.split('\n')
      nacl_version = ReadDriverRevision()
      out[0] += ' nacl-version=%s' % nacl_version
      stdout = '\n'.join(out)
      print stdout,
    else:
      Run(env.get('CC') + env.get('CC_FLAGS'))
    return 0

  unmatched = env.get('UNMATCHED')
  if len(unmatched) > 0:
    UnrecognizedOption(*unmatched)

  # If -arch was given, we are compiling directly to native code
  compiling_to_native = GetArch() is not None

  if env.getbool('ALLOW_NATIVE'):
    if not compiling_to_native:
      Log.Fatal("--pnacl-allow-native without -arch is not meaningful.")
    # For native/mixed links, also bring in the native libgcc and
    # libcrt_platform to avoid link failure if pre-translated native
    # code needs functions from it.
    env.append('LD_FLAGS', env.eval('-L${LIBS_NATIVE_ARCH}'))
    env.append('STDLIBS', '-lgcc')
    env.append('STDLIBS', '-lcrt_platform')


  flags_and_inputs = env.get('INPUTS')
  output = env.getone('OUTPUT')

  if len(flags_and_inputs) == 0:
    if env.getbool('VERBOSE'):
      # -v can be invoked without any inputs. Runs the original
      # command without modifying the commandline for this case.
      Run(env.get('CC') + env.get('CC_FLAGS'))
      return 0
    else:
      Log.Fatal('No input files')

  gcc_mode = env.getone('GCC_MODE')
  output_type = DriverOutputTypes(gcc_mode, compiling_to_native)

  # '-shared' modifies the output from the linker and should be considered when
  # determining the final output type.
  if env.getbool('SHARED'):
    if compiling_to_native:
      Log.Fatal('Building native shared libraries not supported')
    if gcc_mode != '':
      Log.Fatal('-c, -S, and -E are disallowed with -shared')
    output_type = 'pll'

  # INPUTS consists of actual input files and a subset of flags like -Wl,<foo>.
  # Create a version with just the files.
  inputs = [f for f in flags_and_inputs if not IsFlag(f)]
  header_inputs = [f for f in inputs
                   if filetype.IsHeaderType(filetype.FileType(f))]
  # Handle PCH case specially (but only for a limited sense...)
  if header_inputs and gcc_mode != '-E':
    # We only handle doing pre-compiled headers for all inputs or not at
    # all at the moment. This is because DriverOutputTypes only assumes
    # one type of output, depending on the "gcc_mode" flag. When mixing
    # header inputs w/ non-header inputs, some of the outputs will be
    # pch while others will be output_type. We would also need to modify
    # the input->output chaining for the needs_linking case.
    if len(header_inputs) != len(inputs):
      Log.Fatal('mixed compiling of headers and source not supported')
    CompileHeaders(header_inputs, output)
    return 0

  needs_linking = (gcc_mode == '')

  if env.getbool('NEED_DASH_E') and gcc_mode != '-E':
    Log.Fatal("-E or -x required when input is from stdin")

  # There are multiple input files and no linking is being done.
  # There will be multiple outputs. Handle this case separately.
  if not needs_linking:
    if output != '' and len(inputs) > 1:
      Log.Fatal('Cannot have -o with -c, -S, or -E and multiple inputs: %s',
                repr(inputs))

    for f in inputs:
      intype = filetype.FileType(f)
      if not (filetype.IsSourceType(intype) or filetype.IsHeaderType(intype)):
        if ((output_type == 'pp' and intype != 'S') or
            (output_type == 'll') or
            (output_type == 'po' and intype != 'll') or
            (output_type == 's' and intype not in ('ll','po','S')) or
            (output_type == 'o' and intype not in ('ll','po','S','s'))):
          Log.Fatal("%s: Unexpected type of file for '%s'",
                    pathtools.touser(f), gcc_mode)

      if output == '':
        f_output = DefaultOutputName(f, output_type)
      else:
        f_output = output

      namegen = TempNameGen([f], f_output)
      CompileOne(f, output_type, namegen, f_output)
    return 0

  # Linking case
  assert(needs_linking)
  assert(output_type in ('pll', 'pexe', 'nexe'))

  if output == '':
    output = pathtools.normalize('a.out')
  namegen = TempNameGen(flags_and_inputs, output)

  # Compile all source files (c/c++/ll) to .po
  for i in xrange(0, len(flags_and_inputs)):
    if IsFlag(flags_and_inputs[i]):
      continue
    intype = filetype.FileType(flags_and_inputs[i])
    if filetype.IsSourceType(intype) or intype == 'll':
      flags_and_inputs[i] = CompileOne(flags_and_inputs[i], 'po', namegen)

  # Compile all .s/.S to .o
  if env.getbool('ALLOW_NATIVE'):
    for i in xrange(0, len(flags_and_inputs)):
      if IsFlag(flags_and_inputs[i]):
        continue
      intype = filetype.FileType(flags_and_inputs[i])
      if intype in ('s','S'):
        flags_and_inputs[i] = CompileOne(flags_and_inputs[i], 'o', namegen)

  # We should only be left with .po and .o and libraries
  for f in flags_and_inputs:
    if IsFlag(f):
      continue
    intype = filetype.FileType(f)
    if intype in ('o','s','S') or filetype.IsNativeArchive(f):
      if not env.getbool('ALLOW_NATIVE'):
        Log.Fatal('%s: Native object files not allowed in link. '
                  'Use --pnacl-allow-native to override.', pathtools.touser(f))
    assert(intype in ('po','o','so','ldscript') or filetype.IsArchive(f))

  # Fix the user-specified linker arguments
  ld_inputs = []
  for f in flags_and_inputs:
    if f.startswith('-Xlinker='):
      ld_inputs.append(f[len('-Xlinker='):])
    elif f.startswith('-Wl,'):
      ld_inputs += f[len('-Wl,'):].split(',')
    else:
      ld_inputs.append(f)

  if env.getbool('ALLOW_NATIVE'):
    ld_inputs.append('--pnacl-allow-native')

  # Invoke the linker
  env.set('ld_inputs', *ld_inputs)

  ld_args = env.get('LD_ARGS')
  ld_flags = env.get('LD_FLAGS')

  RunDriver('pnacl-ld', ld_flags + ld_args + ['-o', output])
  return 0

def IsFlag(f):
  return f.startswith('-')

def CompileHeaders(header_inputs, output):
  if output != '' and len(header_inputs) > 1:
      Log.Fatal('Cannot have -o <out> and compile multiple header files: %s',
                repr(header_inputs))
  for f in header_inputs:
    f_output = output if output else DefaultPCHOutputName(f)
    RunCC(f, f_output, mode='', emit_llvm_flag='')

def CompileOne(infile, output_type, namegen, output = None):
  if output is None:
    output = namegen.TempNameForInput(infile, output_type)

  chain = DriverChain(infile, output, namegen)
  SetupChain(chain, filetype.FileType(infile), output_type)
  chain.run()
  return output

def RunCC(infile, output, mode, emit_llvm_flag='-emit-llvm'):
  intype = filetype.FileType(infile)
  typespec = filetype.FileTypeToGCCType(intype)
  include_cxx_headers = ((env.get('LANGUAGE') == 'CXX') or
                         (intype in ('c++', 'c++-header')))
  env.setbool('INCLUDE_CXX_HEADERS', include_cxx_headers)
  if IsStdinInput(infile):
    infile = '-'
  RunWithEnv("${RUN_CC}", infile=infile, output=output,
                          emit_llvm_flag=emit_llvm_flag, mode=mode,
                          typespec=typespec)

def RunLLVMAS(infile, output):
  if IsStdinInput(infile):
    infile = '-'
  # This is a bitcode only step - so get rid of "-arch xxx" which
  # might be inherited from the current invocation
  RunDriver('pnacl-as', [infile, '-o', output],
            suppress_inherited_arch_args=True)

def RunNativeAS(infile, output):
  if IsStdinInput(infile):
    infile = '-'
  RunDriver('pnacl-as', [infile, '-o', output])

def RunTranslate(infile, output, mode):
  if not env.getbool('ALLOW_TRANSLATE'):
    Log.Fatal('%s: Trying to convert bitcode to an object file before '
              'bitcode linking. This is supposed to wait until '
              'translation. Use --pnacl-allow-translate to override.',
              pathtools.touser(infile))
  args = env.get('TRANSLATE_FLAGS') + [mode, '--allow-llvm-bitcode-input',
                                       infile, '-o', output]
  if env.getbool('PIC'):
    args += ['-fPIC']
  RunDriver('pnacl-translate', args)


def RunOpt(infile, outfile, pass_list):
  filtered_list = [pass_option for pass_option in pass_list
                   if pass_option not in env.get('LLVM_PASSES_TO_DISABLE')]
  RunDriver('pnacl-opt', filtered_list + [infile, '-o', outfile])


def SetupChain(chain, input_type, output_type):
  assert(output_type in ('pp','ll','po','s','o'))
  cur_type = input_type

  # source file -> pp
  if filetype.IsSourceType(cur_type) and output_type == 'pp':
    chain.add(RunCC, 'cpp', mode='-E')
    cur_type = 'pp'
  if cur_type == output_type:
    return

  # header file -> pre-process
  if filetype.IsHeaderType(cur_type) and output_type == 'pp':
    chain.add(RunCC, 'cpp', mode='-E')
    cur_type = 'pp'
  if cur_type == output_type:
    return

  # source file -> ll
  if (filetype.IsSourceType(cur_type) and
     (env.getbool('FORCE_INTERMEDIATE_LL') or output_type == 'll')):
    chain.add(RunCC, 'll', mode='-S')
    cur_type = 'll'
  if cur_type == output_type:
    return

  # ll -> po
  if cur_type == 'll':
    chain.add(RunLLVMAS, 'po')
    cur_type = 'po'
  if cur_type == output_type:
    return

  # source file -> po (we also force native output to go through this phase
  if filetype.IsSourceType(cur_type) and output_type in ('po', 'o', 's'):
    chain.add(RunCC, 'po', mode='-c')
    cur_type = 'po'
  if cur_type == output_type:
    return

  # po -> o
  if (cur_type == 'po' and output_type == 'o'):
    # If we aren't using biased bitcode, then at least -expand-byval
    # must be run to work with the PPAPI shim calling convention.
    if IsPortable():
      chain.add(RunOpt, 'expand.po', pass_list=['-expand-byval'])
    chain.add(RunTranslate, 'o', mode='-c')
    cur_type = 'o'
  if cur_type == output_type:
    return

  # po -> s
  if cur_type == 'po':
    # If we aren't using biased bitcode, then at least -expand-byval
    # must be run to work with the PPAPI shim calling convention.
    if IsPortable():
      chain.add(RunOpt, 'expand.po', pass_list=['-expand-byval'])
    chain.add(RunTranslate, 's', mode='-S')
    cur_type = 's'
  if cur_type == output_type:
    return

  # S -> s
  if cur_type == 'S':
    chain.add(RunCC, 's', mode='-E')
    cur_type = 's'
    if output_type == 'pp':
      return
  if cur_type == output_type:
    return

  # s -> o
  if cur_type == 's' and output_type == 'o':
    chain.add(RunNativeAS, 'o')
    cur_type = 'o'
  if cur_type == output_type:
    return

  Log.Fatal("Unable to compile .%s to .%s", input_type, output_type)

def get_help(argv):
  tool = env.getone('SCRIPT_NAME')

  if '--help-full' in argv:
    # To get ${CC}, etc.
    env.update(EXTRA_ENV)
    code, stdout, stderr = Run('"${CC}" -help',
                              redirect_stdout=subprocess.PIPE,
                              redirect_stderr=subprocess.STDOUT,
                              errexit=False)
    return stdout
  else:
    return """
This is a "GCC-compatible" driver using clang under the hood.

Usage: %s [options] <inputs> ...

BASIC OPTIONS:
  -o <file>             Output to <file>.
  -E                    Only run the preprocessor.
  -S                    Generate bitcode assembly.
  -c                    Generate bitcode object.
  -I <dir>              Add header search path.
  -L <dir>              Add library search path.
  -D<key>[=<val>]       Add definition for the preprocessor.
  -W<id>                Toggle warning <id>.
  -f<feature>           Enable <feature>.
  -Wl,<arg>             Pass <arg> to the linker.
  -Xlinker <arg>        Pass <arg> to the linker.
  -Wt,<arg>             Pass <arg> to the translator.
  -Xtranslator <arg>    Pass <arg> to the translator.
  -Wp,<arg>             Pass <arg> to the preprocessor.
  -Xpreprocessor,<arg>  Pass <arg> to the preprocessor.
  -x <language>         Treat subsequent input files as having type <language>.
  -static               Produce a static executable (the default).
  -Bstatic              Link subsequent libraries statically.
  -Bdynamic             Link subsequent libraries dynamically.
  -fPIC                 Ignored (only used by translator backend)
                        (accepted for compatibility).
  -pipe                 Ignored (for compatibility).
  -O<n>                 Optimation level <n>: 0, 1, 2, 3, 4 or s.
  -g                    Generate complete debug information.
  -gline-tables-only    Generate debug line-information only
                        (allowing for stack traces).
  -flimit-debug-info    Generate limited debug information.
  -save-temps           Keep intermediate compilation results.
  -v                    Verbose output / show commands.
  -h | --help           Show this help.
  --help-full           Show underlying clang driver's help message
                        (warning: not all options supported).
""" % (tool)
