#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Recipes for NativeClient toolchain packages.

The real entry plumbing is in toolchain_main.py.
"""

import collections
import fnmatch
import platform
import os
import re
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import pynacl.gsd_storage
import pynacl.platform

import command
import toolchain_main


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
NACL_DIR = os.path.dirname(SCRIPT_DIR)


GCC_VERSION = '4.9.2'


# See command.GenerateGitPatches for the schema of entries in this dict.
# Additionally, each may contain a 'repo' key whose value is the name
# to use in place of the package name when calling GitUrl (below).
GIT_REVISIONS = {
    'binutils': {
        'rev': '2c49145108878e9914173cd9c3aa36ab0cede6b3',
        'upstream-branch': 'upstream/binutils-2_26-branch',
        'upstream-name': 'binutils-2.26',
        'upstream-base': 'binutils-2_26',
        },
    'gcc': {
        'rev': '336bd0bc1724efd6f8b2a4d7228e389dc1bc48da',
        'upstream-branch': 'upstream/gcc-4_9-branch',
        'upstream-name': 'gcc-' + GCC_VERSION,
         # Upstream tag gcc-<GCC_VERSION>-release:
        'upstream-base': 'c1283af40b65f1ad862cf5b27e2d9ed10b2076b6',
        },
    'glibc': {
        'rev': 'f0029f1bb31ebce6454e1704ecac1a95b75e1e16',
        'upstream-branch': 'upstream/release/2.23/master',
        'upstream-name': 'glibc-2.23',
        'upstream-base': 'glibc-2.23',
        },
    'gdb': {
        'rev': '4ad027945e8645f00b857488959fc2a3b5b16d05',
        'repo': 'binutils',
        'upstream-branch': 'upstream/gdb-7.9-branch',
        'upstream-name': 'gdb-7.9.1',
        'upstream-base': 'gdb-7.9.1-release',
        },
    }

TAR_FILES = {
    'gmp': command.path.join('gmp', 'gmp-6.0.0a.tar.bz2'),
    'mpfr': command.path.join('mpfr', 'mpfr-3.1.2.tar.bz2'),
    'mpc': command.path.join('mpc', 'mpc-1.0.2.tar.gz'),
    'isl': command.path.join('cloog', 'isl-0.12.2.tar.bz2'),
    'cloog': command.path.join('cloog', 'cloog-0.18.1.tar.gz'),
    'expat': command.path.join('expat', 'expat-2.1.0.tar.gz'),
    }

GIT_BASE_URL = 'https://chromium.googlesource.com/native_client'
GIT_PUSH_URL = 'https://chromium.googlesource.com/native_client'

ALT_GIT_BASE_URL = 'https://chromium.googlesource.com/a/native_client'

KNOWN_MIRRORS = [('http://git.chromium.org/native_client', GIT_BASE_URL)]
PUSH_MIRRORS = [('http://git.chromium.org/native_client', GIT_PUSH_URL),
                (ALT_GIT_BASE_URL, GIT_PUSH_URL),
                (GIT_BASE_URL, GIT_PUSH_URL),
                ('ssh://gerrit.chromium.org/native_client', GIT_PUSH_URL)]


def GitUrl(package, push_url=False):
  repo = GIT_REVISIONS[package].get('repo', package)
  if push_url:
    base_url = GIT_PUSH_URL
  else:
    base_url = GIT_BASE_URL

  return '%s/nacl-%s.git' % (base_url, repo)


def CollectSources():
  sources = {}

  for package in TAR_FILES:
    tar_file = TAR_FILES[package]
    if fnmatch.fnmatch(tar_file, '*.bz2'):
      extract = EXTRACT_STRIP_TBZ2
    elif fnmatch.fnmatch(tar_file, '*.gz'):
      extract = EXTRACT_STRIP_TGZ
    else:
      raise Exception('unexpected file name pattern in TAR_FILES[%r]' % package)
    sources[package] = {
        'type': 'source',
        'commands': [
            command.Command(extract + [command.path.join('%(abs_top_srcdir)s',
                                                         '..', 'third_party',
                                                         tar_file)],
                            cwd='%(output)s'),
            ],
        }

  patch_packages = []
  patch_commands = []
  for package, info in GIT_REVISIONS.iteritems():
    sources[package] = {
        'type': 'source',
        'commands': command.SyncGitRepoCmds(GitUrl(package), '%(output)s',
                                            info['rev'],
                                            git_cache='%(git_cache_dir)s',
                                            push_url=GitUrl(package, True),
                                            known_mirrors=KNOWN_MIRRORS,
                                            push_mirrors=PUSH_MIRRORS),
        }
    patch_packages.append(package)
    patch_info = {'name': package}
    patch_info.update(info)
    patch_commands.append(
        command.GenerateGitPatches('%(' + package + ')s/.git', patch_info))

  sources['patches'] = {
      'type': 'build',
      'dependencies': patch_packages,
      'commands': patch_commands,
      }

  # The gcc_libs component gets the whole GCC source tree.
  sources['gcc_libs'] = sources['gcc']

  # The gcc component omits all the source directories that are used solely
  # for building target libraries.  We don't want those included in the
  # input hash calculation so that we don't rebuild the compiler when the
  # the only things that have changed are target libraries.
  sources['gcc'] = {
        'type': 'source',
        'dependencies': ['gcc_libs'],
        'commands': [command.CopyTree('%(gcc_libs)s', '%(output)s', [
            'boehm-gc',
            'libada',
            'libatomic',
            'libffi',
            'libgcc',
            'libgfortran',
            'libgo',
            'libgomp',
            'libitm',
            'libjava',
            'libmudflap',
            'libobjc',
            'libquadmath',
            'libsanitizer',
            'libssp',
            'libstdc++-v3',
            ])]
      }

  return sources


# Canonical tuples we use for hosts.
WINDOWS_HOST_TUPLE = pynacl.platform.PlatformTriple('win', 'x86-32')
MAC_HOST_TUPLE = pynacl.platform.PlatformTriple('darwin', 'x86-64')
LINUX_X86_32_TUPLE = pynacl.platform.PlatformTriple('linux', 'x86-32')
LINUX_X86_64_TUPLE = pynacl.platform.PlatformTriple('linux', 'x86-64')

# Map of native host tuple to extra tuples that it cross-builds for.
EXTRA_HOSTS_MAP = {
    LINUX_X86_64_TUPLE: [
        WINDOWS_HOST_TUPLE,
        ],
    }

# Map of native host tuple to host tuples that are "native enough".
# For these hosts, we will do a native-style build even though it's
# not the native tuple, just passing some extra compiler flags.
NATIVE_ENOUGH_MAP = {
    LINUX_X86_64_TUPLE: {
        LINUX_X86_32_TUPLE: ['-m32'],
        },
    }

# The list of targets to build toolchains for.
TARGET_LIST = ['arm']

# List upload targets for each host we want to upload packages for.
TARGET = collections.namedtuple('TARGET', ['name', 'pkg_prefix'])
HOST_TARGET = collections.namedtuple('HOST_TARGET',
                                     ['os', 'arch', 'differ3264', 'targets'])

STANDARD_TARGETS = [TARGET('arm', '')]

UPLOAD_HOST_TARGETS = [
    HOST_TARGET('win', 'x86-32', False, STANDARD_TARGETS),
    HOST_TARGET('darwin', 'x86-64', False, STANDARD_TARGETS),
    HOST_TARGET('linux', 'x86-64', False, STANDARD_TARGETS),
    ]

# GDB is built by toolchain_build but injected into package targets built by
# other means. List out what package targets, packages, and the tar file we are
# injecting on top of here.
GDB_INJECT_HOSTS = [
  ('win', 'x86-32'),
  ('darwin', 'x86-64'),
  ('linux', 'x86-64'),
  ]

GDB_INJECT_PACKAGES = [
  ('nacl_x86_newlib', ['core_sdk.tgz', 'naclsdk.tgz']),
  ('nacl_x86_glibc', ['core_sdk.tar.bz2', 'toolchain.tar.bz2']),
  ('nacl_x86_newlib_raw', ['naclsdk.tgz']),
  ('nacl_x86_glibc_raw', ['toolchain.tar.bz2']),
  ]

# These are extra arguments to pass gcc's configure that vary by target.
TARGET_GCC_CONFIG = {
    'arm': ['--with-tune=cortex-a15'],
    }

PACKAGE_NAME = 'Native Client SDK [%(build_signature)s]'
BUG_URL = 'http://gonacl.com/reportissue'

CONFIGURE_COMMON = [
    '--with-pkgversion=' + PACKAGE_NAME,
    '--with-bugurl=' + BUG_URL,
    '--prefix=',
    '--disable-silent-rules',
    ]

TAR_XV = ['tar', '-x', '-v']
EXTRACT_STRIP_TGZ = TAR_XV + ['--gzip', '--strip-components=1', '-f']
EXTRACT_STRIP_TBZ2 = TAR_XV + ['--bzip2', '--strip-components=1', '-f']
CONFIGURE_CMD = ['sh', '%(src)s/configure']
MAKE_PARALLEL_CMD = ['make', '-j%(cores)s']
MAKE_CHECK_CMD = MAKE_PARALLEL_CMD + ['check']
MAKE_DESTDIR_CMD = ['make', 'DESTDIR=%(abs_output)s']

# This file gets installed by multiple packages' install steps, but it is
# never useful when installed in isolation.  So we remove it from the
# installation directories before packaging up.
REMOVE_INFO_DIR = command.Remove(command.path.join('%(output)s',
                                                   'share', 'info', 'dir'))

def ConfigureHostArch(host):
  configure_args = []

  is_cross = CrossCompiling(host)

  if is_cross:
    extra_cc_args = []
    configure_args.append('--host=' + host)
  else:
    extra_cc_args = NATIVE_ENOUGH_MAP.get(NATIVE_TUPLE, {}).get(host, [])
    if extra_cc_args:
      # The host we've chosen is "native enough", such as x86-32 on x86-64.
      # But it's not what config.guess will yield, so we need to supply
      # a --build switch to ensure things build correctly.
      configure_args.append('--build=' + host)

  if HostIsMac(host):
    # This fixes a failure in building GCC's insn-attrtab.c with newer Mac
    # SDK versions.
    extra_cc_args.append('-fbracket-depth=1000')

  extra_cxx_args = list(extra_cc_args)
  if fnmatch.fnmatch(host, '*-linux*'):
    # Avoid shipping binaries with a runtime dependency on
    # a particular version of the libstdc++ shared library.
    # TODO(mcgrathr): Do we want this for MinGW and/or Mac too?
    extra_cxx_args.append('-static-libstdc++')

  if extra_cc_args:
    # These are the defaults when there is no setting, but we will add
    # additional switches, so we must supply the command name too.
    if is_cross:
      cc = host + '-gcc'
    else:
      cc = 'gcc'
    configure_args.append('CC=' + ' '.join([cc] + extra_cc_args))

  if extra_cxx_args:
    # These are the defaults when there is no setting, but we will add
    # additional switches, so we must supply the command name too.
    if is_cross:
      cxx = host + '-g++'
    else:
      cxx = 'g++'
    configure_args.append('CXX=' + ' '.join([cxx] + extra_cxx_args))

  if HostIsWindows(host):
    # The i18n support brings in runtime dependencies on MinGW DLLs
    # that we don't want to have to distribute alongside our binaries.
    # So just disable it, and compiler messages will always be in US English.
    configure_args.append('--disable-nls')

  return configure_args


def ConfigureHostCommon(host):
  return CONFIGURE_COMMON + ConfigureHostArch(host) + [
      '--without-gcc-arch',
      ]


def ConfigureHostLib(host):
  return ConfigureHostCommon(host) + [
      '--disable-shared',
      ]


def ConfigureHostTool(host):
  return ConfigureHostCommon(host) + [
      '--without-zlib',
      ]


def MakeCommand(host, extra_args=[]):
  if HostIsWindows(host):
    # There appears to be nothing we can pass at top-level configure time
    # that will prevent the configure scripts from finding MinGW's libiconv
    # and using it.  We have to force this variable into the environment
    # of the sub-configure runs, which are run via make.
    make_command = MAKE_PARALLEL_CMD + ['HAVE_LIBICONV=no']
  else:
    make_command = MAKE_PARALLEL_CMD
  return make_command + extra_args


# Return the 'make check' command to run.
# When cross-compiling, don't try to run test suites.
def MakeCheckCommand(host):
  if CrossCompiling(host):
    return ['true']
  return MAKE_CHECK_CMD


def InstallDocFiles(subdir, files):
  doc_dir = command.path.join('%(output)s', 'share', 'doc', subdir)
  dirs = sorted(set([command.path.dirname(command.path.join(doc_dir, file))
                     for file in files]))
  commands = ([command.Mkdir(dir, parents=True) for dir in dirs] +
              [command.Copy(command.path.join('%(' + subdir + ')s', file),
                            command.path.join(doc_dir, file))
               for file in files])
  return commands


# The default strip behavior removes debugging and symbol table
# sections, but it leaves the .comment section.  This contains the
# compiler version string, and so it changes when the compiler changes
# even if the actual machine code it produces is completely identical.
# Hence, the target library packages will always change when the
# compiler changes unless these sections are removed.  Doing this
# requires somehow teaching the makefile rules to pass the
# --remove-section=.comment switch to TARGET-strip.  For the GCC
# target libraries, setting STRIP_FOR_TARGET is sufficient.  But
# quoting nightmares make it difficult to pass a command with a space
# in it as the STRIP_FOR_TARGET value.  So the build writes a little
# script that can be invoked with a simple name.
#
# Though the gcc target libraries' makefiles are smart enough to obey
# STRIP_FOR_TARGET for library files, the newlib makefiles just
# blindly use $(INSTALL_DATA) for both header (text) files and library
# files.  Hence it's necessary to override its INSTALL_DATA setting to
# one that will do stripping using this script, and thus the script
# must silently do nothing to non-binary files.
def ConfigureTargetPrep(arch):
  script_file = 'strip_for_target'

  config_target = arch + '-nacl'
  script_contents = """\
#!/bin/sh
mode=--strip-all
for arg; do
  case "$arg" in
  -*) ;;
  *)
    type=`file --brief --mime-type "$arg"`
    case "$type" in
      application/x-executable|application/x-sharedlib) ;;
      application/x-archive|application/x-object) mode=--strip-debug ;;
      *) exit 0 ;;
    esac
    ;;
  esac
done
exec %s-strip $mode --remove-section=.comment "$@"
""" % config_target

  return [
      command.WriteData(script_contents, script_file),
      command.Command(['chmod', '+x', script_file]),
      ]


def ConfigureTargetArgs(arch):
  config_target = arch + '-nacl'
  return [
      '--target=' + config_target,
      '--with-sysroot=/' + config_target,
      'STRIP_FOR_TARGET=%(cwd)s/strip_for_target',
      ]


def CommandsInBuild(command_lines):
  return [
      command.RemoveDirectory('build'),
      command.Mkdir('build'),
      ] + [command.Command(cmd, cwd='build')
           for cmd in command_lines]


def PopulateDeps(dep_dirs):
  commands = [command.RemoveDirectory('all_deps'),
              command.Mkdir('all_deps')]
  commands += [command.Command('cp -r "%s/"* all_deps' % dirname, shell=True)
               for dirname in dep_dirs]
  return commands


def WithDepsOptions(options, component=None):
  if component is None:
    directory = command.path.join('%(cwd)s', 'all_deps')
  else:
    directory = '%(abs_' + component + ')s'
  return ['--with-' + option + '=' + directory
          for option in options]


# Return the component name we'll use for a base component name and
# a host tuple.  The component names cannot contain dashes or other
# non-identifier characters, because the names of the files uploaded
# to Google Storage are constrained.  GNU configuration tuples contain
# dashes, which we translate to underscores.
def ForHost(component_name, host):
  return component_name + '_' + pynacl.gsd_storage.LegalizeName(host)


# These are libraries that go into building the compiler itself.
def HostGccLibs(host):
  def H(component_name):
    return ForHost(component_name, host)
  host_gcc_libs = {
      H('gmp'): {
          'type': 'build',
          'dependencies': ['gmp'],
          'commands': [
              command.Command(ConfigureCommand('gmp') +
                              ConfigureHostLib(host) + [
                                  '--with-sysroot=%(abs_output)s',
                                  '--enable-cxx',
                                  # Without this, the built library will
                                  # assume the instruction set details
                                  # available on the build machine.  With
                                  # this, it dynamically chooses what code
                                  # to use based on the details of the
                                  # actual host CPU at runtime.
                                  '--enable-fat',
                                  ]),
              command.Command(MakeCommand(host)),
              command.Command(MakeCheckCommand(host)),
              command.Command(MAKE_DESTDIR_CMD + ['install-strip']),
              ],
          },
      H('mpfr'): {
          'type': 'build',
          'dependencies': ['mpfr', H('gmp')],
          'commands': [
              command.Command(ConfigureCommand('mpfr') +
                              ConfigureHostLib(host) +
                              WithDepsOptions(['sysroot', 'gmp'], H('gmp'))),
              command.Command(MakeCommand(host)),
              command.Command(MakeCheckCommand(host)),
              command.Command(MAKE_DESTDIR_CMD + ['install-strip']),
              ],
          },
      H('mpc'): {
          'type': 'build',
          'dependencies': ['mpc', H('gmp'), H('mpfr')],
          'commands': PopulateDeps(['%(' + H('gmp') + ')s',
                                    '%(' + H('mpfr') + ')s']) + [
              command.Command(ConfigureCommand('mpc') +
                              ConfigureHostLib(host) +
                              WithDepsOptions(['sysroot', 'gmp', 'mpfr'])),
              command.Command(MakeCommand(host)),
              command.Command(MakeCheckCommand(host)),
              command.Command(MAKE_DESTDIR_CMD + ['install-strip']),
              ],
          },
      H('isl'): {
          'type': 'build',
          'dependencies': ['isl', H('gmp')],
          'commands': [
              command.Command(ConfigureCommand('isl') +
                              ConfigureHostLib(host) +
                              WithDepsOptions(['sysroot', 'gmp-prefix'],
                                              H('gmp'))),
              command.Command(MakeCommand(host)),
              command.Command(MakeCheckCommand(host)),
              command.Command(MAKE_DESTDIR_CMD + ['install-strip']),
              # The .pc files wind up containing some absolute paths
              # that make the output depend on the build directory name.
              # The dependents' configure scripts don't need them anyway.
              command.RemoveDirectory(command.path.join(
                  '%(output)s', 'lib', 'pkgconfig')),
              ],
          },
      H('cloog'): {
          'type': 'build',
          'dependencies': ['cloog', H('gmp'), H('isl')],
          'commands': PopulateDeps(['%(' + H('gmp') + ')s',
                                    '%(' + H('isl') + ')s']) + [
              command.Command(ConfigureCommand('cloog') +
                              ConfigureHostLib(host) + [
                                  '--with-bits=gmp',
                                  '--with-isl=system',
                                  ] + WithDepsOptions(['sysroot',
                                                       'gmp-prefix',
                                                       'isl-prefix'])),
              command.Command(MakeCommand(host)),
              command.Command(MakeCheckCommand(host)),
              command.Command(MAKE_DESTDIR_CMD + ['install-strip']),
              # The .pc files wind up containing some absolute paths
              # that make the output depend on the build directory name.
              # The dependents' configure scripts don't need them anyway.
              command.RemoveDirectory(command.path.join(
                  '%(output)s', 'lib', 'pkgconfig')),
              ],
          },
      H('expat'): {
          'type': 'build',
          'dependencies': ['expat'],
          'commands': [
              command.Command(ConfigureCommand('expat') +
                              ConfigureHostLib(host)),
              command.Command(MakeCommand(host)),
              command.Command(MakeCheckCommand(host)),
              command.Command(MAKE_DESTDIR_CMD + [
                  # expat does not support the install-strip target.
                  'installlib',
                  'INSTALL=%(expat)s/conftools/install-sh -c -s',
                  'INSTALL_DATA=%(expat)s/conftools/install-sh -c -m 644',
                  ]),
              ],
          },
      }
  return host_gcc_libs


HOST_GCC_LIBS_DEPS = ['gmp', 'mpfr', 'mpc', 'isl', 'cloog']

def HostGccLibsDeps(host):
  return [ForHost(package, host) for package in HOST_GCC_LIBS_DEPS]


def SDKLibs(host, target):
  def H(component_name):
    return ForHost(component_name, host)

  host_components = [H('binutils_%s' % target), H('gcc_%s' % target)]
  target_components = ['glibc_' + target, 'gcc_libs_' + target]
  components = host_components + target_components

  sdk_compiler = H('sdk_compiler_' + target)

  builds = {
    sdk_compiler: {
        'type': 'work',
        'dependencies': components,
        'commands': [command.CopyRecursive('%(' + item + ')s', '%(output)s')
                     for item in components],
    },

    'sdk_libs_' + target: {
        'type': 'build',
        'dependencies': [sdk_compiler],
        'inputs': {
            'src_untrusted': os.path.join(NACL_DIR, 'src', 'untrusted'),
            'src_include': os.path.join(NACL_DIR, 'src', 'include'),
            'scons.py': os.path.join(NACL_DIR, 'scons.py'),
            'site_scons': os.path.join(NACL_DIR, 'site_scons'),
        },
        'commands': [
          command.Command(
              [sys.executable, '%(scons.py)s',
               '--verbose', '--mode=nacl', '-j%(cores)s', 'naclsdk_validate=0',
               'platform=%s' % target,
               '--nacl_glibc',
               'nacl_glibc_dir=%(abs_' + sdk_compiler + ')s',
               'DESTINATION_ROOT=%(work_dir)s',
               'includedir=' + command.path.join('%(output)s',
                                                 target + '-nacl', 'include'),
               'libdir=' + command.path.join('%(output)s',
                                             target + '-nacl', 'lib'),
               'install'],
              cwd=NACL_DIR),
        ],
    },
  }

  return builds


def ConfigureCommand(source_component):
  return [command % {'src': '%(' + source_component + ')s'}
          for command in CONFIGURE_CMD]


# When doing a Canadian cross, we need native-hosted cross components
# to do the GCC build.
def GccDeps(host, target):
  components = ['binutils_' + target]
  if CrossCompiling(host):
    components.append('gcc_' + target)
    host = NATIVE_TUPLE
  return [ForHost(component, host) for component in components]


def GccCommand(host, target, cmd):
  components_for_path = GccDeps(host, target)
  return command.Command(
      cmd, path_dirs=[command.path.join('%(abs_' + component + ')s', 'bin')
                      for component in components_for_path])


def ConfigureGccCommand(source_component, host, target, extra_args=[]):
  return GccCommand(
      host,
      target,
      ConfigureCommand(source_component) +
      ConfigureHostTool(host) +
      ConfigureTargetArgs(target) +
      TARGET_GCC_CONFIG.get(target, []) + [
          '--with-gmp=%(abs_' + ForHost('gmp', host) + ')s',
          '--with-mpfr=%(abs_' + ForHost('mpfr', host) + ')s',
          '--with-mpc=%(abs_' + ForHost('mpc', host) + ')s',
          '--with-isl=%(abs_' + ForHost('isl', host) + ')s',
          '--with-cloog=%(abs_' + ForHost('cloog', host) + ')s',
          '--enable-cloog-backend=isl',
          '--with-linker-hash-style=gnu',
          '--enable-linker-build-id',
          '--enable-languages=c,c++,lto',
          ] + extra_args)


def HostTools(host, target):
  def H(component_name):
    return ForHost(component_name, host)

  def WindowsAlternate(if_windows, if_not_windows, if_mac=None):
    if if_mac is not None and HostIsMac(host):
      return if_mac
    elif HostIsWindows(host):
      return if_windows
    else:
      return if_not_windows

  # Return the file name with the appropriate suffix for an executable file.
  def Exe(file):
    return file + WindowsAlternate('.exe', '')

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
      commands.append(command.Mkdir(command.path.join('%(cwd)s', dir)))
      commands.append(command.WriteData(
        dummy_makefile, command.path.join('%(cwd)s', dir, 'Makefile')))
    return commands

  tools = {
      H('binutils_' + target): {
          'type': 'build',
          'dependencies': ['binutils'],
          'commands': ConfigureTargetPrep(target) + [
              command.Command(
                  ConfigureCommand('binutils') +
                  ConfigureHostTool(host) +
                  # The Mac compiler is too warning-happy for -Werror.
                  WindowsAlternate([], [], ['--disable-werror']) +
                  ConfigureTargetArgs(target) + [
                      # Ensure that all the NaCl backends get included,
                      # just for convenience of using the same tools for
                      # whatever target machine.  The upstream default
                      # includes all the 32-bit *-nacl targets when any
                      # *-nacl target is selected (via --target), but only
                      # includes 64-bit secondary targets for 64-bit hosts.
                      '--enable-targets=arm-nacl,i686-nacl,x86_64-nacl',
                      '--enable-deterministic-archives',
                      '--enable-gold',
                      ] + WindowsAlternate([], ['--enable-plugins']))
              ] + DummyDirCommands(binutils_dummy_dirs) + [
              command.Command(MakeCommand(host)),
              command.Command(MakeCheckCommand(host)),
              command.Command(MAKE_DESTDIR_CMD + ['install-strip']),
              REMOVE_INFO_DIR,
              ] + InstallDocFiles('binutils',
                                  ['COPYING3'] +
                                  [command.path.join(subdir, 'NEWS')
                                   for subdir in
                                   ['binutils', 'gas', 'ld', 'gold']]) +
              # The top-level lib* directories contain host libraries
              # that we don't want to include in the distribution.
              [command.RemoveDirectory(command.path.join('%(output)s', name))
               for name in ['lib', 'lib32', 'lib64']],
          },

      H('gcc_' + target): {
          'type': 'build',
          'dependencies': (['gcc'] + HostGccLibsDeps(host) +
                           GccDeps(host, target)),
          'commands': ConfigureTargetPrep(target) + [
              ConfigureGccCommand('gcc', host, target),
              # GCC's configure step writes configargs.h with some strings
              # including the configure command line, which get embedded
              # into the gcc driver binary.  The build only works if we use
              # absolute paths in some of the configure switches, but
              # embedding those paths makes the output differ in repeated
              # builds done in different directories, which we do not want.
              # So force the generation of that file early and then edit it
              # in place to replace the absolute paths with something that
              # never varies.  Note that the 'configure-gcc' target will
              # actually build some components before running gcc/configure.
              GccCommand(host, target,
                         MakeCommand(host, ['configure-gcc'])),
              command.Command(['sed', '-i', '-e',
                               ';'.join(['s@%%(abs_%s)s@.../%s_install@g' %
                                         (component, component)
                                         for component in
                                         HostGccLibsDeps(host)] +
                                        ['s@%(cwd)s@...@g']),
                               command.path.join('gcc', 'configargs.h')]),
              # gcc/Makefile's install rules ordinarily look at the
              # installed include directory for a limits.h to decide
              # whether the lib/gcc/.../include-fixed/limits.h header
              # should be made to expect a libc-supplied limits.h or not.
              # Since we're doing this build in a clean environment without
              # any libc installed, we need to force its hand here.
              GccCommand(host, target,
                         MakeCommand(host, [
                             'all-gcc',
                             'LIMITS_H_TEST=true',
                             'MAKEOVERRIDES=inhibit_libc=true',
                             ])),
              # gcc/Makefile's install targets populate this directory
              # only if it already exists.
              command.Mkdir(command.path.join('%(output)s',
                                              target + '-nacl', 'bin'),
                            True),
              GccCommand(host, target,
                         MAKE_DESTDIR_CMD + ['install-strip-gcc']),
              REMOVE_INFO_DIR,
              # Note we include COPYING.RUNTIME here and not with gcc_libs.
              ] + InstallDocFiles('gcc', ['COPYING3', 'COPYING.RUNTIME']),
          },

      # GDB can support all the targets in one host tool.
      H('gdb'): {
          'type': 'build',
          'dependencies': ['gdb', H('expat')],
          'commands': [
              command.Command(
                  ConfigureCommand('gdb') +
                  ConfigureHostTool(host) + [
                      '--target=x86_64-nacl',
                      '--enable-targets=arm-none-eabi-nacl',
                      '--with-expat',
                      # Windows (MinGW) is missing ncurses; we need to
                      # build one here and link it in statically for
                      # --enable-tui.  See issue nativeclient:3911.
                      '--%s-tui' % WindowsAlternate('disable', 'enable'),
                      'CPPFLAGS=-I%(abs_' + H('expat') + ')s/include',
                      'LDFLAGS=-L%(abs_' + H('expat') + ')s/lib',
                      ] +
                  # TODO(mcgrathr): Should use --with-python to ensure
                  # we have it on Linux/Mac.
                  WindowsAlternate(['--without-python'], []) +
                  # TODO(mcgrathr): The default -Werror only breaks because
                  # the OSX default compiler is an old front-end that does
                  # not understand all the GCC options.  Maybe switch to
                  # using clang (system or Chromium-supplied) on Mac.
                  (['--disable-werror'] if HostIsMac(host) else [])),
              command.Command(MakeCommand(host) + ['all-gdb']),
              command.Command(MAKE_DESTDIR_CMD + [
                  '-C', 'gdb', 'install-strip',
                  ]),
              REMOVE_INFO_DIR,
              ] + [command.Command(['ln', '-f',
                                    command.path.join('%(abs_output)s',
                                                      'bin',
                                                      Exe('x86_64-nacl-gdb')),
                                    command.path.join('%(abs_output)s',
                                                      'bin',
                                                      Exe(arch + '-nacl-gdb'))])
                   for arch in ['i686', 'arm']] + InstallDocFiles('gdb', [
                       'COPYING3',
                       command.path.join('gdb', 'NEWS'),
                       ]),
          },
      }

  # TODO(mcgrathr): The ARM cross environment does not supply a termcap
  # library, so it cannot build GDB.
  if host.startswith('arm') and CrossCompiling(host):
    del tools[H('gdb')]

  return tools

def TargetCommands(host, target, command_list,
                   host_deps=['binutils', 'gcc'],
                   target_deps=[]):
  # First we have to copy the host tools into a common directory.
  # We can't just have both directories in our PATH, because the
  # compiler looks for the assembler and linker relative to itself.
  commands = PopulateDeps(['%(' + ForHost(dep + '_' + target, host) + ')s'
                           for dep in host_deps] +
                          ['%(' + dep + '_' + target + ')s'
                           for dep in target_deps])
  bindir = command.path.join('%(cwd)s', 'all_deps', 'bin')
  commands += [command.Command(cmd, path_dirs=[bindir])
               for cmd in command_list]
  return commands


def TargetLibs(host, target):
  lib_deps = [ForHost(component + '_' + target, host)
              for component in ['binutils', 'gcc']]

  # The 'minisdk_<target>' component is a workalike subset of what the full
  # NaCl SDK provides.  The glibc build uses a handful of things from the
  # SDK (ncval, sel_ldr, etc.), and expects them relative to $NACL_SDK_ROOT
  # in the layout that the SDK uses.  We provide a small subset built here
  # using SCons (and explicit copying, below), containing only the things
  # the build actually needs.
  def SconsCommand(args):
    return command.Command([sys.executable, '%(scons.py)s',
                            '--verbose', '-j%(cores)s',
                            'DESTINATION_ROOT=%(abs_work_dir)s'] + args,
                           cwd=NACL_DIR)

  scons_target = pynacl.platform.GetArch3264(target)
  sdk_target = scons_target.replace('-', '_')
  nacl_scons_out = 'nacl-' + scons_target
  irt_scons_out = 'nacl_irt-' + scons_target
  trusted_scons_out = 'opt-linux-' + scons_target
  host_scons_out = 'opt-%s-%s' % (pynacl.platform.GetOS(),
                                  pynacl.platform.GetArch3264())

  support = {
      'minisdk_' + target: {
          'type': 'work',
          'inputs': {
              'src': os.path.join(NACL_DIR, 'src'),
              'sconstruct': os.path.join(NACL_DIR, 'SConstruct'),
              'scons.py': os.path.join(NACL_DIR, 'scons.py'),
              'site_scons': os.path.join(NACL_DIR, 'site_scons'),
              'arm_trusted': os.path.join(NACL_DIR, 'toolchain', 'linux_x86',
                                          'arm_trusted'),
              },
          'commands': [
              SconsCommand(['platform=' + scons_target,
                            'nacl_helper_bootstrap', 'sel_ldr',
                            'irt_core', 'elf_loader']),
              command.Mkdir(command.path.join('%(output)s', 'tools')),
              command.Copy(command.path.join(irt_scons_out, 'staging',
                                             'irt_core.nexe'),
                           command.path.join('%(output)s', 'tools',
                                             'irt_core_' +
                                             sdk_target + '.nexe')),
              command.Copy(command.path.join(nacl_scons_out, 'staging',
                                             'elf_loader.nexe'),
                           command.path.join('%(output)s', 'tools',
                                             'elf_loader_' +
                                             sdk_target + '.nexe')),
              ] + [command.Copy(command.path.join(trusted_scons_out,
                                                  'staging', name),
                                command.path.join('%(output)s', 'tools',
                                                  name + '_' + sdk_target),
                                permissions=True)
                   for name in ['sel_ldr', 'nacl_helper_bootstrap']] + [
                       SconsCommand(['platform=' +
                                     pynacl.platform.GetArch3264(),
                                     'ncval_new']),
                       command.Copy(command.path.join(host_scons_out, 'staging',
                                                      'ncval_new'),
                                    command.path.join('%(output)s', 'tools',
                                                      'ncval'),
                                    permissions=True),
                       command.Mkdir(command.path.join('%(output)s', 'tools',
                                                       'arm_trusted', 'lib'),
                                     parents=True),
                       ] + [command.Copy(command.path.join('%(arm_trusted)s',
                                                           *(dir + [name])),
                                         command.path.join('%(output)s',
                                                           'tools',
                                                           'arm_trusted',
                                                           'lib', name))
                            for dir, name in [
                                (['lib', 'arm-linux-gnueabihf'],
                                 'librt.so.1'),
                                (['lib', 'arm-linux-gnueabihf'],
                                 'libpthread.so.0'),
                                (['lib', 'arm-linux-gnueabihf'],
                                 'libgcc_s.so.1'),
                                (['lib', 'arm-linux-gnueabihf'],
                                 'libc.so.6'),
                                (['lib', 'arm-linux-gnueabihf'],
                                 'ld-linux-armhf.so.3'),
                                (['lib', 'arm-linux-gnueabihf'],
                                 'libm.so.6'),
                                (['usr', 'lib', 'arm-linux-gnueabihf'],
                                 'libstdc++.so.6'),
                                ]
                          ]
          },
      }

  minisdk_root = 'NACL_SDK_ROOT=%(abs_minisdk_' + target + ')s'

  glibc_configparms = """
# Avoid -lgcc_s, which we do not have yet.
override gnulib-tests := -lgcc

# Work around a compiler bug.
CFLAGS-doasin.c = -mtune=generic-armv7-a
"""

  glibc_sysroot = '%(abs_glibc_' + target + ')s'
  glibc_tooldir = '%s/%s-nacl' % (glibc_sysroot, target)

  libs = {
      # The glibc build needs a libgcc.a (and the unwind.h header).
      # This is never going to be installed, only used to build glibc.
      # So it could be a 'work' target.  But it's a 'build' target instead
      # so it can benefit from memoization when glibc has changed but
      # binutils and gcc have not.
      'bootstrap_libgcc_' + target: {
          'type': 'build',
          'dependencies': ['gcc_libs'] + lib_deps + HostGccLibsDeps(host),
          # This actually builds the compiler again and uses that compiler
          # to build libgcc.  That's by far the easiest thing to get going
          # given the interdependencies of libgcc on the gcc subdirectory,
          # and building the compiler doesn't really take all that long in
          # the grand scheme of things.  TODO(mcgrathr): If upstream ever
          # cleans up all their interdependencies better, unpack the compiler,
          # configure with --disable-gcc.
          'commands': ConfigureTargetPrep(target) + [
              ConfigureGccCommand('gcc_libs', host, target, [
                  #'--with-build-sysroot=' + glibc_tooldir,
                  '--disable-dlopen',
                  '--disable-shared',
                  # This doesn't really have anything to with newlib.
                  # It says to build a libgcc that does not refer to
                  # any header files or functions from any C library.
                  '--with-newlib',
                  ]),
              GccCommand(host, target,
                         MakeCommand(host) + [
                             #'build_tooldir=' + glibc_tooldir,
                             #'MAKEOVERRIDES=NATIVE_SYSTEM_HEADER_DIR=/include',
                             'all-target-libgcc',
                             ]),
              GccCommand(host, target,
                         MAKE_DESTDIR_CMD + ['install-strip-target-libgcc']),
              REMOVE_INFO_DIR,
              ],
          },

      'glibc_' + target: {
          'type': 'build',
          'dependencies': ['glibc', 'minisdk_' + target,
                           'bootstrap_libgcc_' + target] + lib_deps,
          'commands': (ConfigureTargetPrep(target) +
                       [command.WriteData(glibc_configparms, 'configparms')] +
                       TargetCommands(host, target, [
                           # The bootstrap libgcc.a we built above contains
                           # the unwinder code.  But the GCC driver expects
                           # the split style used for the static archives when
                           # building libgcc_s.so, which we will do later.
                           # So just provide a dummy libgcc_eh.a to be found
                           # by the static links done while building libc.
                           [target + '-nacl-ar', 'crs', 'libgcc_eh.a'],
                           ConfigureCommand('glibc') + CONFIGURE_COMMON + [
                               '--host=%s-nacl' % target,
                               '--with-headers=%(abs_top_srcdir)s/..',
                               'STRIP=%(cwd)s/strip_for_target',
                           ],
                           MakeCommand(host) + [minisdk_root],
                           # TODO(mcgrathr): Can drop check-abi when
                           # check is enabled; check includes check-abi.
                           MakeCommand(host) + [minisdk_root, 'check-abi'],
                           # TODO(mcgrathr): Enable test suite later.
                           #MakeCommand(host) + [minisdk_root, 'check'],
                           ['make', 'install', minisdk_root,
                            # glibc's install rules always use a layout
                            # appropriate for a native installation.
                            # To install it in a cross-compilation layout
                            # we have to explicitly point it at the target
                            # subdirectory.  However, documentation files
                            # should not go there.
                            'install_root=%(abs_output)s/' + target + '-nacl',
                            'inst_infodir=%(abs_output)s/share/info',
                           ],
                           ], target_deps=['bootstrap_libgcc']) +
                       InstallDocFiles('glibc', ['COPYING.LIB']) + [
                           REMOVE_INFO_DIR,
                           ]
                     ),
          },

      'gcc_libs_' + target: {
          'type': 'build',
          'dependencies': (['gcc_libs'] + lib_deps + ['glibc_' + target] +
                           HostGccLibsDeps(host)),
          # This actually builds the compiler again and uses that compiler
          # to build the target libraries.  That's by far the easiest thing
          # to get going given the interdependencies of the target
          # libraries (especially libgcc) on the gcc subdirectory, and
          # building the compiler doesn't really take all that long in the
          # grand scheme of things.
          # TODO(mcgrathr): If upstream ever cleans up all their
          # interdependencies better, unpack the compiler, configure with
          # --disable-gcc, and just build all-target.
          'commands': ConfigureTargetPrep(target) + [
              ConfigureGccCommand('gcc_libs', host, target, [
                  '--with-build-sysroot=' + glibc_tooldir,
                  ]),
              GccCommand(host, target,
                         MakeCommand(host) + [
                             'build_tooldir=' + glibc_tooldir,
                             'MAKEOVERRIDES=NATIVE_SYSTEM_HEADER_DIR=/include',
                             'all-target',
                             ]),
              GccCommand(host, target,
                         MAKE_DESTDIR_CMD + ['install-strip-target']),
              REMOVE_INFO_DIR,
              ],
          },
      }

  libs.update(support)
  return libs

# Compute it once.
NATIVE_TUPLE = pynacl.platform.PlatformTriple()


# For our purposes, "cross-compiling" means not literally that we are
# targetting a host that does not match NATIVE_TUPLE, but that we are
# targetting a host whose binaries we cannot run locally.  So x86-32
# on x86-64 does not count as cross-compiling.  See NATIVE_ENOUGH_MAP, above.
def CrossCompiling(host):
  return (host != NATIVE_TUPLE and
          host not in NATIVE_ENOUGH_MAP.get(NATIVE_TUPLE, {}))


def HostIsWindows(host):
  return host == WINDOWS_HOST_TUPLE


def HostIsMac(host):
  return host == MAC_HOST_TUPLE


# We build target libraries only on Linux for two reasons:
# 1. We only need to build them once.
# 2. Linux is the fastest to build.
# TODO(mcgrathr): In future set up some scheme whereby non-Linux
# bots can build target libraries but not archive them, only verifying
# that the results came out the same as the ones archived by the
# official builder bot.  That will serve as a test of the host tools
# on the other host platforms.
def BuildTargetLibsOn(host):
  return host == LINUX_X86_64_TUPLE


def GetPackageTargets():
  """Package Targets describes all the final package targets.

  This build can be built among many build bots, but eventually all things
  will be combined together. This package target dictionary describes the final
  output of the entire build.
  """
  package_targets = {}

  # Add in standard upload targets.
  for host_target in UPLOAD_HOST_TARGETS:
    for target in host_target.targets:
      target_arch = target.name
      package_prefix = target.pkg_prefix

      # Each package target contains non-platform specific glibc and gcc libs.
      # These packages are added inside of TargetLibs(host, target).
      glibc_package = 'glibc_%s' % target_arch
      gcc_lib_package = 'gcc_libs_%s' % target_arch
      sdk_lib_packages = ['sdk_libs_%s' % target_arch]
      shared_packages = [glibc_package, gcc_lib_package]

      # Each package target contains arm binutils and gcc.
      # These packages are added inside of HostTools(host, target).
      platform_triple = pynacl.platform.PlatformTriple(host_target.os,
                                                       host_target.arch)
      binutils_package = ForHost('binutils_%s' % target_arch, platform_triple)
      gcc_package = ForHost('gcc_%s' % target_arch, platform_triple)
      gdb_package = ForHost('gdb', platform_triple)

      # Create a list of packages for a target.
      platform_packages = [binutils_package, gcc_package, gdb_package]
      raw_packages = shared_packages + platform_packages
      all_packages = raw_packages + sdk_lib_packages

      os_name = pynacl.platform.GetOS(host_target.os)
      if host_target.differ3264:
        arch_name = pynacl.platform.GetArch3264(host_target.arch)
      else:
        arch_name = pynacl.platform.GetArch(host_target.arch)
      package_target = '%s_%s' % (os_name, arch_name)
      package_name = '%snacl_%s_glibc' % (package_prefix,
                                          pynacl.platform.GetArch(target_arch))
      raw_package_name = package_name + '_raw'

      # Toolchains by default are "raw" unless they include the Core SDK
      package_target_dict = package_targets.setdefault(package_target, {})
      package_target_dict.setdefault(raw_package_name, []).extend(raw_packages)
      package_target_dict.setdefault(package_name, []).extend(all_packages)

  # GDB is a special and shared, we will inject it into various other packages.
  for platform, arch in GDB_INJECT_HOSTS:
    platform_triple = pynacl.platform.PlatformTriple(platform, arch)
    os_name = pynacl.platform.GetOS(platform)
    arch_name = pynacl.platform.GetArch(arch)

    gdb_packages = [ForHost('gdb', platform_triple)]
    package_target = '%s_%s' % (os_name, arch_name)

    for package_name, package_archives in GDB_INJECT_PACKAGES:
      combined_packages = package_archives + gdb_packages
      package_target_dict = package_targets.setdefault(package_target, {})
      package_target_dict.setdefault(package_name, []).extend(combined_packages)

  return dict(package_targets)


def CollectPackagesForHost(host, targets):
  packages = HostGccLibs(host).copy()
  for target in targets:
    packages.update(HostTools(host, target))
    if BuildTargetLibsOn(host):
      packages.update(TargetLibs(host, target))
      packages.update(SDKLibs(host, target))
  return packages


def CollectPackages(targets):
  packages = CollectSources()

  packages.update(CollectPackagesForHost(NATIVE_TUPLE, targets))

  for host in EXTRA_HOSTS_MAP.get(NATIVE_TUPLE, []):
    packages.update(CollectPackagesForHost(host, targets))

  return packages


PACKAGES = CollectPackages(TARGET_LIST)
PACKAGE_TARGETS = GetPackageTargets()


if __name__ == '__main__':
  tb = toolchain_main.PackageBuilder(PACKAGES, PACKAGE_TARGETS, sys.argv[1:])
  # TODO(mcgrathr): The bot ought to run some native_client tests
  # using the new toolchain, like the old x86 toolchain bots do.
  sys.exit(tb.Main())
