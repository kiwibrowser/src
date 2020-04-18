#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Enable 'with' statements in Python 2.5
from __future__ import with_statement

import os.path
import platform
import re
import shutil
import subprocess
import sys
import time

from buildbot_lib import (
    BuildContext, BuildStatus, Command, EnsureDirectoryExists, GNArch,
    ParseStandardCommandLine, RemoveDirectory, RemovePath,
    RemoveGypBuildDirectories, RemoveSconsBuildDirectories, RunBuild, SCons,
    SetupLinuxEnvironment, SetupWindowsEnvironment,
    Step, StepLink, StepText, TryToCleanContents,
    RunningOnBuildbot)


def SetupContextVars(context):
  # The branch is set to native_client on the main bots, on the trybots it's
  # set to ''.  Otherwise, we should assume a particular branch is being used.
  context['branch'] = os.environ.get('BUILDBOT_BRANCH', 'native_client')
  context['off_trunk'] = context['branch'] not in ['native_client', '']


def ValidatorTest(context, architecture, validator, warn_only=False):
  cmd = [
      sys.executable,
      'tests/abi_corpus/validator_regression_test.py',
      '--keep-going',
      '--validator', validator,
      '--arch', architecture
  ]
  if warn_only:
    cmd.append('--warn-only')
  Command(context, cmd=cmd)


def SummarizeCoverage(context):
  Command(context, [
      sys.executable,
      'tools/coverage_summary.py',
      context['platform'] + '-' + context['default_scons_platform'],
  ])


def ArchiveCoverage(context):
  gsutil = '/b/build/third_party/gsutil/gsutil'
  gsd_url = 'http://gsdview.appspot.com/nativeclient-coverage2/revs'
  variant_name = ('coverage-' + context['platform'] + '-' +
                  context['default_scons_platform'])
  coverage_path = variant_name + '/html/index.html'
  revision = os.environ.get('BUILDBOT_REVISION', 'None')
  link_url = gsd_url + '/' + revision + '/' + coverage_path
  gsd_base = 'gs://nativeclient-coverage2/revs'
  gs_path = gsd_base + '/' + revision + '/' + variant_name
  cov_dir = 'scons-out/' + variant_name + '/coverage'
  # Copy lcov file.
  Command(context, [
      sys.executable, gsutil,
      'cp', '-a', 'public-read',
      cov_dir + '/coverage.lcov',
      gs_path + '/coverage.lcov',
  ])
  # Copy html.
  Command(context, [
      sys.executable, gsutil,
      'cp', '-R', '-a', 'public-read',
      'html', gs_path,
  ], cwd=cov_dir)
  print '@@@STEP_LINK@view@%s@@@' % link_url


def CommandGclientRunhooks(context):
  if context.Windows():
    gclient = 'gclient.bat'
  else:
    gclient = 'gclient'
  print 'Running gclient runhooks...'
  Command(context, cmd=[gclient, 'runhooks', '--force'])


def DoGNBuild(status, context, force_clang=False, force_arch=None):
  if context['no_gn']:
    return False

  use_clang = force_clang or context['clang']

  # Linux builds (or cross-builds) for every target.  Mac builds for
  # x86-32 and x86-64, and can build untrusted code for others.
  if context.Windows() and context['arch'] != '64':
    # The GN scripts for MSVC barf for a target_cpu other than x86 or x64
    # even if we only try to build the untrusted code.  Windows does build
    # for both x86-32 and x86-64 targets, but the GN Windows MSVC toolchain
    # scripts only support x86-64 hosts--and the Windows build of Clang
    # only has x86-64 binaries--while NaCl's x86-32 testing bots have to be
    # actual x86-32 hosts.
    return False

  if force_arch is not None:
    arch = force_arch
  else:
    arch = context['arch']

  if context.Linux():
    # The Linux build uses a sysroot.  'gclient runhooks' installs this
    # for the default architecture, but this might be a cross-build that
    # gclient didn't know was going to be done.  The script completes
    # quickly when it's redundant with a previous run.
    with Step('update_sysroot', status):
      sysroot_arch = {'arm': 'arm',
                      '32': 'i386',
                      '64': 'amd64',
                      'mips32': 'mips'}[arch]
      Command(context, cmd=[sys.executable,
                            '../build/linux/sysroot_scripts/install-sysroot.py',
                            '--arch=' + sysroot_arch])

  out_suffix = '_' + arch
  if force_clang:
    out_suffix += '_clang'
  gn_out = '../out' + out_suffix

  def BoolFlag(cond):
    return 'true' if cond else 'false'

  gn_newlib = BoolFlag(not context['use_glibc'])
  gn_glibc = BoolFlag(context['use_glibc'])
  gn_arch_name = GNArch(arch)

  gn_gen_args = [
      # The Chromium GN definitions might default enable_nacl to false
      # in some circumstances, but various BUILD.gn files involved in
      # the standalone NaCl build assume enable_nacl==true.
      'enable_nacl=true',
      'target_cpu="%s"' % gn_arch_name,
      'is_debug=' + context['gn_is_debug'],
      'use_gcc_glibc=' + gn_glibc,
      'use_clang_newlib=' + gn_newlib,
  ]

  # is_clang is the GN default for Mac and Linux, so
  # don't override that on "non-clang" bots, but do set
  # it explicitly for an explicitly "clang" bot.
  if use_clang:
    gn_gen_args.append('is_clang=true')

  # If this is a 32-bit build but the kernel reports as 64-bit,
  # then gn will set host_cpu=x64 when we want host_cpu=x86.
  if context.Linux() and arch == '32':
    gn_gen_args.append('host_cpu="x86"')

  # Mac can build the untrusted code for machines Mac doesn't
  # support, but the GN files will get confused in a couple of ways.
  if context.Mac() and arch not in ('32', '64'):
    gn_gen_args += [
        # Subtle GN issues mean that $host_toolchain context will
        # wind up seeing current_cpu=="arm" when target_os is left
        # to default to "mac", because host_toolchain matches
        # _default_toolchain and toolchain_args() does not apply to
        # the default toolchain.  Using target_os="ios" ensures that
        # the default toolchain is something different from
        # host_toolchain and thus that toolchain's toolchain_args()
        # block will be applied and set current_cpu correctly.
        'target_os="ios"',
        # build/config/ios/ios_sdk.gni will try to find some
        # XCode magic that won't exist.  This GN flag disables
        # the problematic code.
        'ios_enable_code_signing=false',
        ]

  gn_out_trusted = gn_out
  gn_out_irt = os.path.join(gn_out, 'irt_' + gn_arch_name)

  gn_cmd = [
      'gn.bat' if context.Windows() else 'gn',
      '--dotfile=../native_client/.gn', '--root=..',
      # Note: quotes are not needed around this space-separated
      # list of args.  The shell would remove them before passing
      # them to a program, and Python bypasses the shell.  Adding
      # quotes will cause an error because GN will see unexpected
      # double quotes.
      '--args=%s' % ' '.join(gn_gen_args),
      'gen', gn_out,
  ]

  gn_ninja_cmd = ['ninja', '-C', gn_out]
  if gn_arch_name not in ('x86', 'x64') and not context.Linux():
    # On non-Linux non-x86, we can only build the untrusted code.
    gn_ninja_cmd.append('untrusted')

  with Step('gn_compile' + out_suffix, status):
    Command(context, cmd=gn_cmd)
    Command(context, cmd=gn_ninja_cmd)

  return (gn_out_trusted, gn_out_irt)

def DoGNTest(status, context, using_gn, gn_perf_prefix, gn_step_suffix):
  if not using_gn:
    return

  gn_out_trusted, gn_out_irt = using_gn

  # Non-Linux can build non-x86 untrusted code, but can't build or run
  # the trusted code and so cannot test.
  if context['arch'] not in ('32', '64') and not context.Linux():
    return

  gn_sel_ldr = os.path.join(gn_out_trusted, 'sel_ldr')
  if context.Windows():
    gn_sel_ldr += '.exe'
  gn_extra = [
      'force_sel_ldr=' + gn_sel_ldr,
      'force_irt=' + os.path.join(gn_out_irt, 'irt_core.nexe'),
      'perf_prefix=' + gn_perf_prefix,
      ]
  if context.Linux():
    gn_extra.append('force_bootstrap=' +
                    os.path.join(gn_out_trusted, 'nacl_helper_bootstrap'))
  def RunGNTests(step_suffix, extra_scons_modes, suite_suffix):
    for suite in ['small_tests', 'medium_tests', 'large_tests']:
      with Step(suite + step_suffix + gn_step_suffix, status,
                halt_on_fail=False):
        SCons(context,
              mode=context['default_scons_mode'] + extra_scons_modes,
              args=[suite + suite_suffix] + gn_extra)
  if not context['use_glibc']:
    RunGNTests('', [], '')
  RunGNTests(' under IRT', ['nacl_irt_test'], '_irt')


def BuildScript(status, context):
  inside_toolchain = context['inside_toolchain']

  # Clean out build directories.
  with Step('clobber', status):
    RemoveSconsBuildDirectories()
    RemoveGypBuildDirectories()

  with Step('cleanup_temp', status):
    # Picking out drive letter on which the build is happening so we can use
    # it for the temp directory.
    if context.Windows():
      build_drive = os.path.splitdrive(os.path.abspath(__file__))[0]
      tmp_dir = os.path.join(build_drive, os.path.sep + 'temp')
      context.SetEnv('TEMP', tmp_dir)
      context.SetEnv('TMP', tmp_dir)
    else:
      tmp_dir = '/tmp'
    print 'Making sure %s exists...' % tmp_dir
    EnsureDirectoryExists(tmp_dir)
    print 'Cleaning up the contents of %s...' % tmp_dir
    # Only delete files and directories like:
    #   */nacl_tmp/*
    # TODO(bradnelson): Drop this after a bit.
    # Also drop files and directories like these to cleanup current state:
    #   */nacl_tmp*
    #   */nacl*
    #   83C4.tmp
    #   .org.chromium.Chromium.EQrEzl
    #   tmp_platform*
    #   tmp_mmap*
    #   tmp_pwrite*
    #   tmp_syscalls*
    #   workdir*
    #   nacl_chrome_download_*
    #   browserprofile_*
    #   tmp*
    file_name_re = re.compile(
        r'[\\/\A]('
        r'tmp_nacl[\\/].+|'
        r'tmp_nacl.+|'
        r'nacl.+|'
        r'[0-9a-fA-F]+\.tmp|'
        r'\.org\.chrom\w+\.Chrom\w+\.[^\\/]+|'
        r'tmp_platform[^\\/]+|'
        r'tmp_mmap[^\\/]+|'
        r'tmp_pwrite[^\\/]+|'
        r'tmp_syscalls[^\\/]+|'
        r'workdir[^\\/]+|'
        r'nacl_chrome_download_[^\\/]+|'
        r'browserprofile_[^\\/]+|'
        r'tmp[^\\/]+'
        r')$')
    file_name_filter = lambda fn: file_name_re.search(fn) is not None

    # Clean nacl_tmp/* separately, so we get a list of leaks.
    nacl_tmp = os.path.join(tmp_dir, 'nacl_tmp')
    if os.path.exists(nacl_tmp):
      for bot in os.listdir(nacl_tmp):
        bot_path = os.path.join(nacl_tmp, bot)
        print 'Cleaning prior build temp dir: %s' % bot_path
        sys.stdout.flush()
        if os.path.isdir(bot_path):
          for d in os.listdir(bot_path):
            path = os.path.join(bot_path, d)
            print 'Removing leftover: %s' % path
            sys.stdout.flush()
            RemovePath(path)
          os.rmdir(bot_path)
        else:
          print 'Removing rogue file: %s' % bot_path
          RemovePath(bot_path)
      os.rmdir(nacl_tmp)
    # Clean /tmp so we get a list of what's accumulating.
    TryToCleanContents(tmp_dir, file_name_filter)

    # Recreate TEMP, as it may have been clobbered.
    if 'TEMP' in os.environ and not os.path.exists(os.environ['TEMP']):
      os.makedirs(os.environ['TEMP'])

    # Mac has an additional temporary directory; clean it up.
    # TODO(bradnelson): Fix Mac Chromium so that these temp files are created
    #     with open() + unlink() so that they will not get left behind.
    if context.Mac():
      subprocess.call(
          "find /var/folders -name '.org.chromium.*' -exec rm -rfv '{}' ';'",
          shell=True)
      subprocess.call(
          "find /var/folders -name '.com.google.Chrome*' -exec rm -rfv '{}' ';'",
          shell=True)

  # Skip over hooks when run inside the toolchain build because
  # package_version would overwrite the toolchain build.
  if not inside_toolchain:
    with Step('gclient_runhooks', status):
      CommandGclientRunhooks(context)

  # Always update Clang.  On Linux and Mac, it's the default for the GN build.
  # It's also used for the Linux Breakpad build. On Windows, we do a second
  # Clang GN build.
  with Step('update_clang', status):
    Command(context, cmd=[sys.executable, '../tools/clang/scripts/update.py'])

  # Make sure our GN build is working.
  using_gn = DoGNBuild(status, context)
  using_gn_clang = False
  if (context.Windows() and
      not context['clang'] and
      context['arch'] in ('32', '64')):
    # On Windows, do a second GN build with is_clang=true.
    using_gn_clang = DoGNBuild(status, context, True)

  if context.Windows() and context['arch'] == '64':
    # On Windows, do a second pair of GN builds for 32-bit.  The 32-bit
    # bots can't do GN builds at all, because the toolchains GN uses don't
    # support 32-bit hosts.  The 32-bit binaries built here cannot be
    # tested on a 64-bit host, but compile time issues can be caught.
    DoGNBuild(status, context, False, '32')
    if not context['clang']:
      DoGNBuild(status, context, True, '32')

  # Just build both bitages of validator and test for --validator mode.
  if context['validator']:
    with Step('build ragel_validator-32', status):
      SCons(context, platform='x86-32', parallel=True, args=['ncval_new'])
    with Step('build ragel_validator-64', status):
      SCons(context, platform='x86-64', parallel=True, args=['ncval_new'])

    # Check validator trie proofs on both 32 + 64 bits.
    with Step('check validator proofs', status):
      SCons(context, platform='x86-64', parallel=False, args=['dfachecktries'])

    with Step('predownload validator corpus', status):
      Command(context,
          cmd=[sys.executable,
               'tests/abi_corpus/validator_regression_test.py',
               '--download-only'])

    with Step('validator_regression_test ragel x86-32', status,
        halt_on_fail=False):
      ValidatorTest(
          context, 'x86-32',
          'scons-out/opt-linux-x86-32/staging/ncval_new')
    with Step('validator_regression_test ragel x86-64', status,
        halt_on_fail=False):
      ValidatorTest(
          context, 'x86-64',
          'scons-out/opt-linux-x86-64/staging/ncval_new')

    return

  # Run checkdeps script to vet #includes.
  with Step('checkdeps', status):
    Command(context, cmd=[sys.executable, 'tools/checkdeps/checkdeps.py'])

  # On a subset of Linux builds, build Breakpad tools for testing.
  if context['use_breakpad_tools']:
    with Step('breakpad configure', status):
      Command(context, cmd=['mkdir', '-p', 'breakpad-out'])

      # Breakpad requires C++11, so use clang and the sysroot rather than
      # hoping that the host toolchain will provide support.
      configure_args = []
      if context.Linux():
        cc = 'CC=../../third_party/llvm-build/Release+Asserts/bin/clang'
        cxx = 'CXX=../../third_party/llvm-build/Release+Asserts/bin/clang++'
        flags = ''
        if context['arch'] == '32':
          flags += ' -m32'
          sysroot_arch = 'i386'
        else:
          flags += ' -m64'
          sysroot_arch = 'amd64'
        flags += (' --sysroot=../../build/linux/debian_jessie_%s-sysroot' %
                  sysroot_arch)
        configure_args += [cc + flags, cxx + flags]
        configure_args += ['CXXFLAGS=-I../..']  # For third_party/lss
      Command(context, cwd='breakpad-out',
              cmd=['bash', '../../breakpad/configure'] + configure_args)

    with Step('breakpad make', status):
      Command(context, cmd=['make', '-j%d' % context['max_jobs'],
                            # This avoids a broken dependency on
                            # src/third_party/lss files within the breakpad
                            # source directory.  We are not putting lss
                            # there, but using the -I switch above to
                            # find the lss in ../third_party instead.
                            'includelss_HEADERS=',
                            ],
              cwd='breakpad-out')

  # The main compile step.
  with Step('scons_compile', status):
    SCons(context, parallel=True, args=[])

  if context['coverage']:
    with Step('collect_coverage', status, halt_on_fail=True):
      SCons(context, args=['coverage'])
    with Step('summarize_coverage', status, halt_on_fail=False):
      SummarizeCoverage(context)
    slave_type = os.environ.get('BUILDBOT_SLAVE_TYPE')
    if slave_type != 'Trybot' and slave_type is not None:
      with Step('archive_coverage', status, halt_on_fail=True):
        ArchiveCoverage(context)
    return

  # Android bots don't run tests for now.
  if context['android']:
    return

  ### BEGIN tests ###
  if not context['use_glibc']:
    # Bypassing the IRT with glibc is not a supported case,
    # and in fact does not work at all with the new glibc.
    with Step('small_tests', status, halt_on_fail=False):
      SCons(context, args=['small_tests'])
    with Step('medium_tests', status, halt_on_fail=False):
      SCons(context, args=['medium_tests'])
    with Step('large_tests', status, halt_on_fail=False):
      SCons(context, args=['large_tests'])

  with Step('compile IRT tests', status):
    SCons(context, parallel=True, mode=['nacl_irt_test'])

  with Step('small_tests under IRT', status, halt_on_fail=False):
    SCons(context, mode=context['default_scons_mode'] + ['nacl_irt_test'],
          args=['small_tests_irt'])
  with Step('medium_tests under IRT', status, halt_on_fail=False):
    SCons(context, mode=context['default_scons_mode'] + ['nacl_irt_test'],
          args=['medium_tests_irt'])
  with Step('large_tests under IRT', status, halt_on_fail=False):
    SCons(context, mode=context['default_scons_mode'] + ['nacl_irt_test'],
          args=['large_tests_irt'])
  ### END tests ###

  ### BEGIN GN tests ###
  DoGNTest(status, context, using_gn, 'gn_', ' (GN)')
  DoGNTest(status, context, using_gn_clang, 'gn_clang_', '(GN, Clang)')
  ### END GN tests ###


def Main():
  # TODO(ncbray) make buildbot scripts composable to support toolchain use case.
  context = BuildContext()
  status = BuildStatus(context)
  ParseStandardCommandLine(context)
  SetupContextVars(context)
  if context.Windows():
    SetupWindowsEnvironment(context)
  elif context.Linux():
    if not context['android']:
      SetupLinuxEnvironment(context)
  elif context.Mac():
    # No setup to do for Mac.
    pass
  else:
    raise Exception("Unsupported platform.")
  RunBuild(BuildScript, status)


def TimedMain():
  start_time = time.time()
  try:
    Main()
  finally:
    time_taken = time.time() - start_time
    print 'RESULT BuildbotTime: total= %.3f minutes' % (time_taken / 60)


if __name__ == '__main__':
  TimedMain()
