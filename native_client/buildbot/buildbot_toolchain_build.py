#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Main entry point for toolchain_build buildbots.

Passes its arguments to toolchain_build.py.
"""

import optparse
import os
import subprocess
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import pynacl.platform

import packages

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
NACL_DIR = os.path.dirname(SCRIPT_DIR)
BUILD_DIR = os.path.join(NACL_DIR, 'build')
TOOLCHAIN_BUILD_DIR = os.path.join(NACL_DIR, 'toolchain_build')
TOOLCHAIN_BUILD_OUT_DIR = os.path.join(TOOLCHAIN_BUILD_DIR, 'out')
TOOLCHAIN_BUILD_PACKAGES = os.path.join(TOOLCHAIN_BUILD_OUT_DIR, 'packages')

PKG_VER = os.path.join(BUILD_DIR, 'package_version', 'package_version.py')
BUILDBOT_STANDARD = os.path.join(SCRIPT_DIR, 'buildbot_standard.py')

TEMP_TOOLCHAIN_DIR = os.path.join(TOOLCHAIN_BUILD_PACKAGES, 'output')
TEMP_PACKAGES_FILE = os.path.join(TOOLCHAIN_BUILD_OUT_DIR, 'packages.txt')

TOOLCHAIN_TESTS = {
#   TOOLCHAIN_NAME:    [(MODE,  ARCH,  CLIB)]
    'nacl_arm_glibc': [('opt', 'arm', 'glibc')],
    'nacl_arm_glibc_raw': [('opt', 'arm', 'glibc')],
    }

def main(args):
  parser = optparse.OptionParser(
    usage='USAGE: %prog [options] <build_name>')
  parser.add_option(
    '--buildbot',
    default=False, action='store_true',
    help='Run as a buildbot.')
  parser.add_option(
    '--trybot',
    default=False, action='store_true',
    help='Run as a trybot.')
  parser.add_option(
    '--test_toolchain', dest='test_toolchains',
    default=[], action='append',
    help='Append toolchain to test.')
  parser.add_option(
    '--skip-toolchain-build', dest='skip_toolchain_build',
    default=False, action='store_true',
    help='Skip Toolchain Build.')

  options, arguments = parser.parse_args(args)
  if len(arguments) != 1:
    print 'Error - expected build_name arguments'
    return 1

  build_name, = arguments

  build_script = os.path.join(NACL_DIR, 'toolchain_build', build_name + '.py')
  if not os.path.isfile(build_script):
    print 'Error - Unknown build script: %s' % build_script
    return 1

  if sys.platform == 'win32':
    print '@@@BUILD_STEP install mingw@@@'
    sys.stdout.flush()
    subprocess.check_call([os.path.join(NACL_DIR, 'buildbot', 'mingw_env.bat')])

  print '@@@BUILD_STEP run_pynacl_tests.py@@@'
  sys.stdout.flush()
  subprocess.check_call([
      sys.executable, os.path.join(NACL_DIR, 'pynacl', 'run_pynacl_tests.py')])

  bot_arg = ['--bot']
  if options.buildbot:
    bot_arg.append('--buildbot')
  elif options.trybot:
    bot_arg.append('--trybot')

  # Toolchain build emits its own annotator stages.
  sys.stdout.flush()
  if not options.skip_toolchain_build:
    subprocess.check_call([sys.executable,
                           build_script] +
                           bot_arg +
                           ['--packages-file', TEMP_PACKAGES_FILE])

  if options.buildbot or options.trybot:
    packages.UploadPackages(TEMP_PACKAGES_FILE, options.trybot)

  # Run toolchain tests for built toolchains
  for toolchain_name in options.test_toolchains:
    print '@@@BUILD_STEP test toolchains (%s)@@@' % toolchain_name
    sys.stdout.flush()
    test_options = TOOLCHAIN_TESTS.get(toolchain_name, None)
    if test_options is None:
      print 'Error - unknown toolchain to test with: %s' % toolchain_name
      return 1

    # Extract the toolchain into a temporary directory.
    subprocess.check_call([sys.executable,
                           PKG_VER,
                           '--packages', toolchain_name,
                           '--tar-dir', TOOLCHAIN_BUILD_PACKAGES,
                           '--dest-dir', TEMP_TOOLCHAIN_DIR,
                           'extract',
                           '--skip-missing'])

    toolchain_dir = os.path.join(TEMP_TOOLCHAIN_DIR,
                                 '%s_%s' % (pynacl.platform.GetOS(),
                                            pynacl.platform.GetArch()),
                                 toolchain_name)

    # Use buildbot_standard to run all the scons tests for each test option.
    for mode, arch, clib in test_options:
      if toolchain_name.startswith('nacl_'):
        scons_toolchain_arg = 'nacl_%s_dir' % clib
      elif toolchain_name.startswith('pnacl_'):
        scons_toolchain_arg = 'pnacl_%s_dir' % clib
      else:
        print 'Error - Could not figure out toolchain type: %s' % toolchain_name
        return 1

      subprocess.check_call([sys.executable,
                             BUILDBOT_STANDARD,
                             '--step-suffix= [%s %s]' % (toolchain_name, mode),
                             '--scons-args', '%s=%s' % (scons_toolchain_arg,
                                                        toolchain_dir),
                             mode, arch, clib])

  return 0

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
