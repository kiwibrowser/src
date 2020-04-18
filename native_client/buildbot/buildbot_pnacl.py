#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys

from buildbot_lib import (
    BuildContext, BuildStatus, Command, ParseStandardCommandLine,
    RemoveSconsBuildDirectories, RunBuild, SetupLinuxEnvironment,
    SetupWindowsEnvironment, SCons, Step )

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import pynacl.platform

def RunSconsTests(status, context):
  # Clean out build directories, unless we have built elsewhere.
  if not context['skip_build']:
    with Step('clobber scons', status):
      RemoveSconsBuildDirectories()

  # Run checkdeps script to vet #includes.
  with Step('checkdeps', status):
    Command(context, cmd=[sys.executable, 'tools/checkdeps/checkdeps.py'])

  arch = context['default_scons_platform']

  flags_subzero = ['use_sz=1']
  flags_build = ['do_not_run_tests=1']
  flags_run = []

  # This file is run 3 different ways for ARM builds. The qemu-only trybot does
  # a normal build-and-run with the emulator just like the x86 bots. The panda
  # build side runs on an x86 machines with skip_run, and then packs up the
  # result and triggers an ARM hardware tester that run with skip_build
  if arch != 'arm':
    # Unlike their arm counterparts we do not run trusted tests on x86 bots.
    # Trusted tests get plenty of coverage by other bots, e.g. nacl-gcc bots.
    # We make the assumption here that there are no "exotic tests" which
    # are trusted in nature but are somehow depedent on the untrusted TC.
    flags_build.append('skip_trusted_tests=1')
    flags_run.append('skip_trusted_tests=1')

  if context['skip_run']:
    flags_run.append('do_not_run_tests=1')
    if arch == 'arm':
      # For ARM hardware bots, force_emulator= disables use of QEMU, which
      # enables building tests which don't work under QEMU.
      flags_build.append('force_emulator=')
      flags_run.append('force_emulator=')
  if context['skip_build']:
    flags_run.extend(['naclsdk_validate=0', 'built_elsewhere=1'])

  if not context['skip_build']:
    # For ARM builders which will trigger hardware testers, run the hello world
    # test with the emulator as a basic sanity check before doing anything else.
    if arch == 'arm' and context['skip_run']:
        with Step('hello_world ' + arch, status):
          SCons(context, parallel=True, args=['run_hello_world_test'])
    with Step('build_all ' + arch, status):
      SCons(context, parallel=True, args=flags_build)
    if arch in ('arm', 'x86-32', 'x86-64'):
      with Step('build_all subzero ' + arch, status):
        SCons(context, parallel=True, args=flags_build + flags_subzero)

  smoke_tests = ['small_tests', 'medium_tests']
  # Normal pexe-mode tests
  with Step('smoke_tests ' + arch, status, halt_on_fail=False):
    SCons(context, parallel=True, args=flags_run + smoke_tests)
  # Large tests cannot be run in parallel
  with Step('large_tests ' + arch, status, halt_on_fail=False):
    SCons(context, parallel=False, args=flags_run + ['large_tests'])
  # Run small_tests, medium_tests, and large_tests with Subzero.
  # TODO(stichnot): Move this to the sandboxed translator section
  # along with the translate_fast flag once pnacl-sz.nexe is ready.
  if arch in ('arm', 'x86-32', 'x86-64'):
    # Normal pexe-mode tests
    with Step('smoke_tests subzero ' + arch, status, halt_on_fail=False):
      SCons(context, parallel=True,
            args=flags_run + flags_subzero + smoke_tests)
    # Large tests cannot be run in parallel
    with Step('large_tests subzero ' + arch, status, halt_on_fail=False):
      SCons(context, parallel=False,
            args=flags_run + flags_subzero + ['large_tests'])

  with Step('nonpexe_tests ' + arch, status, halt_on_fail=False):
    SCons(context, parallel=True,
          args=flags_run + ['pnacl_generate_pexe=0', 'nonpexe_tests'])

  irt_mode = context['default_scons_mode'] + ['nacl_irt_test']
  # Build all the tests with the IRT
  if not context['skip_build']:
    with Step('build_all_irt ' + arch, status):
      SCons(context, parallel=True, mode=irt_mode, args=flags_build)
  smoke_tests_irt = ['small_tests_irt', 'medium_tests_irt']
  # Run tests with the IRT.
  with Step('smoke_tests_irt ' + arch, status, halt_on_fail=False):
    SCons(context, parallel=True, mode=irt_mode,
          args=flags_run + smoke_tests_irt)
  with Step('large_tests_irt ' + arch, status, halt_on_fail=False):
    SCons(context, parallel=False, mode=irt_mode,
          args=flags_run + ['large_tests_irt'])

  # Run some nacl_clang tests. Eventually we will have bots that just run
  # buildbot_standard with nacl_clang and this can be split out.
  context['pnacl'] = False
  context['nacl_clang'] = True
  if not context['skip_build']:
    with Step('build_nacl_clang ' + arch, status, halt_on_fail=False):
      SCons(context, parallel=True, args=flags_build)
  with Step('smoke_tests_nacl_clang ' + arch, status, halt_on_fail=False):
    SCons(context, parallel=True,
          args=flags_run + ['small_tests', 'medium_tests'])
  with Step('large_tests_nacl_clang ' + arch, status, halt_on_fail=False):
    SCons(context, parallel=False,
          args=flags_run + ['large_tests'])
  context['pnacl'] = True
  context['nacl_clang'] = False

  # Test sandboxed translation
  # TODO(dschuff): The standalone sandboxed translator driver does not have
  # the batch script wrappers, so it can't run on Windows. Either add them to
  # the translator package or make SCons use the pnacl_newlib drivers except
  # on the ARM bots where we don't have the pnacl_newlib drivers.
  # TODO(sbc): Enable these tests for mips once we build the version of the
  # translator nexe
  if not context.Windows() and arch != 'mips32':
    flags_run_sbtc = ['use_sandboxed_translator=1']
    sbtc_tests = ['toolchain_tests_irt']
    if arch == 'arm':
      # When splitting the build from the run, translate_in_build_step forces
      # the translation to run on the run side (it usually runs on the build
      # side because that runs with more parallelism)
      if context['skip_build'] or context['skip_run']:
        flags_run_sbtc.append('translate_in_build_step=0')
      else:
        # The ARM sandboxed translator is flaky under qemu, so run a very small
        # set of tests on the qemu-only trybot.
        sbtc_tests = ['run_hello_world_test_irt']
    else:
      sbtc_tests.append('large_code')

    with Step('sandboxed_translator_tests ' + arch, status,
              halt_on_fail=False):
      SCons(context, parallel=True, mode=irt_mode,
            args=flags_run + flags_run_sbtc + sbtc_tests)
    with Step('sandboxed_translator_fast_tests ' + arch, status,
              halt_on_fail=False):
      SCons(context, parallel=True, mode=irt_mode,
            args=flags_run + flags_run_sbtc + ['translate_fast=1'] + sbtc_tests)

  # Test Non-SFI Mode.
  # The only architectures that the PNaCl toolchain supports Non-SFI
  # versions of are currently x86-32 and ARM.
  # The x86-64 toolchain bot currently also runs these tests from
  # buildbot_pnacl.sh
  if context.Linux() and (arch == 'x86-32' or arch == 'arm'):
    with Step('nonsfi_tests ' + arch, status, halt_on_fail=False):
      SCons(context, parallel=True, mode=irt_mode,
            args=flags_run +
                ['nonsfi_nacl=1',
                 'nonsfi_tests',
                 'nonsfi_tests_irt'])

    # Build with pnacl_generate_pexe=0 to allow using pnacl-clang with
    # direct-to-native mode. This allows assembly to be used in tests.
    with Step('nonsfi_tests_nopnacl_generate_pexe ' + arch,
              status, halt_on_fail=False):
      extra_args = ['nonsfi_nacl=1',
                    'pnacl_generate_pexe=0',
                    'nonsfi_tests',
                    'nonsfi_tests_irt']
      # nonsfi_tests_irt with pnacl_generate_pexe=0 does not pass on x86-32.
      # https://code.google.com/p/nativeclient/issues/detail?id=4093
      if arch == 'x86-32':
        extra_args.remove('nonsfi_tests_irt')
      SCons(context, parallel=True, mode=irt_mode,
            args=flags_run + extra_args)

    # Test nonsfi_loader linked against host's libc.
    with Step('nonsfi_tests_host_libc ' + arch, status, halt_on_fail=False):
      # Using skip_nonstable_bitcode=1 here disables the tests for
      # zero-cost C++ exception handling, which don't pass for Non-SFI
      # mode yet because we don't build libgcc_eh for Non-SFI mode.
      SCons(context, parallel=True, mode=irt_mode,
            args=flags_run +
                ['nonsfi_nacl=1', 'use_newlib_nonsfi_loader=0',
                 'nonsfi_tests', 'nonsfi_tests_irt',
                 'toolchain_tests_irt', 'skip_nonstable_bitcode=1'])

  # Test unsandboxed mode.
  if (context.Linux() or context.Mac()) and arch == 'x86-32':
    if context.Linux():
      tests = ['run_' + test + '_test_irt' for test in
               ['hello_world', 'irt_futex', 'thread', 'float',
                'malloc_realloc_calloc_free', 'dup', 'cond_timedwait',
                'getpid']]
    else:
      # TODO(mseaborn): Use the same test list as on Linux when the threading
      # tests pass for Mac.
      tests = ['run_hello_world_test_irt']
    with Step('unsandboxed_tests ' + arch, status, halt_on_fail=False):
      SCons(context, parallel=True, mode=irt_mode,
            args=flags_run + ['pnacl_unsandboxed=1'] + tests)

def Main():
  context = BuildContext()
  status = BuildStatus(context)
  ParseStandardCommandLine(context)

  if context.Linux():
    SetupLinuxEnvironment(context)
  elif context.Windows():
    SetupWindowsEnvironment(context)
  elif context.Mac():
    # No setup to do for Mac.
    pass
  else:
    raise Exception('Unsupported platform')

  # Panda bots only have 2 cores.
  if pynacl.platform.GetArch() == 'arm':
    context['max_jobs'] = 2

  RunBuild(RunSconsTests, status)

if __name__ == '__main__':
  Main()
