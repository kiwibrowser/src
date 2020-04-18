#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Recipes for PNaCl target libs."""

import fnmatch
import os
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import pynacl.gsd_storage
import pynacl.platform

import command
import pnacl_commands

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
NACL_DIR = os.path.dirname(SCRIPT_DIR)

CLANG_VER = '3.7.0'

def ToolName(name):
  return 'pnacl-' + name

# Return the path to a tool to build target libraries
# msys should be false if the path will be called directly rather than passed to
# an msys or cygwin tool such as sh or make.
def PnaclTool(toolname, arch='le32', msys=True):
  if not msys and pynacl.platform.IsWindows():
    ext = '.bat'
  else:
    ext = ''
  if toolname in ['llvm-mc']:
    # Some tools, like llvm-mc, don't need or have a special pnacl- prefix
    # binary.
    base = toolname
  elif IsBCArch(arch):
    base = ToolName(toolname)
  else:
    base = '-'.join([TargetArch(arch), 'nacl', toolname])
  return command.path.join('%(abs_target_lib_compiler)s',
                           'bin', base + ext)

# PNaCl tools for newlib's environment, e.g. CC_FOR_TARGET=/path/to/pnacl-clang
TOOL_ENV_NAMES = { 'CC': 'clang', 'CXX': 'clang++', 'AR': 'ar', 'NM': 'nm',
                   'RANLIB': 'ranlib', 'READELF': 'readelf', 'AS': 'as' }

def TargetTools(arch):
  return [ tool + '_FOR_TARGET=' + PnaclTool(name, arch=arch, msys=True)
           for tool, name in TOOL_ENV_NAMES.iteritems() ]


def MakeCommand():
  make_command = ['make']
  if not pynacl.platform.IsWindows():
    # The make that ships with msys sometimes hangs when run with -j.
    # The ming32-make that comes with the compiler itself reportedly doesn't
    # have this problem, but it has issues with pathnames with LLVM's build.
    make_command.append('-j%(cores)s')
  return make_command

# Return the component name to use for a component name with
# a host triple. GNU configuration triples contain dashes, which are converted
# to underscores so the names are legal for Google Storage.
def GSDJoin(*args):
  return '_'.join([pynacl.gsd_storage.LegalizeName(arg) for arg in args])


def TripleFromArch(bias_arch):
  return bias_arch + '-nacl'


def IsBiasedBCArch(arch):
  return arch.endswith('_bc')


def IsBCArch(arch):
  return IsBiasedBCArch(arch) or arch == 'le32'


def IsNonSFIArch(arch):
  return arch.endswith('-nonsfi')


# Return the target arch recognized by the compiler (e.g. i686) from an arch
# string which might be a biased-bitcode-style arch (e.g. i686_bc)
def TargetArch(bias_arch):
  if IsBiasedBCArch(bias_arch):
    return bias_arch[:bias_arch.index('_bc')]
  return bias_arch


def MultilibArch(bias_arch):
  return 'x86_64' if bias_arch == 'i686' else bias_arch


def MultilibLibDir(bias_arch):
  suffix = '32' if bias_arch == 'i686' else ''
  return os.path.join(TripleFromArch(MultilibArch(bias_arch)), 'lib' + suffix)


def BiasedBitcodeTargetFlag(bias_arch):
  arch = TargetArch(bias_arch)
  flagmap = {
      # Arch     Target                           Extra flags.
      'x86_64': ('x86_64-unknown-nacl',           []),
      'i686':   ('i686-unknown-nacl',             []),
      'arm':    ('armv7-unknown-nacl-gnueabihf',  ['-mfloat-abi=hard']),
      'mipsel': ('mipsel-unknown-nacl',           []),
  }
  # The -emit-llvm flag is only needed for native Clang drivers since
  # pnacl-clang already passes that option to every Clang invocation.
  # TODO(phosek): Refactor the per-toolchain options into a separate class.
  return ['--target=%s' % flagmap[arch][0], '-emit-llvm'] + flagmap[arch][1]


def TranslatorArchToBiasArch(arch):
  archmap = {'x86-32': 'i686',
             'x86-32-nonsfi': 'i686',
             'x86-64': 'x86_64',
             'arm': 'arm',
             'arm-nonsfi': 'arm',
             'mips32': 'mipsel'}
  return archmap[arch]

def TargetLibCflags(bias_arch):
  flags = '-g -O2'
  if IsBCArch(bias_arch):
    flags += ' -mllvm -inline-threshold=5'
  else:
    # Use sections for the library builds to allow better GC for the IRT.
    flags += ' -ffunction-sections -fdata-sections'
    if bias_arch != 'arm':
      # Newlib target libs need to be build with the LLVM assembler on x86-64
      # because it uses the sandbox base address-hiding form of calls, which
      # we need in the IRT. On x86-32 that doesn't matter but the LLVM assembler
      # sometimes generates slightly better code and it's good to be consistent.
      flags += ' -integrated-as'
  if IsBiasedBCArch(bias_arch):
    flags += ' ' + ' '.join(BiasedBitcodeTargetFlag(bias_arch))
  return flags


def NewlibIsystemCflags(bias_arch):
  include_arch = MultilibArch(bias_arch)
  return ' '.join([
    '-isystem',
    command.path.join('%(' + GSDJoin('abs_newlib', include_arch) +')s',
                      TripleFromArch(include_arch), 'include')])


def LibCxxCflags(bias_arch):
  # HAS_THREAD_LOCAL is used by libc++abi's exception storage, the fallback is
  # pthread otherwise.
  return ' '.join([TargetLibCflags(bias_arch), NewlibIsystemCflags(bias_arch),
                   '-fexceptions', '-DHAS_THREAD_LOCAL=1',
                   '-D__ARM_DWARF_EH__'])


def NativeTargetFlag(bias_arch):
  arch = TargetArch(bias_arch)
  flagmap = {
      # Arch  Target                      Extra flags.
      'x86_64': ('x86_64-linux-gnu',        []),
      'i686':   ('i686-linux-gnu',          []),
      'arm':    ('armv7a-linux-gnueabihf',  []),
      'mipsel': ('mipsel-linux-gnu',        []),
  }
  return ['--target=%s' % flagmap[arch][0]] + flagmap[arch][1]


def NonSFITargetLibCflags(bias_arch):
  flags = '-D__native_client_nonsfi__ -fPIC -nostdinc '
  flags += ' '.join([
    '-isystem',
    command.path.join('%(abs_target_lib_compiler)s',
                      'lib', 'clang', CLANG_VER, 'include')])
  if bias_arch == 'i686':
    flags += ' -malign-double -mllvm -mtls-use-call'
  flags += ' ' + ' '.join(NativeTargetFlag(bias_arch))
  return flags


# Build a single object file for the target.
def BuildTargetObjectCmd(source, output, bias_arch, output_dir='%(cwd)s',
                         extra_flags=[]):
  flags = ['-Wall', '-Werror', '-O2', '-c'] + extra_flags
  if IsBiasedBCArch(bias_arch):
    flags.extend(BiasedBitcodeTargetFlag(bias_arch))
  flags.extend(NewlibIsystemCflags(bias_arch).split())
  return command.Command(
      [PnaclTool('clang', arch=bias_arch, msys=False)] + flags + [
          command.path.join('%(src)s', source),
     '-o', command.path.join(output_dir, output)])


# Build a single object file as native code for the translator.
def BuildTargetTranslatorCmd(sourcefile, output, arch, extra_flags=[],
                             source_dir='%(src)s', output_dir='%(cwd)s'):
  bias_arch = TranslatorArchToBiasArch(arch)
  flags = ['-Wall', '-Werror', '-O3', '-integrated-as', '-c'] + extra_flags
  if IsNonSFIArch(arch):
    flags.extend(NonSFITargetLibCflags(bias_arch).split())
  return command.Command(
    [PnaclTool('clang', arch=bias_arch, msys=False)] + flags +
     # TODO(dschuff): this include breaks the input encapsulation for build
     # rules.
     ['-I%(top_srcdir)s/..'] +
    NewlibIsystemCflags('le32').split() +
    [command.path.join(source_dir, sourcefile),
     '-o', command.path.join(output_dir, output)])


def BuildLibgccEhCmd(sourcefile, output, arch, no_nacl_gcc):
  # Return a command to compile a file from libgcc_eh (see comments in at the
  # rule definition below).
  flags_common = ['-DENABLE_RUNTIME_CHECKING', '-g', '-O2', '-W', '-Wall',
                  '-Wwrite-strings', '-Wcast-qual', '-Wstrict-prototypes',
                  '-Wmissing-prototypes', '-Wold-style-definition',
                  '-DIN_GCC', '-DCROSS_DIRECTORY_STRUCTURE', '-DIN_LIBGCC2',
                  '-D__GCC_FLOAT_NOT_NEEDED', '-Dinhibit_libc',
                  '-DHAVE_CC_TLS', '-DHIDE_EXPORTS',
                  '-fno-stack-protector', '-fexceptions',
                  '-fvisibility=hidden',
                  '-I.', '-I../.././gcc', '-I%(abs_gcc_src)s/gcc/libgcc',
                  '-I%(abs_gcc_src)s/gcc', '-I%(abs_gcc_src)s/include',
                  '-isystem', './include']
  # For x86 we use nacl-gcc to build libgcc_eh because of some issues with
  # LLVM's handling of the gcc intrinsics used in the library. See
  # https://code.google.com/p/nativeclient/issues/detail?id=1933
  # and http://llvm.org/bugs/show_bug.cgi?id=8541
  # For ARM, LLVM does work and we use it to avoid dealing with the fact that
  # arm-nacl-gcc uses different libgcc support functions than PNaCl.
  if arch in ('arm', 'mips32'):
    cc = PnaclTool('clang', arch=TranslatorArchToBiasArch(arch), msys=False)
    flags_naclcc = []
  else:
    if no_nacl_gcc:
      return command.WriteData('', output)
    os_name = pynacl.platform.GetOS()
    arch_name = pynacl.platform.GetArch()
    platform_dir = '%s_%s' % (os_name, arch_name)
    newlib_dir = 'nacl_x86_newlib_raw'

    nnacl_dir = os.path.join(NACL_DIR, 'toolchain', platform_dir,
                             newlib_dir, 'bin')
    gcc_binaries = {
        'x86-32': 'i686-nacl-gcc',
        'x86-64': 'x86_64-nacl-gcc',
    }

    cc = os.path.join(nnacl_dir, gcc_binaries[arch])
    flags_naclcc = []
  return command.Command([cc] + flags_naclcc + flags_common +
                         ['-c',
                          command.path.join('%(gcc_src)s', 'gcc', sourcefile),
                          '-o', output])



def TargetLibsSrc(GitSyncCmds):
  newlib_sys_nacl = command.path.join('%(output)s',
                                      'newlib', 'libc', 'sys', 'nacl')
  source = {
      'newlib_src': {
          'type': 'source',
          'output_dirname': 'pnacl-newlib',
          'commands': [
              # Clean any headers exported from the NaCl tree before syncing.
              command.CleanGitWorkingDir(
                  '%(output)s',
                  reset=False,
                  path=os.path.join('newlib', 'libc', 'include'))] +
              GitSyncCmds('nacl-newlib') +
              # Remove newlib versions of headers that will be replaced by
              # headers from the NaCl tree.
              [command.RemoveDirectory(command.path.join(newlib_sys_nacl,
                                                         dirname))
               for dirname in ['bits', 'sys', 'machine']] + [
              command.Command([
                  sys.executable,
                  command.path.join('%(top_srcdir)s', 'src', 'trusted',
                                    'service_runtime', 'export_header.py'),
                  command.path.join('%(top_srcdir)s', 'src', 'trusted',
                                    'service_runtime', 'include'),
                  newlib_sys_nacl],
                  cwd='%(abs_output)s',
              )] + [
              command.Copy(
                  os.path.join('%(top_srcdir)s', 'src', 'untrusted', 'pthread',
                               header),
                  os.path.join('%(output)s', 'newlib', 'libc', 'include',
                               header))
              for header in ('pthread.h', 'semaphore.h')
       ]
      },
      'compiler_rt_src': {
          'type': 'source',
          'output_dirname': 'compiler-rt',
          'commands': GitSyncCmds('compiler-rt'),
      },
      'gcc_src': {
          'type': 'source',
          'output_dirname': 'pnacl-gcc',
          'commands': GitSyncCmds('gcc'),
      },
  }
  return source


def NewlibLibcScript(arch, elfclass_x86_64='elf32'):
  template = """/*
 * This is a linker script that gets installed as libc.a for the
 * newlib-based NaCl toolchain.  It brings in the constituent
 * libraries that make up what -lc means semantically.
 */
OUTPUT_FORMAT(%s)
GROUP ( libnacl.a libcrt_common.a )
"""
  if arch == 'arm':
    # Listing three formats instead of one makes -EL/-EB switches work
    # for the endian-switchable ARM backend.
    format_list = ['elf32-littlearm-nacl',
                   'elf32-bigarm-nacl',
                   'elf32-littlearm-nacl']
  elif arch == 'i686':
    format_list = ['elf32-i386-nacl']
  elif arch == 'x86_64':
    format_list = ['%s-x86-64-nacl' % elfclass_x86_64]
  elif arch == 'mipsel' :
    format_list = ['elf32-tradlittlemips-nacl']
  else:
    raise Exception('TODO(mcgrathr): OUTPUT_FORMAT for %s' % arch)
  return template % ', '.join(['"' + fmt + '"' for fmt in format_list])


def NewlibDirectoryCmds(bias_arch, newlib_triple):
  commands = []
  def NewlibLib(name):
    return os.path.join('%(output)s', newlib_triple, 'lib', name)
  if not IsBCArch(bias_arch):
    commands.extend([
      command.Rename(NewlibLib('libc.a'), NewlibLib('libcrt_common.a')),
      command.WriteData(NewlibLibcScript(bias_arch, 'elf64'),
                        NewlibLib('libc.a'))])
  target_triple = TripleFromArch(bias_arch)
  if bias_arch != 'i686':
    commands.extend([
        # For biased bitcode builds, we configured newlib with target=le32-nacl
        # to get its pure C implementation, so rename its output dir (which
        # matches the target to the output dir for the package we are building)
        command.Rename(os.path.join('%(output)s', newlib_triple),
                       os.path.join('%(output)s', target_triple)),
        # Copy nacl_random.h, used by libc++. It uses the IRT, so should
        # be safe to include in the toolchain.
        command.Mkdir(
            os.path.join('%(output)s', target_triple, 'include', 'nacl')),
        command.Copy(os.path.join('%(top_srcdir)s', 'src', 'untrusted',
                                  'nacl', 'nacl_random.h'),
                     os.path.join('%(output)s', target_triple, 'include',
                                  'nacl', 'nacl_random.h'))
    ])
  else:
    # Use multilib-style directories for i686
    multilib_triple = TripleFromArch(MultilibArch(bias_arch))
    commands.extend([
        command.Rename(os.path.join('%(output)s', newlib_triple),
                       os.path.join('%(output)s', multilib_triple)),
        command.RemoveDirectory(
            os.path.join('%(output)s', multilib_triple, 'include')),
        command.Rename(
            os.path.join('%(output)s', multilib_triple, 'lib'),
            os.path.join('%(output)s', multilib_triple, 'lib32')),
    ])
  # Remove the 'share' directory from the biased builds; the data is
  # duplicated exactly and takes up 2MB per package.
  if bias_arch != 'le32':
    commands.append(command.RemoveDirectory(os.path.join('%(output)s','share')))
  return commands


def LibcxxDirectoryCmds(bias_arch):
  if bias_arch != 'i686':
    return []
  lib_dir = os.path.join('%(output)s', MultilibLibDir(bias_arch))
  return [
      # Use the multlib-style lib dir and shared headers for i686
      command.Mkdir(os.path.dirname(lib_dir)),
      command.Rename(os.path.join('%(output)s', 'i686-nacl', 'lib'),
                     lib_dir),
      # The only thing left in i686-nacl is the headers
      command.RemoveDirectory(os.path.join('%(output)s', 'i686-nacl')),
  ]

def D2NLibsSupportCommands(bias_arch, clang_libdir):
  def TL(lib):
    return GSDJoin(lib, pynacl.platform.GetArch3264(bias_arch))
  def TranslatorFile(lib, filename):
    return os.path.join('%(' + TL(lib) + ')s', filename)
  commands = [
              # Build compiler_rt which is now also used for the PNaCl
              # translator.
              command.Command(MakeCommand() + [
                  '-C', '%(abs_compiler_rt_src)s', 'ProjObjRoot=%(cwd)s',
                  'VERBOSE=1',
                  'AR=' + PnaclTool('ar', arch=bias_arch),
                  'RANLIB=' + PnaclTool('ranlib', arch=bias_arch),
                  'CC=' + PnaclTool('clang', arch=bias_arch), 'clang_nacl',
                  'EXTRA_CFLAGS=' + NewlibIsystemCflags(bias_arch)]),
              command.Mkdir(clang_libdir, parents=True),
              command.Copy(os.path.join(
                'clang_nacl',
                'full-' + bias_arch.replace('i686', 'i386')
                                   .replace('mipsel', 'mips32'),
                'libcompiler_rt.a'),
                 os.path.join('%(output)s', clang_libdir,
                              'libgcc.a')),
              command.Copy(
                  TranslatorFile('libgcc_eh', 'libgcc_eh.a'),
                  os.path.join('%(output)s', clang_libdir, 'libgcc_eh.a')),
              BuildTargetObjectCmd('clang_direct/crtbegin.c', 'crtbeginT.o',
                                   bias_arch, output_dir=clang_libdir),
              BuildTargetObjectCmd('crtend.c', 'crtend.o',
                                   bias_arch, output_dir=clang_libdir),
  ]
  if bias_arch == "mipsel":
       commands.extend([
           BuildTargetObjectCmd('bitcode/pnaclmm.c', 'pnaclmm.o', bias_arch),
           BuildTargetObjectCmd('clang_direct/nacl-tp-offset.c',
                                'nacl_tp_offset.o', bias_arch,
                                extra_flags=['-I%(top_srcdir)s/..']),
           command.Command([PnaclTool('ar'), 'rc',
               command.path.join(clang_libdir, 'libpnacl_legacy.a'),
               command.path.join('pnaclmm.o'),
               command.path.join('nacl_tp_offset.o')])
       ])
  return commands

def TargetLibBuildType(is_canonical):
  return 'build' if is_canonical else 'build_noncanonical'

def TargetLibs(bias_arch, is_canonical):
  def T(component_name):
    return GSDJoin(component_name, bias_arch)
  target_triple = TripleFromArch(bias_arch)
  newlib_triple = target_triple if not IsBCArch(bias_arch) else 'le32-nacl'
  newlib_cpp_flags = ''
  if IsBCArch(bias_arch):
    # This avoids putting the body of memcpy in libc for bitcode
    newlib_cpp_flags = ' -DPNACL_BITCODE'
  elif bias_arch == 'arm':
    # This ensures that nacl gets the asm version of memcpy (gcc defines this
    # macro for armv7a but clang does not)
    newlib_cpp_flags = ' -D__ARM_FEATURE_UNALIGNED'

  clang_libdir = os.path.join(
      '%(output)s', 'lib', 'clang', CLANG_VER, 'lib', target_triple)
  libc_libdir = os.path.join('%(output)s', MultilibLibDir(bias_arch))
  libs = {
      T('newlib'): {
          'type': TargetLibBuildType(is_canonical),
          'dependencies': [ 'newlib_src', 'target_lib_compiler'],
          'commands' : [
              command.SkipForIncrementalCommand(
                  ['sh', '%(newlib_src)s/configure'] +
                  TargetTools(bias_arch) +
                  ['CFLAGS_FOR_TARGET=' +
                      TargetLibCflags(bias_arch) +
                      newlib_cpp_flags,
                  '--prefix=',
                  '--disable-newlib-supplied-syscalls',
                  '--disable-texinfo',
                  '--disable-libgloss',
                  '--enable-newlib-iconv',
                  '--enable-newlib-iconv-from-encodings=' +
                  'UTF-8,UTF-16LE,UCS-4LE,UTF-16,UCS-4',
                  '--enable-newlib-iconv-to-encodings=' +
                  'UTF-8,UTF-16LE,UCS-4LE,UTF-16,UCS-4',
                  '--enable-newlib-io-long-long',
                  '--enable-newlib-io-long-double',
                  '--enable-newlib-io-c99-formats',
                  '--enable-newlib-mb',
                  '--target=' + newlib_triple
              ]),
              command.Command(MakeCommand()),
              command.Command(['make', 'DESTDIR=%(abs_output)s', 'install']),
          ] + NewlibDirectoryCmds(bias_arch, newlib_triple)
      },
      T('libcxx'): {
          'type': TargetLibBuildType(is_canonical),
          'dependencies': ['libcxx_src', 'libcxxabi_src', 'llvm_src', 'gcc_src',
                           'target_lib_compiler', T('newlib'),
                           GSDJoin('newlib', MultilibArch(bias_arch)),
                           T('libs_support')],
          'commands' :
              [command.SkipForIncrementalCommand(
                  [pnacl_commands.PrebuiltCmake(), '-G', 'Unix Makefiles',
                   '-DCMAKE_C_COMPILER_WORKS=1',
                   '-DCMAKE_CXX_COMPILER_WORKS=1',
                   '-DCMAKE_INSTALL_PREFIX=',
                   '-DCMAKE_BUILD_TYPE=Release',
                   '-DCMAKE_C_COMPILER=' + PnaclTool('clang', bias_arch),
                   '-DCMAKE_CXX_COMPILER=' + PnaclTool('clang++', bias_arch),
                   '-DCMAKE_SYSTEM_NAME=nacl',
                   '-DCMAKE_AR=' + PnaclTool('ar', bias_arch),
                   '-DCMAKE_NM=' + PnaclTool('nm', bias_arch),
                   '-DCMAKE_RANLIB=' + PnaclTool('ranlib', bias_arch),
                   '-DCMAKE_LD=' + PnaclTool('illegal', bias_arch),
                   '-DCMAKE_AS=' + PnaclTool('as', bias_arch),
                   '-DCMAKE_OBJDUMP=' + PnaclTool('illegal', bias_arch),
                   '-DCMAKE_C_FLAGS=-std=gnu11 ' + LibCxxCflags(bias_arch),
                   '-DCMAKE_CXX_FLAGS=-std=gnu++11 ' + LibCxxCflags(bias_arch),
                   '-DLIT_EXECUTABLE=' + command.path.join(
                       '%(llvm_src)s', 'utils', 'lit', 'lit.py'),
                   # The lit flags are used by the libcxx testsuite, which is
                   # currenty driven by an external script.
                   '-DLLVM_LIT_ARGS=--verbose  --param shell_prefix="' +
                    os.path.join(NACL_DIR,'run.py') +' -arch env --retries=1" '+
                    '--param exe_suffix=".pexe" --param use_system_lib=true ' +
                    '--param cxx_under_test="' + os.path.join(NACL_DIR,
                        'toolchain/linux_x86/pnacl_newlib',
                        'bin/pnacl-clang++') +
                    '" '+
                    '--param link_flags="-std=gnu++11 --pnacl-exceptions=sjlj"',
                   '-DLIBCXX_ENABLE_CXX0X=0',
                   '-DLIBCXX_ENABLE_SHARED=0',
                   '-DLIBCXX_CXX_ABI=libcxxabi',
                   '-DLIBCXX_LIBCXXABI_INCLUDE_PATHS=' + command.path.join(
                       '%(abs_libcxxabi_src)s', 'include'),
                   '%(libcxx_src)s']),
              command.Copy(os.path.join('%(gcc_src)s', 'gcc',
                                        'unwind-generic.h'),
                           os.path.join('include', 'unwind.h')),
              command.Command(MakeCommand() + ['VERBOSE=1']),
              command.Command([
                  'make',
                  'DESTDIR=' + os.path.join('%(abs_output)s', target_triple),
                  'VERBOSE=1',
                  'install']),
          ] + LibcxxDirectoryCmds(bias_arch)
      },
  }
  if IsBCArch(bias_arch):
    libs.update({
      T('compiler_rt_bc'): {
          'type': TargetLibBuildType(is_canonical),
          'dependencies': ['compiler_rt_src', 'target_lib_compiler'],
          'commands': [
              command.Mkdir(clang_libdir, parents=True),
              command.Command(MakeCommand() + [
                  '-f',
                  command.path.join('%(compiler_rt_src)s', 'lib', 'builtins',
                                    'Makefile-pnacl-bitcode'),
                  'libgcc.a', 'CC=' + PnaclTool('clang'),
                  'AR=' + PnaclTool('ar')] +
                  ['SRC_DIR=' + command.path.join('%(abs_compiler_rt_src)s',
                                                  'lib', 'builtins'),
                   'CFLAGS=' + ' '.join([
                     '-DPNACL_' + TargetArch(bias_arch).replace('-', '_')])
                  ]),
              command.Copy('libgcc.a', os.path.join(clang_libdir, 'libgcc.a')),
          ],
      },
      T('libs_support'): {
          'type': TargetLibBuildType(is_canonical),
          'dependencies': [ T('newlib'), 'target_lib_compiler'],
          'inputs': { 'src': os.path.join(NACL_DIR,
                                          'pnacl', 'support', 'bitcode')},
          'commands': [
              command.Mkdir(clang_libdir, parents=True),
              command.Mkdir(libc_libdir, parents=True),
              # Two versions of crt1.x exist, for different scenarios (with and
              # without EH).  See:
              # https://code.google.com/p/nativeclient/issues/detail?id=3069
              command.Copy(command.path.join('%(src)s', 'crt1.x'),
                           command.path.join(libc_libdir, 'crt1.x')),
              command.Copy(command.path.join('%(src)s', 'crt1_for_eh.x'),
                           command.path.join(libc_libdir, 'crt1_for_eh.x')),
              # Install crti.bc (empty _init/_fini)
              BuildTargetObjectCmd('crti.c', 'crti.bc', bias_arch,
                                    output_dir=libc_libdir),
              # Install crtbegin bitcode (__cxa_finalize for C++)
              BuildTargetObjectCmd('crtbegin.c', 'crtbegin.bc', bias_arch,
                                    output_dir=clang_libdir),
              # Stubs for _Unwind_* functions when libgcc_eh is not included in
              # the native link).
              BuildTargetObjectCmd('unwind_stubs.c', 'unwind_stubs.bc',
                                    bias_arch, output_dir=clang_libdir),
              BuildTargetObjectCmd('sjlj_eh_redirect.cc',
                                    'sjlj_eh_redirect.bc', bias_arch,
                                    output_dir=clang_libdir),
              # libpnaclmm.a (__atomic_* library functions).
              BuildTargetObjectCmd('pnaclmm.c', 'pnaclmm.bc', bias_arch),
              command.Command([
                  PnaclTool('ar'), 'rc',
                  command.path.join(clang_libdir, 'libpnaclmm.a'),
                  'pnaclmm.bc']),
          ]
      }
    })
  else:
    # For now some of the D2N support libs currently come from our native
    # translator libs (libgcc_eh). crti/crtn and crt1
    # come from libnacl, built by scons/gyp.  TODO(dschuff): Do D2N libgcc_eh.

    # Translate from bias_arch's triple-style (i686) names to the translator's
    # style (x86-32). We don't change the translator's naming scheme to avoid
    # churning the in-browser translator.
    def TL(lib):
      return GSDJoin(lib, pynacl.platform.GetArch3264(bias_arch))
    libs.update({
      T('libs_support'): {
          'type': TargetLibBuildType(is_canonical),
          'dependencies': [ 'compiler_rt_src',
                            GSDJoin('newlib', MultilibArch(bias_arch)),
                            TL('libgcc_eh'),
                            'target_lib_compiler'],
          'inputs': { 'src': os.path.join(NACL_DIR, 'pnacl', 'support'),
                      'tls_params': os.path.join(NACL_DIR, 'src', 'untrusted',
                                                 'nacl', 'tls_params.h')},
          'commands': D2NLibsSupportCommands(bias_arch, clang_libdir),
      }
    })

  return libs


def SubzeroRuntimeCommands(arch, out_dir):
  """Returns a list of commands to build the Subzero runtime.

  When the commands are executed, temporary files are created in the current
  directory, and .o files are created in the out_dir directory.  If arch isn't
  among a whitelist, an empty list of commands is returned.
  """
  AsmSourceBase = None
  # LlcArchArgs contains arguments extracted from pnacl-translate.py.
  if arch == 'x86-32-linux':
    Triple = 'i686-linux-gnu'
    LlcArchArgs = [ '-mcpu=pentium4m']
  elif arch == 'x86-32':
    Triple = 'i686-none-nacl-gnu'
    LlcArchArgs = [ '-mcpu=pentium4m']
  elif arch == 'x86-32-nonsfi':
    Triple = 'i686-linux-gnu'
    LlcArchArgs = [ '-mcpu=pentium4m', '-relocation-model=pic',
                    '-force-tls-non-pic', '-malign-double']
    AsmSourceBase = 'szrt_asm_x8632'
  elif arch == 'x86-64-linux':
    Triple = 'x86_64-none-linux-gnux32'
    LlcArchArgs = [ '-mcpu=x86-64']
  elif arch == 'x86-64':
    Triple = 'x86_64-none-nacl'
    LlcArchArgs = [ '-mcpu=x86-64']
  elif arch == 'arm-linux':
    Triple = 'arm-linux-gnu'
    LlcArchArgs = [ '-mcpu=cortex-a9', '-float-abi=hard', '-mattr=+neon']
  elif arch == 'arm':
    Triple = 'armv7a-none-nacl'
    LlcArchArgs = [ '-mcpu=cortex-a9', '-float-abi=hard', '-mattr=+neon']
  elif arch == 'arm-nonsfi':
    Triple = 'armv7a-none-linux-gnueabihf'
    LlcArchArgs = [ '-mcpu=cortex-a9', '-float-abi=hard', '-mattr=+neon',
                    '-relocation-model=pic', '-force-tls-non-pic',
                    '-malign-double']
    AsmSourceBase = 'szrt_asm_arm32'
  else:
    return []
  LlcArchArgs.append('-mtriple=' + Triple)

  return [
    command.Command([
        PnaclTool('clang'), '-O2',
        '-c', command.path.join('%(subzero_src)s', 'runtime', 'szrt.c'),
        '-o', 'szrt.tmp.bc']),
    command.Command([
        PnaclTool('opt'), '-pnacl-abi-simplify-preopt',
        '-pnacl-abi-simplify-postopt', '-pnaclabi-allow-debug-metadata',
        'szrt.tmp.bc', '-S', '-o', 'szrt.ll']),
    command.Command([
        PnaclTool('llc'), '-externalize', '-function-sections', '-O2',
        '-filetype=obj', '-bitcode-format=llvm',
        '-o', os.path.join(out_dir, 'szrt.o'),
        'szrt.ll'] +
        LlcArchArgs),
    command.Command([
        PnaclTool('llc'), '-externalize', '-function-sections', '-O2',
        '-filetype=obj', '-bitcode-format=llvm',
        '-o', os.path.join(out_dir, 'szrt_ll.o'),
        command.path.join('%(subzero_src)s', 'runtime', 'szrt_ll.ll')] +
        LlcArchArgs),
    ] + ([
      command.Command([
        PnaclTool('llvm-mc'),
        '-filetype=obj',
        '-triple=' + Triple,
        '--defsym', 'NONSFI=1',
        '-o', os.path.join(out_dir, AsmSourceBase + '.o'),
        command.path.join('%(subzero_src)s', 'runtime', AsmSourceBase + '.s')])
    ] if IsNonSFIArch(arch) else [])

def TranslatorLibs(arch, is_canonical, no_nacl_gcc):
  setjmp_arch = arch
  if setjmp_arch.endswith('-nonsfi'):
    setjmp_arch = setjmp_arch[:-len('-nonsfi')]
  bias_arch = TranslatorArchToBiasArch(arch)
  translator_lib_dir = os.path.join('translator', arch, 'lib')

  arch_cmds = []
  if arch == 'arm':
    arch_cmds.append(
        BuildTargetTranslatorCmd('aeabi_read_tp.S', 'aeabi_read_tp.o', arch))
  elif arch == 'x86-32-nonsfi':
    arch_cmds.extend(
        [BuildTargetTranslatorCmd('entry_linux.c', 'entry_linux.o', arch),
         BuildTargetTranslatorCmd('entry_linux_x86_32.S', 'entry_linux_asm.o',
                                  arch)])
  elif arch == 'arm-nonsfi':
    arch_cmds.extend(
        [BuildTargetTranslatorCmd('entry_linux.c', 'entry_linux.o', arch),
         BuildTargetTranslatorCmd('entry_linux_arm.S', 'entry_linux_asm.o',
                                  arch)])

  if not IsNonSFIArch(arch):
    def ClangLib(lib):
      return GSDJoin(lib, bias_arch)
    clang_deps = [ ClangLib('newlib'), ClangLib('libs_support') ]
    # Extract the object files from the newlib archive into our working dir.
    # libcrt_platform.a is later created by archiving all the object files
    # there.
    crt_objs = ['memmove', 'memcmp', 'memset', 'memcpy']
    if arch == 'mips32':
      crt_objs.extend(['memset-stub', 'memcpy-stub'])
    libcrt_platform_string_cmds = [command.Command([
        PnaclTool('ar'), 'x',
        os.path.join('%(' + ClangLib('newlib') + ')s',
                     MultilibLibDir(bias_arch), 'libcrt_common.a'),
        ] + ['lib_a-%s.o' % f for f in crt_objs])]
    # Copy compiler_rt from nacl-clang
    clang_libdir = os.path.join(
        'lib', 'clang', CLANG_VER, 'lib', TripleFromArch(bias_arch))
    compiler_rt_cmds = [command.Copy(
        os.path.join('%(' + ClangLib('libs_support') + ')s',
                     clang_libdir, 'libgcc.a'),
        os.path.join('%(output)s', 'libgcc.a'))]
  else:
    clang_deps = []
    extra_flags = NonSFITargetLibCflags(bias_arch).split()
    libcrt_platform_string_cmds = [BuildTargetTranslatorCmd(
        'string.c', 'string.o', arch, ['-std=c99'],
        source_dir='%(newlib_subset)s')]
    # Build compiler_rt with PNaCl
    compiler_rt_cmds = [
        command.Command(MakeCommand() + [
            '-C', '%(abs_compiler_rt_src)s', 'ProjObjRoot=%(cwd)s',
            'VERBOSE=1',
            'CC=' + PnaclTool('clang', arch=bias_arch), 'clang_nacl',
            'EXTRA_CFLAGS=' + (NewlibIsystemCflags('le32') + ' ' +
                ' '.join(extra_flags))]),
        command.Copy(os.path.join(
                'clang_nacl',
                'full-' + bias_arch.replace('i686', 'i386')
                                   .replace('mipsel', 'mips32'),
                'libcompiler_rt.a'),
            os.path.join('%(output)s', 'libgcc.a')),
    ]

  libs = {
      GSDJoin('libs_support_translator', arch): {
          'type': TargetLibBuildType(is_canonical),
          'output_subdir': translator_lib_dir,
          'dependencies': [ 'newlib_src', 'compiler_rt_src', 'subzero_src',
                            'target_lib_compiler', 'newlib_le32'] + clang_deps,
          # These libs include
          # arbitrary stuff from native_client/src/{include,untrusted,trusted}
          'inputs': { 'src': os.path.join(NACL_DIR, 'pnacl', 'support'),
                      'include': os.path.join(NACL_DIR, 'src'),
                      'newlib_subset': os.path.join(
                          NACL_DIR, 'src', 'third_party',
                          'pnacl_native_newlib_subset')},
          'commands': SubzeroRuntimeCommands(arch, '.') + [
              BuildTargetTranslatorCmd('crtbegin.c', 'crtbegin.o', arch,
                                       output_dir='%(output)s'),
              BuildTargetTranslatorCmd('crtbegin.c', 'crtbegin_for_eh.o', arch,
                                       ['-DLINKING_WITH_LIBGCC_EH'],
                                       output_dir='%(output)s'),
              BuildTargetTranslatorCmd('crtend.c', 'crtend.o', arch,
                                       output_dir='%(output)s'),
              # libcrt_platform.a
              BuildTargetTranslatorCmd('pnacl_irt.c', 'pnacl_irt.o', arch),
              BuildTargetTranslatorCmd('relocate.c', 'relocate.o', arch),
              BuildTargetTranslatorCmd(
                  'setjmp_%s.S' % setjmp_arch.replace('-', '_'),
                  'setjmp.o', arch),
              # Pull in non-errno __ieee754_fmod from newlib and rename it to
              # fmod. This is to support the LLVM frem instruction.
              BuildTargetTranslatorCmd(
                  'e_fmod.c', 'e_fmod.o', arch,
                  ['-std=c99', '-I%(abs_newlib_src)s/newlib/libm/common/',
                   '-D__ieee754_fmod=fmod'],
                  source_dir='%(abs_newlib_src)s/newlib/libm/math'),
              BuildTargetTranslatorCmd(
                  'ef_fmod.c', 'ef_fmod.o', arch,
                  ['-std=c99', '-I%(abs_newlib_src)s/newlib/libm/common/',
                   '-D__ieee754_fmodf=fmodf'],
                  source_dir='%(abs_newlib_src)s/newlib/libm/math')] +
              arch_cmds + libcrt_platform_string_cmds + [
              command.Command(' '.join([
                  PnaclTool('ar'), 'rc',
                  command.path.join('%(output)s', 'libcrt_platform.a'),
                  '*.o']), shell=True),
              # Dummy IRT shim
              BuildTargetTranslatorCmd(
                  'dummy_shim_entry.c', 'dummy_shim_entry.o', arch),
              command.Command([PnaclTool('ar'), 'rc',
                               command.path.join('%(output)s',
                                                 'libpnacl_irt_shim_dummy.a'),
                               'dummy_shim_entry.o']),
          ] + compiler_rt_cmds,
      },
  }

  if not arch.endswith('-nonsfi'):
    libs.update({
      GSDJoin('libgcc_eh', arch): {
          'type': TargetLibBuildType(is_canonical),
          'output_subdir': translator_lib_dir,
          'dependencies': [ 'gcc_src', 'target_lib_compiler'],
          'inputs': { 'scripts': os.path.join(NACL_DIR, 'pnacl', 'scripts')},
          'commands': [
              # Instead of trying to use gcc's build system to build only
              # libgcc_eh, we just build the C files and archive them manually.
              command.RemoveDirectory('include'),
              command.Mkdir('include'),
              command.Copy(os.path.join('%(gcc_src)s', 'gcc',
                           'unwind-generic.h'),
                           os.path.join('include', 'unwind.h')),
              command.Copy(os.path.join('%(scripts)s', 'libgcc-tconfig.h'),
                           'tconfig.h'),
              command.WriteData('', 'tm.h'),
              BuildLibgccEhCmd('unwind-dw2.c', 'unwind-dw2.o', arch,
                               no_nacl_gcc),
              BuildLibgccEhCmd('unwind-dw2-fde-glibc.c',
                               'unwind-dw2-fde-glibc.o', arch, no_nacl_gcc),
              command.Command([PnaclTool('ar'), 'rc',
                               command.path.join('%(output)s', 'libgcc_eh.a'),
                               'unwind-dw2.o', 'unwind-dw2-fde-glibc.o']),
          ],
      },
    })
  return libs

def UnsandboxedRuntime(arch, is_canonical):
  assert arch in ('arm-linux', 'x86-32-linux', 'x86-32-mac', 'x86-64-linux')

  compiler = {
    'arm-linux': 'arm-linux-gnueabihf-gcc',
    'x86-32-linux': 'gcc',
    'x86-32-mac': 'gcc',
    # x86-64 can't use gcc because the gcc available in the bots does not
    # support x32. clang is good enough for the task, and it is available in
    # the bots.
    'x86-64-linux': '%(abs_target_lib_compiler)s/bin/clang',
  }[arch]

  arch_cflags = {
    'arm-linux': ['-mcpu=cortex-a9', '-D__arm_nonsfi_linux__'],
    'x86-32-linux': ['-m32'],
    'x86-32-mac': ['-m32'],
    'x86-64-linux': ['-mx32'],
  }[arch]

  libs = {
      GSDJoin('unsandboxed_runtime', arch): {
          'type': TargetLibBuildType(is_canonical),
          'output_subdir': os.path.join('translator', arch, 'lib'),
          'dependencies': [ 'subzero_src', 'target_lib_compiler'],
          # This lib #includes
          # arbitrary stuff from native_client/src/{include,untrusted,trusted}
          'inputs': { 'support': os.path.join(NACL_DIR, 'src', 'nonsfi', 'irt'),
                      'untrusted': os.path.join(
                          NACL_DIR, 'src', 'untrusted', 'irt'),
                      'include': os.path.join(NACL_DIR, 'src'), },
          'commands': [
              # The NaCl headers insist on having a platform macro such as
              # NACL_LINUX defined, but src/nonsfi/irt_interfaces.c does not
              # itself use any of these macros, so defining NACL_LINUX here
              # even on non-Linux systems is OK.
              # TODO(dschuff): this include path breaks the input encapsulation
              # for build rules.
              command.Command([compiler] + arch_cflags + ['-O2', '-Wall',
                  '-Werror', '-I%(top_srcdir)s/..',
                  '-DNACL_LINUX=1', '-DDEFINE_MAIN',
                  '-c', command.path.join('%(support)s', 'irt_interfaces.c'),
                  '-o', command.path.join('%(output)s', 'unsandboxed_irt.o')]),
              command.Command([compiler] + arch_cflags + ['-O2', '-Wall',
                  '-Werror', '-I%(top_srcdir)s/..',
                  '-c', command.path.join('%(support)s', 'irt_random.c'),
                  '-o', command.path.join('%(output)s', 'irt_random.o')]),
              command.Command([compiler] + arch_cflags + ['-O2', '-Wall',
                  '-Werror', '-I%(top_srcdir)s/..',
                  '-c', command.path.join('%(untrusted)s', 'irt_query_list.c'),
                  '-o', command.path.join('%(output)s', 'irt_query_list.o')]),
          ] + SubzeroRuntimeCommands(arch, '%(output)s'),
      },
  }
  return libs


def SDKCompiler(arches):
  arch_packages = ([GSDJoin('newlib', arch) for arch in arches] +
                   [GSDJoin('libcxx', arch) for arch in arches])
  compiler = {
      'sdk_compiler': {
          'type': 'work',
          'output_subdir': 'sdk_compiler',
          'dependencies': ['target_lib_compiler'] + arch_packages,
          'commands': [
              command.CopyRecursive('%(' + t + ')s', '%(output)s')
              for t in ['target_lib_compiler'] + arch_packages],
      },
  }
  return compiler


def SDKLibs(arch, is_canonical, extra_flags=[]):
  scons_flags = ['--verbose', 'MODE=nacl', '-j%(cores)s', 'naclsdk_validate=0',
                 'pnacl_newlib_dir=%(abs_sdk_compiler)s',
                 'DESTINATION_ROOT=%(work_dir)s']
  scons_flags.extend(extra_flags)
  if arch == 'le32':
    scons_flags.extend(['bitcode=1', 'platform=x86-32'])
  elif not IsBCArch(arch):
    scons_flags.extend(['nacl_clang=1',
                        'platform=' + pynacl.platform.GetArch3264(arch)])
  else:
    raise ValueError('Should not be building SDK libs for', arch)
  libs = {
      GSDJoin('core_sdk_libs', arch): {
          'type': TargetLibBuildType(is_canonical),
          'dependencies': ['sdk_compiler', 'target_lib_compiler'],
          'inputs': {
              'src_untrusted': os.path.join(NACL_DIR, 'src', 'untrusted'),
              'src_include': os.path.join(NACL_DIR, 'src', 'include'),
              'scons.py': os.path.join(NACL_DIR, 'scons.py'),
              'site_scons': os.path.join(NACL_DIR, 'site_scons'),
          },
          'commands': [
            command.Command(
                [sys.executable, '%(scons.py)s',
                 'includedir=' +os.path.join('%(output)s',
                                             TripleFromArch(MultilibArch(arch)),
                                             'include'),
                 'libdir=' + os.path.join('%(output)s', MultilibLibDir(arch)),
                 'install'] + scons_flags,
                cwd=NACL_DIR),
          ],
      }
  }
  return libs
