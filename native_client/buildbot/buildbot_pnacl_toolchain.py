#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import logging
import os
import platform
import subprocess
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import pynacl.platform
import pynacl.file_tools

import buildbot_lib
import packages

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
NACL_DIR = os.path.dirname(SCRIPT_DIR)
TOOLCHAIN_BUILD_DIR = os.path.join(NACL_DIR, 'toolchain_build')
TOOLCHAIN_BUILD_OUT_DIR = os.path.join(TOOLCHAIN_BUILD_DIR, 'out')

TEMP_PACKAGES_FILE = os.path.join(TOOLCHAIN_BUILD_OUT_DIR, 'packages.txt')

BUILD_DIR = os.path.join(NACL_DIR, 'build')
PACKAGE_VERSION_DIR = os.path.join(BUILD_DIR, 'package_version')
PACKAGE_VERSION_SCRIPT = os.path.join(PACKAGE_VERSION_DIR, 'package_version.py')

GOMA_DEFAULT_PATH = '/b/build/goma'
GOMA_PATH = os.environ.get('GOMA_DIR', GOMA_DEFAULT_PATH)
GOMA_CTL = os.path.join(GOMA_PATH, 'goma_ctl.py')

# As this is a buildbot script, we want verbose logging. Note however, that
# toolchain_build has its own log settings, controlled by its CLI flags.
logging.getLogger().setLevel(logging.DEBUG)

parser = argparse.ArgumentParser(description='PNaCl toolchain buildbot script')
group = parser.add_mutually_exclusive_group()
group.add_argument('--buildbot', action='store_true',
                 help='Buildbot mode (build and archive the toolchain)')
group.add_argument('--trybot', action='store_true',
                 help='Trybot mode (build but do not archove the toolchain)')
parser.add_argument('--tests-arch', choices=['x86-32', 'x86-64'],
                    default='x86-64',
                    help='Host architecture for tests in buildbot_pnacl.sh')
parser.add_argument('--skip-tests', action='store_true',
                    help='Skip running tests after toolchain built')
# Note: LLVM's tablegen doesn't run when built with the memory sanitizer.
# TODO(kschimp): Add thread, memory, and undefined sanitizers once bugs fixed.
parser.add_argument('--sanitize', choices=['address'
                                           #, 'thread', 'memory', 'undefined'
                                          ],
                    help='Build with corresponding sanitizer')
parser.add_argument('--no-goma', action='store_true', default=False,
                    help='Do not run with goma')
args = parser.parse_args()

host_os = buildbot_lib.GetHostPlatform()

if args.sanitize:
  if host_os != 'linux' or args.tests_arch != 'x86-64':
    raise Exception("Error: Can't run sanitize bot unless linux x86-64")

# This is a minimal context, not useful for running tests yet, but enough for
# basic Step handling.
context = buildbot_lib.BuildContext()
buildbot_lib.SetDefaultContextAttributes(context)
context['pnacl'] = True
status = buildbot_lib.BuildStatus(context)

toolchain_install_dir = os.path.join(
    NACL_DIR,
    'toolchain',
    '%s_%s' % (host_os, pynacl.platform.GetArch()),
    'pnacl_newlib')

use_goma = (buildbot_lib.RunningOnBuildbot() and not args.no_goma
            and os.path.isfile(GOMA_CTL))

# If NOCONTROL_GOMA is set, the script does not start/stop goma compiler_proxy.
control_goma = use_goma and not os.environ.get('NOCONTROL_GOMA')


def ToolchainBuildCmd(sync=False, extra_flags=[]):
  sync_flag = ['--sync'] if sync else []
  executable_args = [os.path.join('toolchain_build','toolchain_build_pnacl.py'),
                     '--verbose', '--clobber',
                     '--packages-file', TEMP_PACKAGES_FILE]

  if pynacl.platform.IsLinux64():
    executable_args.append('--build-sbtc')

  if args.sanitize:
    executable_args.append('--sanitize')
    executable_args.append(args.sanitize)
    executable_args.append('--cmake')
    executable_args.append('llvm_x86_64_linux')

  if args.buildbot:
    executable_args.append('--buildbot')
  elif args.trybot:
    executable_args.append('--trybot')

  # Enabling LLVM assertions have a higher cost on Windows, particularly in the
  # presence of threads. So disable them on windows but leave them on elsewhere
  # to get the extra error checking.
  # See https://code.google.com/p/nativeclient/issues/detail?id=3830
  if host_os == 'win':
    executable_args.append('--disable-llvm-assertions')

  if use_goma:
    executable_args.append('--goma=' + GOMA_PATH)

  return [sys.executable] + executable_args + sync_flag + extra_flags


def RunWithLog(cmd):
  logging.info('Running: ' + ' '.join(cmd))
  subprocess.check_call(cmd)
  sys.stdout.flush()


# Clean out any installed toolchain parts that were built by previous bot runs.
with buildbot_lib.Step('Clobber TC install dir', status):
  print 'Removing', toolchain_install_dir
  pynacl.file_tools.RemoveDirectoryIfPresent(toolchain_install_dir)


# Run checkdeps so that the PNaCl toolchain trybots catch mistakes that would
# cause the normal NaCl bots to fail.
with buildbot_lib.Step('checkdeps', status):
  buildbot_lib.Command(
      context,
      [sys.executable,
       os.path.join(NACL_DIR, 'tools', 'checkdeps', 'checkdeps.py')])


if host_os != 'win':
  with buildbot_lib.Step('update clang', status):
    buildbot_lib.Command(
        context,
        [sys.executable,
         os.path.join(
             NACL_DIR, '..', 'tools', 'clang', 'scripts', 'update.py')])

if control_goma:
  buildbot_lib.Command(context, cmd=[sys.executable, GOMA_CTL, 'restart'])

# toolchain_build outputs its own buildbot annotations, so don't use
# buildbot_lib.Step to run it here.

# The package_version tools don't have a way to distinguish canonical packages
# (i.e. those we want to upload) from non-canonical ones; they only know how to
# process all the archives that are present. We can't just leave out the
# the non-canonical packages entirely because they are extracted by the
# package_version tool.
# First build only the packages that will be uploaded, and upload them.
RunWithLog(ToolchainBuildCmd(sync=True, extra_flags=['--canonical-only']))

if control_goma:
  buildbot_lib.Command(context, cmd=[sys.executable, GOMA_CTL, 'stop'])

if args.skip_tests:
  sys.exit(0)

if args.buildbot or args.trybot:
  # Don't upload packages from the 32-bit linux bot to avoid racing on
  # uploading the same packages as the 64-bit linux bot
  if host_os != 'linux' or pynacl.platform.IsArch64Bit():
    packages.UploadPackages(TEMP_PACKAGES_FILE, args.trybot, args.sanitize)

# Since windows bots don't build target libraries or run tests yet, Run a basic
# sanity check that tests the host components (LLVM, binutils, gold plugin).
# Then exit, since the rest of this file is just test running.
# For now full test coverage is only achieved on the main waterfall bots.
if host_os == 'win':
  packages.ExtractPackages(TEMP_PACKAGES_FILE, overlay_packages=False)
  with buildbot_lib.Step('Test host binaries and gold plugin', status,
                         halt_on_fail=False):
    buildbot_lib.Command(
        context,
        [sys.executable,
        os.path.join('tests', 'gold_plugin', 'gold_plugin_test.py'),
        '--toolchaindir', toolchain_install_dir])
else:
  # We need to run the LLVM regression tests after the first toolchain_build
  # run, while the LLVM build directory is still there. Otherwise it gets
  # deleted on the next toolchain_build run.
  # TODO(dschuff): Fix windows regression test runner (upstream in the LLVM
  # codebase or locally in the way we build LLVM) ASAP
  with buildbot_lib.Step('LLVM Regression', status,
                         halt_on_fail=False):
    llvm_test = [sys.executable,
                 os.path.join(NACL_DIR, 'pnacl', 'scripts', 'llvm-test.py'),
                 '--llvm-regression',
                 '--verbose']
    buildbot_lib.Command(context, llvm_test)


  sys.stdout.flush()
  # Now build all the packages (including the non-canonical ones) and extract
  # them for local testing.
  RunWithLog(ToolchainBuildCmd())
  packages.ExtractPackages(TEMP_PACKAGES_FILE, overlay_packages=False)

  sys.stdout.flush()

for arch in ['x86-32', 'x86-64', 'arm']:
  with buildbot_lib.Step('driver tests ' + arch, status, halt_on_fail=False):
    buildbot_lib.Command(
      context,
      [sys.executable,
       os.path.join('pnacl', 'driver', 'tests', 'driver_tests.py'),
       '--platform=' + arch])

if host_os == 'win':
  sys.exit(status.ReturnValue())


sys.stdout.flush()

# On Linux we build all toolchain components (driven from this script), and then
# call buildbot_pnacl.sh which builds the sandboxed translator and runs tests
# for all the components.
# On Mac we build the toolchain but not the sandboxed translator, and run the
# same tests as the main waterfall bot (which also does not run the sandboxed
# translator: see https://code.google.com/p/nativeclient/issues/detail?id=3856 )
if host_os == 'mac':
  subprocess.check_call([sys.executable,
                         os.path.join(NACL_DIR, 'buildbot','buildbot_pnacl.py'),
                         'opt', '64', 'pnacl'])
else:
  # Now we run the PNaCl buildbot script. It in turn runs the PNaCl build.sh
  # script (currently only for the sandboxed translator) and runs scons tests.
  # TODO(dschuff): re-implement the test-running portion of buildbot_pnacl.sh
  # using buildbot_lib, and use them here and in the non-toolchain builder.
  buildbot_shell = os.path.join(NACL_DIR, 'buildbot', 'buildbot_pnacl.sh')

  # Generate flags for buildbot_pnacl.sh

  arch = 'x8664' if args.tests_arch == 'x86-64' else 'x8632'

  if args.buildbot:
    trybot_mode = 'false'
  else:
    trybot_mode = 'true'

  platform_arg = 'mode-buildbot-tc-' + arch + '-linux'

  command = ['bash', buildbot_shell, platform_arg,  trybot_mode]
  RunWithLog(command)
