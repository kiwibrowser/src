#!/usr/bin/python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
cr_cronet.py - cr - like helper tool for cronet developers
"""

import argparse
import os
import sys

def run(command, extra_options=''):
  command = command + ' ' + extra_options
  print command
  return os.system(command)


def build(out_dir, test_target, extra_options=''):
  return run('ninja -C ' + out_dir + ' ' + test_target,
             extra_options)


def install(out_dir):
  cmd = 'build/android/adb_install_apk.py ' + out_dir + '/apks/{0}'
  return run(cmd.format('CronetTest.apk')) or \
    run(cmd.format('ChromiumNetTestSupport.apk'))


def test(out_dir, extra_options):
  return run(out_dir + '/bin/run_cronet_test_instrumentation_apk ' + \
             extra_options)


def unittest(out_dir, extra_options):
  return run(out_dir + '/bin/run_cronet_unittests_android ' + \
             extra_options)


def test_ios(out_dir, extra_options):
  return run(out_dir + '/iossim -c "' + extra_options + '" ' + \
             out_dir + '/cronet_test.app')


def unittest_ios(out_dir, extra_options):
  return run(out_dir + '/iossim -c "' + extra_options + '" ' + \
             out_dir + '/cronet_unittests_ios.app')


def debug(extra_options):
  return run('build/android/adb_gdb --start ' + \
             '--activity=.CronetTestActivity ' + \
             '--program-name=CronetTest ' + \
             '--package-name=org.chromium.net',
             extra_options)


def stack(out_dir):
  return run('adb logcat -d | CHROMIUM_OUTPUT_DIR=' + out_dir +
          ' third_party/android_platform/development/scripts/stack')

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('command',
                      choices=['gn',
                               'sync',
                               'build',
                               'install',
                               'proguard',
                               'test',
                               'build-test',
                               'unit',
                               'build-unit',
                               'stack',
                               'debug',
                               'build-debug'])
  parser.add_argument('-g', '--gn', action='store_true',
                      help='use gn output directory suffix')
  parser.add_argument('-d', '--out_dir', action='store',
                      help='name of the build directory')
  parser.add_argument('-i', '--iphoneos', action='store_true',
                      help='build for physical iphone')
  parser.add_argument('-x', '--x86', action='store_true',
                      help='build for Intel x86 architecture')
  parser.add_argument('-r', '--release', action='store_true',
                      help='use release configuration')
  parser.add_argument('-a', '--asan', action='store_true',
                      help='use address sanitizer')

  options, extra_options_list = parser.parse_known_args()
  print options
  print extra_options_list

  is_ios = (sys.platform == 'darwin')
  if is_ios:
    target_os = 'ios'
    test_target = 'cronet_test'
    unit_target = 'cronet_unittests_ios'
    gn_args = 'is_cronet_build=true enable_reporting=false ' \
        'use_xcode_clang=true ios_deployment_target="9.0" '
    gn_extra = '--ide=xcode'
    if options.iphoneos:
      gn_args += 'target_cpu="arm64" '
      out_dir_suffix = '-iphoneos'
    else:
      gn_args += 'target_cpu="x64" '
      out_dir_suffix = '-iphonesimulator'
      if options.asan:
        gn_args += 'is_asan=true use_xcode_clang=true '
        out_dir_suffix += '-asan'
  else:
    target_os = 'android'
    test_target = 'cronet_test_instrumentation_apk'
    unit_target = 'cronet_unittests_android'
    gn_args = 'use_errorprone_java_compiler=true enable_reporting=true '
    gn_extra = ''
    out_dir_suffix = ''
    if options.x86:
      gn_args += 'target_cpu="x86" '
      out_dir_suffix = '-x86'
    else:
      gn_args += 'arm_use_neon=false '
    if options.asan:
      # ASAN on Android requires one-time setup described here:
      # https://www.chromium.org/developers/testing/addresssanitizer
      gn_args += 'is_asan=true is_clang=true is_debug=false '
      out_dir_suffix += '-asan'

  gn_args += 'target_os="' + target_os + '" enable_websockets=false '+ \
      'disable_file_support=true disable_ftp_support=true '+ \
      'disable_brotli_filter=false ' + \
      'use_platform_icu_alternatives=true '+ \
      'is_component_build=false ' + \
      'use_crash_key_stubs=true ' + \
      'ignore_elf32_limitations=true use_partition_alloc=false ' + \
      'include_transport_security_state_preload_list=false '

  extra_options = ' '.join(extra_options_list)
  if options.gn:
    out_dir_suffix += "-gn"

  if options.release:
    out_dir = 'out/Release' + out_dir_suffix
    gn_args += 'is_debug=false is_official_build=true '
  else:
    out_dir = 'out/Debug' + out_dir_suffix

  if options.out_dir:
    out_dir = options.out_dir

  if (options.command=='gn'):
    return run ('gn gen %s --args=\'%s\' %s' % (out_dir, gn_args, gn_extra))
  if (options.command=='sync'):
    return run ('git pull --rebase && gclient sync')
  if (options.command=='build'):
    return build(out_dir, test_target, extra_options)
  if (not is_ios):
    if (options.command=='install'):
      return install(out_dir)
    if (options.command=='proguard'):
      return run ('ninja -C ' + out_dir + ' cronet_sample_proguard_apk')
    if (options.command=='test'):
      return install(out_dir) or test(out_dir, extra_options)
    if (options.command=='build-test'):
      return build(out_dir, test_target) or install(out_dir) or \
          test(out_dir, extra_options)
    if (options.command=='stack'):
      return stack(out_dir)
    if (options.command=='debug'):
      return install(out_dir) or debug(extra_options)
    if (options.command=='build-debug'):
      return build(out_dir, test_target) or install(out_dir) or \
          debug(extra_options)
    if (options.command=='unit'):
      return unittest(out_dir, extra_options)
    if (options.command=='build-unit'):
      return build(out_dir, unit_target) or unittest(out_dir, extra_options)
  else:
    if (options.command=='test'):
      return test_ios(out_dir, extra_options)
    if (options.command=='build-test'):
      return build(out_dir, test_target) or test_ios(out_dir, extra_options)
    if (options.command=='unit'):
      return unittest_ios(out_dir, extra_options)
    if (options.command=='build-unit'):
      return build(out_dir, unit_target) or unittest_ios(out_dir, extra_options)

  parser.print_help()
  return 1


if __name__ == '__main__':
  sys.exit(main())
