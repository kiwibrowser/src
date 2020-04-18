#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Recipes for PNaCl toolchain packages.

   Recipes consist of specially-structured dictionaries, with keys for package
   name, type, commands to execute, etc. The structure is documented in the
   PackageBuilder docstring in toolchain_main.py.

   The real entry plumbing and CLI flags are also in toolchain_main.py.
"""

import fnmatch
import logging
import multiprocessing
import os
import shutil
import subprocess
import sys
import zipfile

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import pynacl.file_tools
import pynacl.gsd_storage
import pynacl.http_download
import pynacl.platform
import pynacl.repo_tools

import command
import pnacl_commands
import pnacl_sandboxed_translator
import pnacl_targetlibs
import toolchain_main

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
NACL_DIR = os.path.dirname(SCRIPT_DIR)
# Use the argparse from third_party to ensure it's the same on all platorms
python_lib_dir = os.path.join(os.path.dirname(NACL_DIR), 'third_party',
                              'python_libs', 'argparse')
sys.path.insert(0, python_lib_dir)
import argparse

PNACL_DRIVER_DIR = os.path.join(NACL_DIR, 'pnacl', 'driver')
NACL_TOOLS_DIR = os.path.join(NACL_DIR, 'tools')

# Scons tests can check this version number to decide whether to enable tests
# for toolchain bug fixes or new features.  This allows tests to be enabled on
# the toolchain buildbots/trybots before the new toolchain version is pinned
# (i.e. before the tests would pass on the main NaCl buildbots/trybots).
# If you are adding a test that depends on a toolchain change, you can
# increment this version number manually.
FEATURE_VERSION = 28

# For backward compatibility, these key names match the directory names
# previously used with gclient
GIT_REPOS = {
    'binutils': 'nacl-binutils.git',
    'clang': 'pnacl-clang.git',
    'llvm': 'pnacl-llvm.git',
    'gcc': 'pnacl-gcc.git',
    'libcxx': 'pnacl-libcxx.git',
    'libcxxabi': 'pnacl-libcxxabi.git',
    'nacl-newlib': 'nacl-newlib.git',
    'llvm-test-suite': 'pnacl-llvm-testsuite.git',
    'compiler-rt': 'pnacl-compiler-rt.git',
    'subzero': 'pnacl-subzero.git',
    'binutils-x86': 'nacl-binutils.git',
    }

GIT_BASE_URL = 'https://chromium.googlesource.com/native_client/'
GIT_PUSH_URL = 'https://chromium.googlesource.com/native_client/'
GIT_DEPS_FILE = os.path.join(NACL_DIR, 'pnacl', 'COMPONENT_REVISIONS')

ALT_GIT_BASE_URL = 'https://chromium.googlesource.com/a/native_client/'

KNOWN_MIRRORS = [('http://git.chromium.org/native_client/', GIT_BASE_URL)]
PUSH_MIRRORS = [('http://git.chromium.org/native_client/', GIT_PUSH_URL),
                (ALT_GIT_BASE_URL, GIT_PUSH_URL),
                (GIT_BASE_URL, GIT_PUSH_URL),
                ('ssh://gerrit.chromium.org/native_client/', GIT_PUSH_URL)]

PACKAGE_NAME = 'Native Client SDK [%(build_signature)s]'
BUG_URL = 'http://gonacl.com/reportissue'

# TODO(dschuff): Some of this mingw logic duplicates stuff in command.py
BUILD_CROSS_MINGW = False
# Path to the mingw cross-compiler libs on Ubuntu
CROSS_MINGW_LIBPATH = '/usr/lib/gcc/i686-w64-mingw32/4.6'
# Path and version of the native mingw compiler to be installed on Windows hosts
MINGW_PATH = os.path.join(NACL_DIR, 'mingw32')
MINGW_VERSION = 'i686-w64-mingw32-4.8.1'

CHROME_CLANG_DIR = os.path.join(os.path.dirname(NACL_DIR), 'third_party',
                                'llvm-build', 'Release+Asserts', 'bin')
CHROME_CLANG = os.path.join(CHROME_CLANG_DIR, 'clang')
CHROME_CLANGXX = os.path.join(CHROME_CLANG_DIR, 'clang++')
CHROME_SYSROOT_DIR = os.path.join(os.path.dirname(NACL_DIR), 'build',
                                  'linux', 'debian_jessie_amd64-sysroot')

try:
  # goma documentation recommends using (10 * cpu count)
  GOMA_JOBS = 10 * multiprocessing.cpu_count()
except NotImplementedError:
  # Use an arbitrary, large enough constant
  GOMA_JOBS = 100

# Required SDK version and target version for Mac builds.
# See MAC_SDK_FLAGS, below.
MAC_SDK_MIN = '10.10'
MAC_DEPLOYMENT_TARGET = '10.6'

# Redirectors are small shims acting like sym links with optional arguments.
# For mac/linux we simply use a shell script which create small redirector
# shell scripts. For windows we compile an executable which redirects to
# the target using a compiled in table.
REDIRECTOR_SCRIPT = os.path.join(NACL_TOOLS_DIR, 'create_redirector.sh')
REDIRECTOR_WIN32_SRC = os.path.join(NACL_TOOLS_DIR, 'redirector')

TOOL_X64_I686_REDIRECTS = [
    #Toolname, Tool Args
    ('as',     '--32'),
    ('ld',     '-melf_i386_nacl'),
    ]

BINUTILS_PROGS = ['addr2line', 'ar', 'as', 'c++filt', 'elfedit', 'ld',
                  'ld.bfd', 'ld.gold', 'nm', 'objcopy', 'objdump', 'ranlib',
                  'readelf', 'size', 'strings', 'strip']

TRANSLATOR_ARCHES = ('x86-32', 'x86-64', 'arm', 'mips32',
                     'x86-32-nonsfi', 'arm-nonsfi')

SANDBOXED_TRANSLATOR_ARCHES = ('x86-32', 'x86-64', 'arm', 'mips32',
                               'x86-32-nonsfi', 'arm-nonsfi')
# MIPS32 doesn't use biased bitcode, and nonsfi targets don't need it.
BITCODE_BIASES = tuple(
    bias for bias in ('le32', 'i686_bc', 'x86_64_bc', 'arm_bc'))

DIRECT_TO_NACL_ARCHES = ['x86_64', 'i686', 'arm', 'mipsel']

MAKE_DESTDIR_CMD = ['make', 'DESTDIR=%(abs_output)s']

def TripleIsWindows(t):
  return fnmatch.fnmatch(t, '*-mingw32*')

def TripleIsCygWin(t):
  return fnmatch.fnmatch(t, '*-cygwin*')

def TripleIsLinux(t):
  return fnmatch.fnmatch(t, '*-linux*')

def TripleIsMac(t):
  return fnmatch.fnmatch(t, '*-darwin*')

def TripleIsX8664(t):
  return fnmatch.fnmatch(t, 'x86_64*')

def HostIsDebug(options):
  return options.host_flavor == 'debug'

def ProgramPath(program):
  """Returns the path for the given program, or None if it doesn't exist."""
  try:
    return pynacl.file_tools.Which(program)
  except pynacl.file_tools.ExecutableNotFound:
    pass
  return None


# Determine the extra compiler flags necessary for Mac.  Do this once at
# top level, rather than every time in CompilersForHost, because running
# the external script is costly.
def MacSdkFlags():
  if not pynacl.platform.IsMac():
    return []
  mac_sdk_sysroot, mac_sdk_version = subprocess.check_output([
      sys.executable,
      os.path.join(os.path.dirname(NACL_DIR), 'build', 'mac', 'find_sdk.py'),
      '--print_sdk_path',
      MAC_SDK_MIN,
      ]).splitlines()
  return ['-isysroot', mac_sdk_sysroot,
          '-mmacosx-version-min=' + MAC_DEPLOYMENT_TARGET]

MAC_SDK_FLAGS = MacSdkFlags()


def InputsForCommands(commands):
  """Returns a dict of extra 'inputs' items for command names.
Each command name that is an absolute path gets an item named for
its basename.  The logic here is that commands named by absolute
paths do not come from the system installation and hence those
binaries themselves should be considered inputs for memoization.
"""
  inputs = {}
  for command in commands:
    if os.path.isabs(command):
      inputs[os.path.basename(command)] = command
  return inputs


# Return a tuple (C compiler, C++ compiler, ar, ranlib) of the compilers and
# tools to compile the host toolchains.
def CompilersForHost(host):
  rtn = {
      # For now we only do native builds for linux and mac
      # treat 32-bit linux like a native build
      'i686-linux': [CHROME_CLANG, CHROME_CLANGXX, 'ar', 'ranlib'],
      'x86_64-linux': [CHROME_CLANG, CHROME_CLANGXX, 'ar', 'ranlib'],
      'x86_64-apple-darwin': [CHROME_CLANG, CHROME_CLANGXX, 'ar', 'ranlib'],
      # Windows build should work for native and cross
      'i686-w64-mingw32': [
          'i686-w64-mingw32-gcc', 'i686-w64-mingw32-g++', 'ar', 'ranlib'],
      # TODO: add arm-hosted support
      'i686-pc-cygwin': ['gcc', 'g++', 'ar', 'ranlib'],
      'le32-nacl': ['pnacl-clang', 'pnacl-clang++', 'pnacl-ar', 'pnacl-ranlib'],
  }[host]

  # Allow caller to override host toolchain
  if 'CC' in os.environ:
    rtn[0] = os.environ['CC']
  if 'CXX' in os.environ:
    rtn[1] = os.environ['CXX']
  if 'AR' in os.environ:
    rtn[2] = os.environ['AR']
  if 'RANLIB' in os.environ:
    rtn[3] = os.environ['RANLIB']

  return rtn

def AflFuzzCompilers(afl_fuzz_dir):
  """Returns the AFL (clang) compiler executables, assuming afl_fuzz_dir
     is the directory containing the afl-fuzz compiler wrappers.
  """
  return ('%s/afl-%s' % (afl_fuzz_dir, 'clang'),
          '%s/afl-%s' % (afl_fuzz_dir, 'clang++'))

def AflFuzzEnvMap(host, options):
  """Returns map of environment bindings for injecting afl-fuzz
     compiler wrappers into the build for the given host.
  """
  if not options.afl_fuzz_dir:
    return {}
  cc, cxx, _, _ = CompilersForHost(host)
  return {'AFL_CC': cc, 'AFL_CXX': cxx}

def AflFuzzEnvList(host, options):
  """Returns the list of variable bindings for injectiong afl-fuzz
     compiler wrappers into the build for the given host.
  """
  arg_map = AflFuzzEnvMap(host, options)
  return sorted([key + '=' + arg_map[key] for key in arg_map])

def GomaCmakeFlags(path):
  return ['-DCMAKE_%s_COMPILER_LAUNCHER=%s' %
          (lang, os.path.join(path, 'gomacc')) for lang in ['C', 'CXX']]

def GomaCompilers(host, options):
  cc, cxx, _, _ = CompilersForHost(host)
  if not options.goma or (cc, cxx) != (CHROME_CLANG, CHROME_CLANGXX):
    return (cc, cxx)
  return (' '.join([os.path.join(options.goma, 'gomacc'), cc]),
          ' '.join([os.path.join(options.goma, 'gomacc'), cxx]))

def GomaPathDirs(host, options):
  cc, cxx, _, _ = CompilersForHost(host)
  if not options.goma or (cc, cxx) != (CHROME_CLANG, CHROME_CLANGXX):
    return []
  return [CHROME_CLANG_DIR]

def GSDJoin(*args):
  return '_'.join([pynacl.gsd_storage.LegalizeName(arg) for arg in args])

# name of a build target, including build flavor (debug/release)
def FlavoredName(component_name, host, options):
  joined_name = GSDJoin(component_name, host)
  if HostIsDebug(options):
    joined_name= joined_name + '_debug'
  return joined_name


def HostArchToolFlags(host, extra_cflags, opts):
  """Return the appropriate CFLAGS, CXXFLAGS, and LDFLAGS based on host
  and opts. Does not attempt to determine flags that are attached
  to CC and CXX directly.

  Returns the tuple (flags, deps) where 'flags' is a dictionary mapping
  'CFLAGS' et al to a list of arguments, and 'deps' is a list of extra
  dependencies for a component using these flags.
  """
  extra_cc_flags = list(extra_cflags)
  result = { 'LDFLAGS' : [],
             'CFLAGS' : [],
             'CXXFLAGS' : []}
  deps = []
  if TripleIsWindows(host):
    result['LDFLAGS'] += ['-L%(abs_libdl)s', '-ldl']
    result['CFLAGS'] += ['-isystem','%(abs_libdl)s']
    result['CXXFLAGS'] += ['-isystem', '%(abs_libdl)s']
    deps.append('libdl')
  else:
    if TripleIsLinux(host) and not TripleIsX8664(host):
      # Chrome clang defaults to 64-bit builds, even when run on 32-bit Linux.
      extra_cc_flags += ['-m32']
    if opts.gcc or host == 'le32-nacl':
      result['CFLAGS']  += extra_cc_flags
      result['CXXFLAGS'] += extra_cc_flags
    else:
      result['CFLAGS'] += extra_cc_flags
      result['LDFLAGS'] += ['-L%(' + FlavoredName('abs_libcxx',
                                                  host, opts) + ')s/lib']
      result['CXXFLAGS'] += ([
        '-stdlib=libc++',
        '-nostdinc++',
        '-I%(' + FlavoredName('abs_libcxx', host, opts) + ')s/include/c++/v1'] +
        extra_cc_flags)
      deps.append(FlavoredName('libcxx', host, opts))
  return result, deps


def ConfigureHostArchFlags(host, extra_cflags, options, extra_configure=None,
                           use_afl_fuzz=False):
  """Return flags passed to LLVM and binutils configure for compilers and
  compile flags.

  Returns the tuple (flags, inputs, deps) where 'flags' is a list of
  arguments to configure, 'inputs' is a dict of extra inputs to be hashed,
  and 'deps' is a list of extra dependencies for a component using these flags.
  """
  configure_args = []
  extra_cc_args = []

  configure_args += options.extra_configure_args
  if extra_configure is not None:
    configure_args += extra_configure
  if options.extra_cc_args is not None:
    extra_cc_args += [options.extra_cc_args]

  native = pynacl.platform.PlatformTriple()
  is_cross = host != native

  if TripleIsLinux(host) and not TripleIsX8664(host):
    assert TripleIsLinux(native)
    # NaCl/Chrome buildbots run 32 bit userspace on a 64 bit kernel. configure
    # guesses that the host is 64-bit even though we want a 32-bit build. But
    # it's still "native enough", so force --build rather than --host.
    # PlatformTriple() returns 32-bit, so this does not appear as a cross build
    # here.
    configure_args.append('--build=' + host)
  elif is_cross:
    configure_args.append('--host=' + host)

  extra_cxx_args = list(extra_cc_args)

  hashables = []

  if not options.gcc:
    cc, cxx, ar, ranlib = CompilersForHost(host)
    hashables += [cc, cxx, ar, ranlib]

    if options.goma:
      cc, cxx = GomaCompilers(host, options)

    # Introduce afl-fuzz compiler wrappers if needed.
    if use_afl_fuzz:
      cc, cxx = AflFuzzCompilers(options.afl_fuzz_dir)

    if ProgramPath('ccache'):
      # Set CCACHE_CPP2 envvar, to avoid an error due to a strange
      # ccache/clang++ interaction.  Specifically, errors about
      # "argument unused during compilation".
      os.environ['CCACHE_CPP2'] = 'yes'
      cc_list = ['ccache', cc]
      cxx_list = ['ccache', cxx]
      extra_cc_args += ['-Qunused-arguments']
      extra_cxx_args += ['-Qunused-arguments']
    else:
      cc_list = [cc]
      cxx_list = [cxx]

    if TripleIsMac(host):
      cc_list += MAC_SDK_FLAGS
      cxx_list += MAC_SDK_FLAGS
    elif TripleIsLinux(host):
      # Unfortunately these cannot go in HostArchToolFlags because configure
      # does not pass CFLAGS to its compile tests
      cc_list += ['--sysroot=%s' % CHROME_SYSROOT_DIR]
      cxx_list += ['--sysroot=%s' % CHROME_SYSROOT_DIR]

    configure_args.append('CC=' + ' '.join(cc_list + extra_cc_args))
    configure_args.append('CXX=' + ' '.join(cxx_list + extra_cxx_args))
    configure_args.append('AR=' + ar)
    configure_args.append('RANLIB=' + ranlib)

  tool_flags, tool_deps = HostArchToolFlags(host, extra_cflags, options)
  configure_args.extend(
       ['CFLAGS=' + ' '.join(tool_flags['CFLAGS']),
        'CXXFLAGS=' + ' '.join(tool_flags['CXXFLAGS']),
        'LDFLAGS=' + ' '.join(tool_flags['LDFLAGS']),
       ])
  if TripleIsWindows(host):
    # The i18n support brings in runtime dependencies on MinGW DLLs
    # that we don't want to have to distribute alongside our binaries.
    # So just disable it, and compiler messages will always be in US English.
    configure_args.append('--disable-nls')
    if is_cross:
      # LLVM's linux->mingw cross build needs this
      configure_args.append('CC_FOR_BUILD=gcc')
  return configure_args, InputsForCommands(hashables), tool_deps


def LibCxxHostArchFlags(host, options):
  cc, cxx, _, _ = CompilersForHost(host)
  hashables = [cc, cxx]
  cmake_flags = []
  if options.goma:
    cmake_flags.extend(GomaCmakeFlags(options.goma))

  cmake_flags.extend(['-DCMAKE_C_COMPILER='+cc, '-DCMAKE_CXX_COMPILER='+cxx])
  if TripleIsLinux(host):
    cflags = ['--sysroot=%s' % CHROME_SYSROOT_DIR]
    if not TripleIsX8664(host):
      # Chrome clang defaults to 64-bit builds, even when run on 32-bit Linux
      cflags.extend(['-m32'])
    cmake_flags.extend(['-DCMAKE_C_FLAGS=%s' % ' '.join(cflags),
                        '-DCMAKE_CXX_FLAGS=%s' % ' '.join(cflags)])
  elif TripleIsMac(host):
    sdk_flags = ' '.join(MAC_SDK_FLAGS)
    cmake_flags.extend(['-DCMAKE_C_FLAGS=' + sdk_flags,
                        '-DCMAKE_CXX_FLAGS=' + sdk_flags])
  return cmake_flags, InputsForCommands(hashables)


def CmakeHostArchFlags(host, options):
  """Set flags passed to LLVM cmake for compilers and compile flags.

  Returns the tuple (flags, inputs, deps) where 'flags' is a list of
  arguments to cmake, 'inputs' is a dict of extra inputs to be hashed,
  and 'deps' is a list of extra dependencies for a component using these flags.
  """
  cmake_flags = []
  if options.afl_fuzz_dir:
    cc, cxx = AflFuzzCompilers(options.afl_fuzz_dir)
  else:
    cc, cxx, _, _ = CompilersForHost(host)
  hashables = [cc, cxx]
  if options.goma:
    cmake_flags.extend(GomaCmakeFlags(options.goma))

  cmake_flags.extend(['-DCMAKE_C_COMPILER='+cc, '-DCMAKE_CXX_COMPILER='+cxx])
  if ProgramPath('ccache'):
    cmake_flags.extend(['-DSYSTEM_HAS_CCACHE=ON'])

  # There seems to be a bug in chrome clang where it exposes the msan interface
  # (even when compiling without msan) but then does not link with an
  # msan-enabled compiler_rt, leaving references to __msan_allocated_memory
  # undefined.
  cmake_flags.append('-DHAVE_SANITIZER_MSAN_INTERFACE_H=FALSE')
  tool_flags, tool_deps = HostArchToolFlags(host, [], options)
  cflags = tool_flags['CFLAGS'] + ['--sysroot=%s' % CHROME_SYSROOT_DIR]
  cxxflags = tool_flags['CXXFLAGS'] + ['--sysroot=%s' % CHROME_SYSROOT_DIR]
  if TripleIsMac(host):
    cflags = MAC_SDK_FLAGS + cflags
    cxxflags = MAC_SDK_FLAGS + cxxflags
  cmake_flags.append('-DCMAKE_C_FLAGS=' + ' '.join(cflags))
  cmake_flags.append('-DCMAKE_CXX_FLAGS=' + ' '.join(cxxflags))
  for linker_type in ['EXE', 'SHARED', 'MODULE']:
    cmake_flags.extend([('-DCMAKE_%s_LINKER_FLAGS=' % linker_type) +
                        ' '.join(tool_flags['LDFLAGS'])])
  return cmake_flags, InputsForCommands(hashables), tool_deps


def ConfigureBinutilsCommon(host, options, is_pnacl):
  # Binutils still has some warnings when building with clang
  if not options.gcc:
    warning_flags = ['-Wno-extended-offsetof', '-Wno-absolute-value',
                    '-Wno-unused-function', '-Wno-unused-const-variable',
                    '-Wno-unneeded-internal-declaration',
                    '-Wno-unused-private-field', '-Wno-format-security']
  else:
    warning_flags = ['-Wno-unused-function', '-Wno-unused-value']

  host_arch_flags, inputs, deps = ConfigureHostArchFlags(
      host, warning_flags, options,
      options.binutils_extra_configure if is_pnacl else None)

  flags = [
      '--with-pkgversion=' + PACKAGE_NAME,
      '--with-bugurl=' + BUG_URL,
      '--without-zlib',
      '--prefix=',
      '--disable-silent-rules',
      '--enable-deterministic-archives',
      ] + host_arch_flags

  return flags, inputs, deps

def LLVMConfigureAssertionsFlags(options):
  if options.enable_llvm_assertions:
    return []
  else:
    return ['--disable-debug', '--disable-assertions']


def MakeCommand(host, options):
  make_command = ['make']
  if not pynacl.platform.IsWindows() or pynacl.platform.IsCygWin():
    # The make that ships with msys sometimes hangs when run with -j.
    # The ming32-make that comes with the compiler itself reportedly doesn't
    # have this problem, but it has issues with pathnames with LLVM's build.
    if options.goma:
      make_command.append('-j%s' % GOMA_JOBS)
    else:
      make_command.append('-j%(cores)s')

  if TripleIsWindows(host):
    # There appears to be nothing we can pass at top-level configure time
    # that will prevent the configure scripts from finding MinGW's libiconv
    # and using it.  We have to force this variable into the environment
    # of the sub-configure runs, which are run via make.
    make_command.append('HAVE_LIBICONV=no')
  return make_command


def NinjaCommand(host, options):
  ninja_command = ['ninja']
  if options.goma:
    ninja_command.append('-j%s' % GOMA_JOBS)
  return ninja_command


def CopyWindowsHostLibs(host):
  if not TripleIsWindows(host) and not TripleIsCygWin(host):
    return []

  if TripleIsCygWin(host):
    lib_path = '/bin'
    libs = ('cyggcc_s-1.dll', 'cygiconv-2.dll', 'cygwin1.dll',
            'cygintl-8.dll', 'cygstdc++-6.dll', 'cygz.dll')
  elif pynacl.platform.IsWindows():
    lib_path = os.path.join(MINGW_PATH, 'bin')
    # The native minGW compiler uses winpthread, but the Ubuntu cross compiler
    # does not.
    libs = ('libgcc_s_sjlj-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll')
  else:
    lib_path = os.path.join(CROSS_MINGW_LIBPATH)
    libs = ('libgcc_s_sjlj-1.dll', 'libstdc++-6.dll')
  return [command.Copy(
                  os.path.join(lib_path, lib),
                  os.path.join('%(output)s', 'bin', lib))
               for lib in libs]

def GetGitSyncCmdsCallback(revisions):
  """Return a callback which returns the git sync commands for a component.

     This allows all the revision information to be processed here while giving
     other modules like pnacl_targetlibs.py the ability to define their own
     source targets with minimal boilerplate.
  """
  def GetGitSyncCmds(component):
    git_url = GIT_BASE_URL + GIT_REPOS[component]
    git_push_url = GIT_PUSH_URL + GIT_REPOS[component]

    return (command.SyncGitRepoCmds(git_url, '%(output)s', revisions[component],
                                    git_cache='%(git_cache_dir)s',
                                    push_url=git_push_url,
                                    known_mirrors=KNOWN_MIRRORS,
                                    push_mirrors=PUSH_MIRRORS) +
            [command.Runnable(lambda opts: opts.IsBot(),
                              pnacl_commands.CmdCheckoutGitBundleForTrybot,
                              component, '%(output)s')])

  return GetGitSyncCmds


def HostToolsSources(GetGitSyncCmds):
  sources = {
      'libcxx_src': {
          'type': 'source',
          'output_dirname': 'libcxx',
          'commands': GetGitSyncCmds('libcxx'),
      },
      'libcxxabi_src': {
          'type': 'source',
          'output_dirname': 'libcxxabi',
          'commands': GetGitSyncCmds('libcxxabi'),
      },
      'binutils_src': {
          'type': 'source',
          'output_dirname': 'binutils',
          'commands': GetGitSyncCmds('binutils'),
      },
      # For some reason, the llvm build using --with-clang-srcdir chokes if the
      # clang source directory is named something other than 'clang', so don't
      # change output_dirname for clang.
      'clang_src': {
          'type': 'source',
          'output_dirname': 'clang',
          'commands': GetGitSyncCmds('clang'),
      },
      'llvm_src': {
          'type': 'source',
          'output_dirname': 'llvm',
          'commands': GetGitSyncCmds('llvm'),
      },
      'subzero_src': {
          'type': 'source',
          'output_dirname': 'subzero',
          'commands': GetGitSyncCmds('subzero'),
      },
      'binutils_x86_src': {
          'type': 'source',
          'output_dirname': 'binutils-x86',
          'commands': GetGitSyncCmds('binutils-x86'),
      },
  }
  return sources

def TestsuiteSources(GetGitSyncCmds):
  sources = {
      'llvm_testsuite_src': {
          'type': 'source',
          'output_dirname': 'llvm-test-suite',
          'commands': GetGitSyncCmds('llvm-test-suite'),
      },
  }
  return sources


def CopyHostLibcxxForLLVMBuild(host, dest, options):
  """Copy libc++ to the working directory for build tools."""
  if options.gcc:
    return []
  if TripleIsLinux(host):
    libname = 'libc++.so.1'
  elif TripleIsMac(host):
    libname = 'libc++.1.dylib'
  else:
    return []
  return [command.Mkdir(dest, parents=True),
          command.Copy('%(' +
                       FlavoredName('abs_libcxx', host, options) +')s/lib/' +
                       libname, os.path.join(dest, libname))]

def CreateSymLinksToDirectToNaClTools(host):
  if host == 'le32-nacl':
    return []
  return (
      [command.Command(['ln', '-f',
                        command.path.join('%(output)s', 'bin','clang'),
                        command.path.join('%(output)s', 'bin',
                                          arch + '-nacl-clang')])
       for arch in DIRECT_TO_NACL_ARCHES] +
      [command.Command(['ln', '-f',
                        command.path.join('%(output)s', 'bin','clang'),
                        command.path.join('%(output)s', 'bin',
                                          arch + '-nacl-clang++')])
       for arch in DIRECT_TO_NACL_ARCHES])

def CreateSymLinksToPNaClTools(host):
  return (
      [command.Command(['ln', '-f',
                        command.path.join('%(output)s', 'bin', tool),
                        command.path.join('%(output)s', 'bin',
                                          'le32-nacl-' + tool)])
       for tool in ['clang', 'clang++']] +
      [command.Command(['ln', '-f',
                        command.path.join('%(output)s', 'bin',
                                          'llvm-' + tool),
                        command.path.join('%(output)s', 'bin',
                                          'le32-nacl-' + tool)])
       for tool in ['ar', 'nm', 'ranlib', 'as']])

def HostLibs(host, options):
  def H(component_name):
    # Return a package name for a component name with a host triple.
    return FlavoredName(component_name, host, options)
  libs = {}
  if TripleIsWindows(host):
    if pynacl.platform.IsWindows():
      ar = 'ar'
    else:
      ar = 'i686-w64-mingw32-ar'

    libs.update({
      'libdl': {
          'type': 'build',
          'inputs' : { 'src' : os.path.join(NACL_DIR, '..', 'third_party',
                                            'dlfcn-win32') },
          'commands': [
              command.CopyTree('%(src)s', 'src'),
              command.Command(['i686-w64-mingw32-gcc',
                               '-o', 'dlfcn.o', '-c',
                               os.path.join('src', 'dlfcn.c'),
                               '-Wall', '-O3', '-fomit-frame-pointer']),
              command.Command([ar, 'cru',
                               'libdl.a', 'dlfcn.o']),
              command.Copy('libdl.a',
                           os.path.join('%(output)s', 'libdl.a')),
              command.Copy(os.path.join('src', 'dlfcn.h'),
                           os.path.join('%(output)s', 'dlfcn.h')),
          ],
      },
    })
  elif not options.gcc:
    # Libc++ is only tested with the clang build
    libcxx_host_arch_flags, libcxx_inputs = LibCxxHostArchFlags(host, options)
    libs.update({
        H('libcxx'): {
            'dependencies': ['libcxx_src', 'libcxxabi_src'],
            'type': 'build',
            'inputs': libcxx_inputs,
            'commands': [
                command.SkipForIncrementalCommand([
                     pnacl_commands.PrebuiltCmake(), '-G', 'Unix Makefiles'] +
                     libcxx_host_arch_flags +
                     ['-DLIBCXX_CXX_ABI=libcxxabi',
                      '-DLIBCXX_LIBCXXABI_INCLUDE_PATHS=' + command.path.join(
                          '%(abs_libcxxabi_src)s', 'include'),
                      '-DLIBCXX_ENABLE_SHARED=ON',
                      '-DCMAKE_INSTALL_PREFIX=',
                      '-DCMAKE_INSTALL_NAME_DIR=@executable_path/../lib',
                      '%(libcxx_src)s'],
                    path_dirs=GomaPathDirs(host, options)),
                command.Command(MakeCommand(host, options) + ['VERBOSE=1'],
                                path_dirs=GomaPathDirs(host, options)),
                command.Command(MAKE_DESTDIR_CMD + ['VERBOSE=1', 'install']),
            ],
        },
    })
  return libs


def HostTools(host, options):
  def H(component_name):
    # Return a package name for a component name with a host triple.
    return FlavoredName(component_name, host, options)
  # Return the file name with the appropriate suffix for an executable file.
  def Exe(file):
    if TripleIsWindows(host):
      return file + '.exe'
    else:
      return file

  # TODO(jfb): gold's build currently generates the following error on Windows:
  #            too many arguments for format.
  binutils_do_werror = not TripleIsWindows(host)
  # TODO(mcgrathr): With post-3.7 Clang, binutils (opcodes) build gets:
  #     error: shifting a negative signed value is undefined
  #            [-Werror,-Wshift-negative-value]
  binutils_do_werror = False
  extra_gold_deps = []
  install_step = 'install-strip'
  if host == 'le32-nacl':
    # TODO(bradnelson): Fix warnings so this can go away.
    binutils_do_werror = False
    extra_gold_deps = [H('llvm')]
    install_step = 'install'

  # The binutils git checkout includes all the directories in the
  # upstream binutils-gdb.git repository, but some of these
  # directories are not included in a binutils release tarball.  The
  # top-level Makefile will try to build whichever of the whole set
  # exist, but we don't want these extra directories built.  So we
  # stub them out by creating dummy <subdir>/Makefile files; having
  # these exist before the configure-<subdir> target in the
  # top-level Makefile runs prevents it from doing anything.
  binutils_dummy_dirs = ['gdb', 'libdecnumber', 'readline', 'sim']
  def DummyDirCommands(dirs):
    dummy_makefile = """\
.DEFAULT:;@echo Ignoring $@
"""
    commands = []
    for dir in dirs:
      commands.append(command.Mkdir(dir))
      commands.append(command.WriteData(
        dummy_makefile, command.path.join(dir, 'Makefile')))
    return commands

  binutils_flags, binutils_inputs, binutils_deps = ConfigureBinutilsCommon(
      host, options, True)
  binutils_inputs['macros'] = os.path.join(
      NACL_DIR, 'pnacl', 'support', 'clang_direct', 'nacl-arm-macros.s')
  tools = {
      # The binutils package is used both for bitcode linking (gold) and
      # for its conventional use with arm-nacl-clang and mipsel-nacl-clang.
      H('binutils'): {
          'dependencies': (['binutils_src'] +
                           extra_gold_deps + binutils_deps),
          'type': 'build',
          'inputs' : binutils_inputs,
          'commands': [
              command.SkipForIncrementalCommand([
                  'sh',
                  '%(binutils_src)s/configure'] + binutils_flags +
                  [
                  '--enable-gold',
                  '--enable-plugins',
                  '--disable-shared',
                  '--enable-targets=arm-nacl,i686-nacl,x86_64-nacl,mipsel-nacl',
                  '--enable-werror=' + ('yes' if binutils_do_werror else 'no'),
                  '--program-prefix=arm-nacl-',
                  '--target=arm-nacl',
                  '--with-sysroot=/arm-nacl',
                  ], path_dirs=GomaPathDirs(host, options))] +
              DummyDirCommands(binutils_dummy_dirs) + [
              command.Command(MakeCommand(host, options),
                              path_dirs=GomaPathDirs(host, options)),
              command.Command(MAKE_DESTDIR_CMD + [install_step])] +
              [command.RemoveDirectory(os.path.join('%(output)s', dir))
               for dir in ('lib', 'lib32')] +
              # Since it has dual use, just create links for both sets of names
              # (i.e. arm-nacl-foo and mipsel-nacl-foo)
              # TODO(dschuff): Use the redirector scripts here like binutils_x86
              [command.Command([
                  'ln', '-f',
                   command.path.join('%(output)s', 'bin', 'arm-nacl-' + tool),
                   command.path.join('%(output)s', 'bin',
                                                   'mipsel-nacl-' + tool)])
               for tool in BINUTILS_PROGS] +
              # BFD ld is the default for nacl-clang.
              [command.Command([
                  'ln', '-f',
                  command.path.join('%(output)s', 'arm-nacl', 'bin', 'ld.bfd'),
                  command.path.join('%(output)s', 'arm-nacl', 'bin', 'ld')]),
               command.Copy(
                  '%(macros)s',
                  os.path.join(
                      '%(output)s', 'arm-nacl', 'lib', 'nacl-arm-macros.s'))] +
              # Gold is the mipsel-nacl linker.
              [command.Command([
                  'ln', '-f',
                  command.path.join('%(output)s', 'bin', 'mipsel-nacl-ld.gold'),
                  command.path.join('%(output)s', 'bin', 'mipsel-nacl-ld')])] +
              # Gold is used as a linker for PNaCl
              [command.Command([
                  'ln', '-f',
                  command.path.join('%(output)s', 'bin', 'arm-nacl-ld.gold'),
                  command.path.join('%(output)s', 'bin', 'le32-nacl-ld.gold')]),
               command.Command([
                  'ln', '-f',
                  command.path.join('%(output)s', 'bin', 'le32-nacl-ld.gold'),
                  command.path.join('%(output)s', 'bin', 'le32-nacl-ld')])]
      },
      H('driver'): {
        'type': 'build',
        'output_subdir': 'bin',
        'inputs': { 'src': PNACL_DRIVER_DIR },
        'commands': [
            command.Runnable(
                None,
                pnacl_commands.InstallDriverScripts,
                '%(src)s', '%(output)s',
                host_windows=TripleIsWindows(host) or TripleIsCygWin(host),
                host_64bit=TripleIsX8664(host))
        ],
      },
  }

  asan_flags = []
  if options.sanitize:
    asan_flags.append('-DLLVM_USE_SANITIZER=%s' % options.sanitize.capitalize())

  # TODO(jfb) Windows currently uses MinGW's GCC 4.8.1 which generates warnings
  #           on upstream LLVM code. Turn on -Werror once these are fixed.
  #           The same applies for the default GCC on current Ubuntu.
  # TODO(mcgrathr): The latest Clang gets -Wredunant-move errors under
  # -Werror, and llvm's configure makes it difficult to get -Wno-... options
  # in the place they need to be, so just disable -Werror for now.
  # After the next LLVM merge, the LLVM sources will probably be compatible
  # with the latest Clang and -Werror.
  llvm_do_werror = False and not (TripleIsWindows(host) or options.gcc)

  # Older CMake ignore CMAKE_*_LINKER_FLAGS during config step.
  # https://public.kitware.com/Bug/view.php?id=14066
  # The workaround is to set LDFLAGS in the environment.
  tool_flags, tool_deps = HostArchToolFlags(host, [], options)
  llvm_cmake_config_env = {'LDFLAGS': ' '.join(tool_flags['LDFLAGS'])}
  llvm_cmake_config_env.update(AflFuzzEnvMap(host, options))

  llvm_host_arch_flags, llvm_inputs, llvm_deps = CmakeHostArchFlags(
      host, options)
  llvm_deps = list(set(tool_deps + llvm_deps))
  llvm_inputs['test_xfails'] = os.path.join(NACL_DIR, 'pnacl', 'scripts')
  llvm_cmake = {
      H('llvm'): {
          'dependencies': ['clang_src', 'llvm_src', 'binutils_src',
                           'subzero_src'] + llvm_deps,
          'inputs': llvm_inputs,
          'type': 'build',
          'commands': [
              command.SkipForIncrementalCommand([
                  pnacl_commands.PrebuiltCmake(), '-G', 'Ninja'] +
                  llvm_host_arch_flags + asan_flags +
                  [
                  '-DBUILD_SHARED_LIBS=ON',
                  '-DCMAKE_BUILD_TYPE=' + ('Debug' if HostIsDebug(options)
                                           else 'Release'),
                  '-DCMAKE_INSTALL_PREFIX=%(output)s',
                  '-DCMAKE_INSTALL_RPATH=$ORIGIN/../lib',
                  '-DLLVM_APPEND_VC_REV=ON',
                  '-DLLVM_BINUTILS_INCDIR=%(abs_binutils_src)s/include',
                  '-DLLVM_BUILD_TESTS=ON',
                  '-DLLVM_ENABLE_ASSERTIONS=ON',
                  '-DLLVM_ENABLE_LIBCXX=OFF',
                  '-LLVM_ENABLE_WERROR=' + ('ON' if llvm_do_werror else 'OFF'),
                  '-DLLVM_ENABLE_ZLIB=OFF',
                  '-DLLVM_EXTERNAL_CLANG_SOURCE_DIR=%(clang_src)s',
                  '-DLLVM_EXTERNAL_SUBZERO_SOURCE_DIR=%(subzero_src)s',
                  '-DLLVM_INSTALL_UTILS=ON',
                  '-DCLANG_ENABLE_LIBXML=OFF',
                  '-DLLVM_TARGETS_TO_BUILD=X86;ARM;Mips',
                  '-DSUBZERO_TARGETS_TO_BUILD=ARM32;MIPS32;X8632;X8664',
                  '%(llvm_src)s'],
                  env=llvm_cmake_config_env,
                  path_dirs=GomaPathDirs(host, options))] +
              CopyHostLibcxxForLLVMBuild(host, 'lib', options) +
              [command.Command(NinjaCommand(host, options) + ['-v'],
                               path_dirs=GomaPathDirs(host, options),
                               env=AflFuzzEnvMap(host, options)),
               command.Command(NinjaCommand(host, options) + ['install'])] +
              CreateSymLinksToDirectToNaClTools(host) +
              CreateSymLinksToPNaClTools(host)
      },
  }
  cleanup_static_libs = []
  shared = []
  if host != 'le32-nacl':
    shared = ['--enable-shared']
    cleanup_static_libs = [
        command.Remove(*[os.path.join('%(output)s', 'lib', f) for f
                         in '*.a', '*Hello.*', 'BugpointPasses.*']),
    ]
  llvm_host_arch_flags, llvm_inputs, llvm_deps = ConfigureHostArchFlags(
      host, [], options, use_afl_fuzz=options.afl_fuzz_dir)
  llvm_inputs.update({'test_xfails': os.path.join(NACL_DIR,
                                                  'pnacl', 'scripts')})
  llvm_autoconf = {
      H('llvm'): {
          'dependencies': ['clang_src', 'llvm_src', 'binutils_src',
                           'subzero_src'] + llvm_deps,
          'inputs': llvm_inputs,
          'type': 'build',
          'commands': [
              command.SkipForIncrementalCommand([
                  'sh',
                  '%(llvm_src)s/configure'] +
                  AflFuzzEnvList(host, options) +
                  llvm_host_arch_flags +
                  LLVMConfigureAssertionsFlags(options) +
                  [
                   '--disable-bindings', # ocaml is currently the only binding.
                   '--disable-jit',
                   '--disable-terminfo',
                   # Nothing built here actually uses -ledit for anything,
                   # but configure will decide to use it and then the tools
                   # buildt will have a runtime dependency on libedit.  On
                   # newer Mac versions, this gets a dependency on
                   # libedit.3.dylib--but older Mac installs have only
                   # libedit.2.dylib available.
                   '--disable-libedit',
                   '--disable-zlib',
                   '--enable-optimized=' + ('no' if HostIsDebug(options)
                                            else 'yes'),
                   '--enable-debug=' + ('yes' if HostIsDebug(options)
                                        else 'no'),
                   '--enable-targets=x86,arm,mips',
                   '--enable-subzero-targets=ARM32,MIPS32,X8632,X8664',
                   '--enable-werror=' + ('yes' if llvm_do_werror else 'no'),
                   # Backtraces require TLS, which is missing on OSX 10.6
                   '--enable-backtraces=' + ('no' if TripleIsMac(host)
                                             else 'yes'),
                   '--prefix=/',
                   '--program-prefix=',
                   '--with-binutils-include=%(abs_binutils_src)s/include',
                   '--with-clang-srcdir=%(abs_clang_src)s',
                   'ac_cv_have_decl_strerror_s=no',
                  ] + shared,
                  path_dirs=GomaPathDirs(host, options))] +
              CopyHostLibcxxForLLVMBuild(
                  host,
                  os.path.join(('Debug+Asserts' if HostIsDebug(options)
                                else 'Release+Asserts'), 'lib'),
                  options) +
              [command.Command(MakeCommand(host, options) +
                  AflFuzzEnvList(host, options) + [
                  'VERBOSE=1',
                  'PNACL_BROWSER_TRANSLATOR=0',
                  'SUBZERO_SRC_ROOT=%(abs_subzero_src)s',
                  'all'],
                  path_dirs=GomaPathDirs(host, options)),
              command.Command(MAKE_DESTDIR_CMD + [
                  'SUBZERO_SRC_ROOT=%(abs_subzero_src)s',
                  'install'])] +
              cleanup_static_libs + [
              command.Remove(*[os.path.join('%(output)s', 'bin', f) for f in
                               Exe('clang-format'), Exe('clang-check'),
                               Exe('c-index-test'), Exe('clang-tblgen'),
                               Exe('llvm-tblgen')])] +
              CreateSymLinksToDirectToNaClTools(host) +
              CreateSymLinksToPNaClTools(host) +
              CopyWindowsHostLibs(host),
      },
  }
  if options.cmake:
    tools.update(llvm_cmake)
  else:
    tools.update(llvm_autoconf)
  return tools


def TargetLibCompiler(host, options):
  def H(component_name):
    return FlavoredName(component_name, host, options)
  compiler = {
      # Because target_lib_compiler is not a memoized target, its name doesn't
      # need to have the host appended to it (it can be different on different
      # hosts), which means that target library build rules don't have to care
      # what host they run on; they can just depend on 'target_lib_compiler'
      'target_lib_compiler': {
          'type': 'work',
          'output_subdir': 'target_lib_compiler',
          'dependencies': [ H('binutils'), H('binutils_x86'), H('llvm') ],
          'inputs': { 'driver': PNACL_DRIVER_DIR },
          'commands': [
              command.CopyRecursive('%(' + t + ')s', '%(output)s')
              for t in [H('llvm'), H('binutils'), H('binutils_x86')]] + [
              command.Runnable(
                  None, pnacl_commands.InstallDriverScripts,
                  '%(driver)s', os.path.join('%(output)s', 'bin'),
                  host_windows=TripleIsWindows(host) or TripleIsCygWin(host),
                  host_64bit=TripleIsX8664(host))
          ]
      },
  }

  if TripleIsWindows(host) or not options.gcc:
    host_lib = 'libdl' if TripleIsWindows(host) else H('libcxx')
    compiler['target_lib_compiler']['dependencies'].append(host_lib)
    compiler['target_lib_compiler']['commands'].append(
        command.CopyRecursive('%(' + host_lib + ')s', '%(output)s'))
  return compiler


def Metadata(revisions, is_canonical):
  data = {
      'metadata': {
          'type': 'build' if is_canonical else 'build_noncanonical',
          'inputs': { 'readme': os.path.join(NACL_DIR, 'pnacl', 'README'),
                      'COMPONENT_REVISIONS': GIT_DEPS_FILE,
                      'driver': PNACL_DRIVER_DIR },
          'commands': [
              command.Copy('%(readme)s', os.path.join('%(output)s', 'README')),
              command.WriteData(str(FEATURE_VERSION),
                                os.path.join('%(output)s', 'FEATURE_VERSION')),
              command.Runnable(None, pnacl_commands.WriteREVFile,
                               os.path.join('%(output)s', 'REV'),
                               GIT_BASE_URL,
                               GIT_REPOS,
                               revisions),
          ],
      }
  }
  return data


def HostToolsDirectToNacl(host, options):
  def H(component_name):
    return FlavoredName(component_name, host, options)

  tools = {}

  if TripleIsWindows(host):
    redirector_table = ''
    for tool, args in TOOL_X64_I686_REDIRECTS:
      redirector_table += '  {L"/bin/i686-nacl-%s.exe",' % tool + \
                          '   L"/bin/x86_64-nacl-%s.exe",' % tool + \
                          ' L"%s"},\n' % args

    cc, cxx, _, _ = CompilersForHost(host)
    tools.update({
        'redirector': {
            'type': 'build',
            'inputs': { 'source_directory': REDIRECTOR_WIN32_SRC },
            'commands': [
                command.WriteData(redirector_table,
                                  'redirector_table_pnacl.txt'),
                command.Command([cc, '-O3', '-std=c99', '-I.', '-o',
                                 os.path.join('%(output)s', 'redirector.exe'),
                                 '-I', os.path.dirname(NACL_DIR),
                                 '-DREDIRECT_DATA="redirector_table_pnacl.txt"',
                                 os.path.join(REDIRECTOR_WIN32_SRC,
                                              'redirector.c')],
                                path_dirs=GomaPathDirs(host, options)),
            ],
        },
    })

    redirect_deps = ['redirector']
    redirect_inputs = {}
    redirect_cmds = [
        command.Command([
            'ln', '-f',
            command.path.join('%(redirector)s', 'redirector.exe'),
            command.path.join('%(output)s', 'bin', 'i686-nacl-%s.exe' % tool)])
        for tool, args in TOOL_X64_I686_REDIRECTS]
  else:
    redirect_deps = []
    redirect_inputs = { 'redirector_script': REDIRECTOR_SCRIPT }
    redirect_cmds = [
        command.Command([
            '%(abs_redirector_script)s',
            command.path.join('%(output)s', 'bin', 'i686-nacl-' + tool),
            'x86_64-nacl-' + tool,
            args])
        for tool, args in TOOL_X64_I686_REDIRECTS]

  binutils_flags, binutils_inputs, binutils_deps = ConfigureBinutilsCommon(
      host, options, False)
  redirect_inputs.update(binutils_inputs)
  install_step = 'install-strip'
  if host == 'le32-nacl':
    install_step = 'install'
  tools.update({
      H('binutils_x86'): {
          'type': 'build',
          'dependencies': ['binutils_x86_src'] + redirect_deps + binutils_deps,
          'inputs': redirect_inputs,
          'commands': [
              command.SkipForIncrementalCommand(
                  ['sh', '%(binutils_x86_src)s/configure'] + binutils_flags +
                  ['--target=x86_64-nacl',
                   # TODO(mcgrathr): Enable gold if we rebase to 2.25.
                   # The 2.24 gold sources are not compatible with the libc++
                   # version we use to build.
                   '--disable-gold',
                   '--enable-targets=x86_64-nacl,i686-nacl',
                   '--disable-werror'],
                  path_dirs=GomaPathDirs(host, options)),
              command.Command(MakeCommand(host, options),
                              path_dirs=GomaPathDirs(host, options)),
              command.Command(MAKE_DESTDIR_CMD + [install_step])] +
              # Remove the share dir from this binutils build and leave the one
              # from the newer version used for bitcode linking. Always remove
              # the lib dirs, which have unneeded host libs.
              [command.RemoveDirectory(os.path.join('%(output)s', dir))
               for dir in ('lib', 'lib32', 'lib64', 'share')] +
              # Create the set of directories for target libs and includes, for
              # experimentation before we actually build them.
              # Libc includes (libs dir is created by binutils)
              # TODO(dschuff): remove these when they are populated by target
              # library packages.
              [command.Mkdir(command.path.join(
                  '%(output)s', 'x86_64-nacl', 'include'), parents=True),
               command.Mkdir(command.path.join(
                   '%(output)s', 'x86_64-nacl', 'lib32')),
               command.Command(['ln', '-s', command.path.join('..','lib32'),
                               command.path.join(
                                   '%(output)s', 'x86_64-nacl', 'lib', '32')]),
               command.Command(['ln', '-s', 'lib',
                               command.path.join(
                                   '%(output)s', 'x86_64-nacl', 'lib64')])] +
              # Create links for i686-flavored names of the tools. For now we
              # don't use the redirector scripts that pass different arguments
              # because the compiler driver doesn't need them.
              [command.Command([
                  'ln', '-f',
                  command.path.join('%(output)s', 'bin', 'x86_64-nacl-' + tool),
                  command.path.join('%(output)s', 'bin', 'i686-nacl-' + tool)])
               for tool in ['addr2line', 'ar', 'nm', 'objcopy', 'objdump',
                            'ranlib', 'readelf', 'size', 'strings', 'strip']] +
              redirect_cmds
      }
  })
  return tools


def ParseComponentRevisionsFile(filename):
  ''' Parse a simple-format deps file, with fields of the form:
key=value
Keys should match the keys in GIT_REPOS above, which match the previous
directory names used by gclient (with the exception that '_' in the file is
replaced by '-' in the returned key name).
Values are the git hashes for each repo.
Empty lines or lines beginning with '#' are ignored.
This function returns a dictionary mapping the keys found in the file to their
values.
'''
  with open(filename) as f:
    deps = {}
    for line in f:
      stripped = line.strip()
      if stripped.startswith('#') or len(stripped) == 0:
        continue
      tokens = stripped.split('=')
      if len(tokens) != 2:
        raise Exception('Malformed component revisions file: ' + filename)
      deps[tokens[0].replace('_', '-')] = tokens[1]
  return deps


def InstallMinGWHostCompiler():
  """Install the MinGW host compiler used to build the host tools on Windows.

  We could use an ordinary source rule for this, but that would require hashing
  hundreds of MB of toolchain files on every build. Instead, check for the
  presence of the specially-named file <version>.installed in the install
  directory. If it is absent, check for the presence of the zip file
  <version>.zip. If it is absent, attempt to download it from Google Storage.
  Then extract the zip file and create the install file.
  """
  if not os.path.isfile(os.path.join(MINGW_PATH, MINGW_VERSION + '.installed')):
    downloader = pynacl.gsd_storage.GSDStorage([], ['nativeclient-mingw'])
    zipfilename = MINGW_VERSION + '.zip'
    zipfilepath = os.path.join(NACL_DIR, zipfilename)
    # If the zip file is not present, try to download it from Google Storage.
    # If that fails, bail out.
    if (not os.path.isfile(zipfilepath) and
        not downloader.GetSecureFile(zipfilename, zipfilepath)):
        print >>sys.stderr, 'Failed to install MinGW tools:'
        print >>sys.stderr, 'could not find or download', zipfilename
        sys.exit(1)
    logging.info('Extracting %s' % zipfilename)
    zf = zipfile.ZipFile(zipfilepath)
    if os.path.exists(MINGW_PATH):
      shutil.rmtree(MINGW_PATH)
    zf.extractall(NACL_DIR)
    with open(os.path.join(MINGW_PATH, MINGW_VERSION + '.installed'), 'w') as _:
      pass
  os.environ['MINGW'] = MINGW_PATH


def GetUploadPackageTargets():
  """Package Targets describes all the archived package targets.

  This build can be built among many build bots, but eventually all things
  will be combined together. This package target dictionary describes the final
  output of the entire build.

  For the pnacl toolchain build we want 2 versions of the toolchain:
    1. pnacl_newlib_raw - The toolchain without core_sdk headers/libraries.
    2. pnacl_newlib - The toolchain with all the core_sdk headers/libraries.
  """
  package_targets = {}

  common_raw_packages = ['metadata']
  common_complete_packages = []

  # Target translator libraries
  for arch in TRANSLATOR_ARCHES:
    legal_arch = pynacl.gsd_storage.LegalizeName(arch)
    common_raw_packages.append('libs_support_translator_%s' % legal_arch)
    if not 'nonsfi' in arch:
      common_raw_packages.append('libgcc_eh_%s' % legal_arch)

  # Target libraries
  for bias in BITCODE_BIASES:
    legal_bias = pynacl.gsd_storage.LegalizeName(bias)
    common_raw_packages.append('newlib_%s' % legal_bias)
    common_raw_packages.append('libcxx_%s' % legal_bias)
    common_raw_packages.append('libs_support_%s' % legal_bias)
    common_raw_packages.append('compiler_rt_bc_%s' % legal_bias)

  # Portable core sdk libs. For now, no biased libs.
  common_complete_packages.append('core_sdk_libs_le32')

  # Direct-to-nacl target libraries
  for arch in DIRECT_TO_NACL_ARCHES:
    common_raw_packages.append('newlib_%s' % arch)
    common_raw_packages.append('libcxx_%s' % arch)
    common_raw_packages.append('libs_support_%s' % arch)
    common_complete_packages.append('core_sdk_libs_%s' % arch)

  # Host components
  host_packages = {}
  for os_name, arch in (('win', 'x86-32'),
                        ('mac', 'x86-64'),
  # These components are all supposed to be the same regardless of which bot is
  # running, however the 32-bit linux bot is special because it builds and tests
  # packages which are never uploaded. Because the package extraction is done by
  # package_version, we still need to output the 32-bit version of the host
  # packages on that bot.
                        ('linux', pynacl.platform.GetArch3264())):
    triple = pynacl.platform.PlatformTriple(os_name, arch)
    legal_triple = pynacl.gsd_storage.LegalizeName(triple)
    host_packages.setdefault(os_name, []).extend(
        ['binutils_%s' % legal_triple,
         'binutils_x86_%s' % legal_triple,
         'llvm_%s' % legal_triple,
         'driver_%s' % legal_triple])
    if os_name != 'win':
      host_packages[os_name].append('libcxx_%s' % legal_triple)

  # Unsandboxed target IRT libraries
  for os_name in ['linux', 'mac']:
    legal_triple = pynacl.gsd_storage.LegalizeName('x86-32-' + os_name)
    host_packages[os_name].append('unsandboxed_runtime_%s' % legal_triple)
  for os_name in ['linux']:
    legal_triple = pynacl.gsd_storage.LegalizeName('arm-' + os_name)
    host_packages[os_name].append('unsandboxed_runtime_%s' % legal_triple)
  for os_name in ['linux']:
    legal_triple = pynacl.gsd_storage.LegalizeName('x86-64-' + os_name)
    host_packages[os_name].append('unsandboxed_runtime_%s' % legal_triple)

  for os_name, os_packages in host_packages.iteritems():
    package_target = '%s_x86' % pynacl.platform.GetOS(os_name)
    package_targets[package_target] = {}

    raw_packages = os_packages + common_raw_packages
    package_targets[package_target]['pnacl_newlib_raw'] = raw_packages

    complete_packages = raw_packages + common_complete_packages
    package_targets[package_target]['pnacl_newlib'] = complete_packages

  package_targets['linux_x86']['pnacl_translator'] = ['sandboxed_translators']

  return package_targets


def main():
  # This sets the logging for gclient-alike repo sync. It will be overridden
  # by the package builder based on the command-line flags.
  logging.getLogger().setLevel(logging.DEBUG)
  parser = argparse.ArgumentParser(add_help=False)
  parser.add_argument('--disable-llvm-assertions', action='store_false',
                      dest='enable_llvm_assertions', default=True)
  parser.add_argument('--cmake', action='store_true', default=False,
                      help="Use LLVM's cmake ninja build instead of autoconf")
  parser.add_argument('--gcc', action='store_true', default=False,
                      help="Use the default compiler 'cc' instead of clang")
  parser.add_argument('--native-clang-driver', action='store_true',
                      default=False, help="Use native Clang toolchain driver")
  parser.add_argument('--sanitize', choices=['address', 'thread', 'memory',
                                             'undefined'],
                      help="Use a sanitizer with LLVM's clang cmake build")
  parser.add_argument('--testsuite-sync', action='store_true', default=False,
                      help=('Sync the sources for the LLVM testsuite. '
                      'Only useful if --sync/ is also enabled'))
  parser.add_argument('--build-sbtc', action='store_true', default=False,
                      help='Build the sandboxed translators')
  parser.add_argument('--pnacl-in-pnacl', action='store_true', default=False,
                      help='Build with a PNaCl toolchain')
  parser.add_argument('--no-sdk-libs', action='store_true',
                      help='Don\'t build the core_sdk_libs scons target')
  parser.add_argument('--no-nacl-gcc', action='store_true',
                      help='Don\'t use nacl-gcc. This is normally used to '
                           'build libgcc_eh on x86. '
                           'WARNING: this results in an empty libgcc_eh '
                           'on x86 and x86_64.')
  parser.add_argument('--extra-cc-args', default=None,
                      help='Extra arguments to pass to cc/cxx')
  parser.add_argument('--extra-configure-arg', dest='extra_configure_args',
                      default=[], action='append',
                      help='Extra arguments to pass pass to host configure')
  parser.add_argument('--binutils-extra-configure',
                      default=[], action='append',
                      help='Extra binutils arguments '
                           'to pass pass to host configure')
  parser.add_argument('--host-flavor', choices=['debug', 'release'],
                      dest='host_flavor',
                      default='release',
                      help='Flavor of the build of the host binaries.')
  parser.add_argument('--afl-fuzz-dir',
                      help='Compile using afl-fuzz compiler wrappers in'
                      + ' given directory')
  parser.add_argument('--goma',
                      help='Compile using goma in given directory')
  args, leftover_args = parser.parse_known_args()
  if '-h' in leftover_args or '--help' in leftover_args:
    print 'The following arguments are specific to toolchain_build_pnacl.py:'
    parser.print_help()
    print 'The rest of the arguments are generic, in toolchain_main.py'

  if args.sanitize and not args.cmake:
    print 'Use of sanitizers requires a cmake build'
    sys.exit(1)

  if args.gcc and args.cmake:
    print 'gcc build is not supported with cmake'
    sys.exit(1)

  if args.afl_fuzz_dir and args.gcc:
    print '--afl-fuzz-dir not allowed when using gcc'
    sys.exit(1)

  if args.native_clang_driver:
    def ToolName(toolname):
      if toolname in ['llc', 'opt']:
        return 'pnacl-' + toolname
      else:
        return 'le32-nacl-' + toolname
    pnacl_targetlibs.ToolName = ToolName

  packages = {}
  upload_packages = {}

  rev = ParseComponentRevisionsFile(GIT_DEPS_FILE)
  upload_packages = GetUploadPackageTargets()
  if pynacl.platform.IsWindows():
    InstallMinGWHostCompiler()
  else:
    pnacl_commands.InstallPrebuiltCMake()

  packages.update(HostToolsSources(GetGitSyncCmdsCallback(rev)))
  if args.testsuite_sync:
    packages.update(TestsuiteSources(GetGitSyncCmdsCallback(rev)))

  if args.pnacl_in_pnacl:
    hosts = ['le32-nacl']
  else:
    hosts = [pynacl.platform.PlatformTriple()]
  if pynacl.platform.IsLinux() and BUILD_CROSS_MINGW:
    hosts.append(pynacl.platform.PlatformTriple('win', 'x86-32'))
  for host in hosts:
    packages.update(HostTools(host, args))
    if not args.pnacl_in_pnacl:
      packages.update(HostLibs(host, args))
    packages.update(HostToolsDirectToNacl(host, args))
  if not args.pnacl_in_pnacl:
    packages.update(TargetLibCompiler(pynacl.platform.PlatformTriple(), args))
  # Don't build the target libs on Windows because of pathname issues.
  # Only the linux64 bot is canonical (i.e. it will upload its packages).
  # The other bots will use a 'work' target instead of a 'build' target for
  # the target libs, so they will not be memoized, but can be used for tests.
  # TODO(dschuff): Even better would be if we could memoize non-canonical
  # build targets without doing things like mangling their names (and for e.g.
  # scons tests, skip running them if their dependencies haven't changed, like
  # build targets)
  is_canonical = pynacl.platform.IsLinux64()
  if ((pynacl.platform.IsLinux() or pynacl.platform.IsMac())
      and not args.pnacl_in_pnacl):
    packages.update(pnacl_targetlibs.TargetLibsSrc(
      GetGitSyncCmdsCallback(rev)))
    for bias in BITCODE_BIASES:
      packages.update(pnacl_targetlibs.TargetLibs(bias, is_canonical))
    for arch in DIRECT_TO_NACL_ARCHES:
      packages.update(pnacl_targetlibs.TargetLibs(arch, is_canonical))
      if not args.no_sdk_libs:
        packages.update(pnacl_targetlibs.SDKLibs(arch, is_canonical))
    for arch in TRANSLATOR_ARCHES:
      packages.update(pnacl_targetlibs.TranslatorLibs(arch, is_canonical,
        args.no_nacl_gcc))
    packages.update(Metadata(rev, is_canonical))
    packages.update(pnacl_targetlibs.SDKCompiler(
                    ['le32'] + DIRECT_TO_NACL_ARCHES))
    packages.update(pnacl_targetlibs.SDKLibs('le32', is_canonical,
        ['pnacl_native_clang_driver=1'] if args.native_clang_driver else []))
    unsandboxed_runtime_canonical = is_canonical or pynacl.platform.IsMac()
    packages.update(pnacl_targetlibs.UnsandboxedRuntime(
        'x86-32-%s' % pynacl.platform.GetOS(), unsandboxed_runtime_canonical))
    if pynacl.platform.IsLinux():
      packages.update(pnacl_targetlibs.UnsandboxedRuntime(
          'arm-%s' % pynacl.platform.GetOS(), unsandboxed_runtime_canonical))
      packages.update(pnacl_targetlibs.UnsandboxedRuntime(
          'x86-64-%s' % pynacl.platform.GetOS(), unsandboxed_runtime_canonical))

  if args.build_sbtc and not args.pnacl_in_pnacl:
    packages.update(pnacl_sandboxed_translator.SandboxedTranslators(
      SANDBOXED_TRANSLATOR_ARCHES))

  tb = toolchain_main.PackageBuilder(packages,
                                     upload_packages,
                                     leftover_args)
  return tb.Main()


if __name__ == '__main__':
  sys.exit(main())
