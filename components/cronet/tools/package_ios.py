#!/usr/bin/python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
package_ios.py - Build and Package Release and Rebug fat libraries for iOS.
"""

import argparse
import os
import shutil
import sys

def run(command, extra_options=''):
  command = command + ' ' + ' '.join(extra_options)
  print command
  return os.system(command)


def build(out_dir, test_target, extra_options=''):
  return run('ninja -C ' + out_dir + ' ' + test_target,
             extra_options)


def lipo_libraries(out_dir, input_dirs, out_lib, input_lib):
  lipo = "lipo -create "
  for input_dir in input_dirs:
    lipo += input_dir + "/" + input_lib + " "
  lipo += '-output ' + out_dir + "/" + out_lib
  return run(lipo)


def copy_build_dir(target_dir, build_dir):
  try:
    shutil.copytree(build_dir, target_dir, ignore=shutil.ignore_patterns('*.a'))
  except OSError as e:
    print('Directory not copied. Error: %s' % e)
  return 0


def package_ios(out_dir, build_dir, build_config):
  build_dir_sim = build_dir
  build_dir_dev = build_dir +'-iphoneos'
  build_target = 'cronet_package'
  target_dir = out_dir
  return build(build_dir_sim, build_target) or \
         build(build_dir_dev, build_target) or \
         copy_build_dir(target_dir, build_dir_dev + "/cronet") or \
         lipo_libraries(target_dir, [build_dir_sim, build_dir_dev], \
                        "libcronet_" + build_config + ".a", \
                        "cronet/libcronet_standalone.a")


def package_ios_framework(out_dir, target, framework_name, extra_options=''):
  print 'Building Cronet Dynamic Framework...'

  # Use Ninja to build all possible combinations.
  build_dirs = ['Debug-iphonesimulator',
                'Debug-iphoneos',
                'Release-iphonesimulator',
                'Release-iphoneos']
  for build_dir in build_dirs:
    print 'Building ' + build_dir
    build_result = run('ninja -C out/' + build_dir + ' ' + target,
                       extra_options)
    if build_result != 0:
      return build_result

  # Package all builds in the output directory
  os.makedirs(out_dir)
  for build_dir in build_dirs:
    shutil.copytree(os.path.join('out', build_dir, framework_name),
                    os.path.join(out_dir, build_dir, framework_name))
    if 'Release' in build_dir:
      shutil.copytree(os.path.join('out', build_dir, framework_name + '.dSYM'),
                      os.path.join(out_dir, build_dir,
                                   framework_name + '.dSYM'))
  # Copy the version file
  shutil.copy2('chrome/VERSION', out_dir)
  # Copy the headers
  shutil.copytree(os.path.join(out_dir, build_dirs[0],
                               framework_name, 'Headers'),
                  os.path.join(out_dir, 'Headers'))


def package_ios_framework_using_gn(out_dir='out/Framework', extra_options=''):
  print 'Building Cronet Dynamic Framework using gn...'

  # Package all builds in the output directory
  os.makedirs(out_dir)
  build_dir = ''
  for (build_config, gn_extra_args) in [('Debug', 'is_debug=true use_xcode_clang=true'),
        ('Release', 'is_debug=false enable_stripping=true is_official_build=true')]:
    for (target_device, target_cpu, additional_cpu) in [('os', 'arm', 'arm64'),
        ('simulator', 'x86', 'x64')]:
      target_dir = '%s-iphone%s' % (build_config, target_device)
      build_dir = os.path.join("out", target_dir)
      gn_args = 'target_os="ios" enable_websockets=false ' \
                'is_cronet_build=true is_component_build=false ' \
                'use_crash_key_stubs=true ' \
                'disable_file_support=true disable_ftp_support=true ' \
                'include_transport_security_state_preload_list=false ' \
                'ios_deployment_target="9.0" ' \
                'use_platform_icu_alternatives=true ' \
                'disable_brotli_filter=false enable_dsyms=true ' \
                'target_cpu="%s" additional_target_cpus = ["%s"] %s' % \
                (target_cpu, additional_cpu, gn_extra_args)

      print 'Generating Ninja ' + gn_args
      gn_result = run('gn gen %s --args=\'%s\'' % (build_dir, gn_args))
      if gn_result != 0:
        return gn_result

      print 'Building ' + build_dir
      build_result = run('ninja -C %s cronet_package' % build_dir,
                         extra_options)
      if build_result != 0:
        return build_result

      # Copy framework.
      shutil.copytree(os.path.join(build_dir, 'Cronet.framework'),
          os.path.join(out_dir, 'Dynamic', target_dir, 'Cronet.framework'))
      # Copy symbols.
      shutil.copytree(os.path.join(build_dir, 'Cronet.dSYM'),
          os.path.join(out_dir, 'Dynamic', target_dir, 'Cronet.framework.dSYM'))
      # Copy static framework.
      shutil.copytree(os.path.join(build_dir, 'Static', 'Cronet.framework'),
          os.path.join(out_dir, 'Static', target_dir, 'Cronet.framework'))

  # Copy common files from last built package.
  package_dir = os.path.join(build_dir, 'cronet')
  shutil.copy2(os.path.join(package_dir, 'AUTHORS'), out_dir)
  shutil.copy2(os.path.join(package_dir, 'LICENSE'), out_dir)
  shutil.copy2(os.path.join(package_dir, 'VERSION'), out_dir)
  # Copy the headers.
  shutil.copytree(os.path.join(build_dir,
                               'Cronet.framework', 'Headers'),
                  os.path.join(out_dir, 'Headers'))
  print 'Cronet framework is packaged into %s' % out_dir


def main():
  description = (
    '1. To build Cronet.framework call:\n'
    'package_ios.py --framework out/Frameworks\n'
    '2. To build CrNet.framework call:\n'
    'package_ios.py --crnet out/crnet\n'
  )
  parser = argparse.ArgumentParser(description=description)

  parser.add_argument('out_dir', nargs=1, help='path to output directory')
  parser.add_argument('-g', '--gn', action='store_true',
                      help='build using gn')
  parser.add_argument('-d', '--debug', action='store_true',
                      help='use release configuration')
  parser.add_argument('-r', '--release', action='store_true',
                      help='use release configuration')
  parser.add_argument('--framework', action='store_true',
                      help='build Cronet dynamic framework')
  parser.add_argument('--crnet', action='store_true',
                      help='build CrNet dynamic framework')
  parser.add_argument('--use_full_icu', action='store_true',
                      help='use full version of ICU instead of \
                      platform ICU alternative.')

  options, extra_options_list = parser.parse_known_args()
  print options
  print extra_options_list

  out_dir = options.out_dir[0]

  # Make sure that the output directory does not exist
  if os.path.exists(out_dir):
    print >>sys.stderr, 'The output directory already exists: ' + out_dir
    return 1

  use_platform_icu_alternatives = 'use_platform_icu_alternatives=1' \
      if not options.use_full_icu else 'use_platform_icu_alternatives=0'

  gyp_defines = 'GYP_DEFINES="OS=ios enable_websockets=0 '+ \
      'disable_file_support=1 disable_ftp_support=1 '+ \
      'enable_errorprone=1 disable_brotli_filter=0 chromium_ios_signing=0 ' + \
      'target_subarch=both ' + use_platform_icu_alternatives + '"'

  if not options.gn:
    run (gyp_defines + ' gclient runhooks')

  if options.framework:
    return package_ios_framework(out_dir, 'cronet_framework',
                                 'Cronet.framework', extra_options_list)

  if options.crnet:
    return package_ios_framework(out_dir, 'crnet_framework',
                                 'CrNet.framework', extra_options_list)

  if options.gn:
    return package_ios_framework_using_gn(out_dir, extra_options_list)

  return package_ios(out_dir, "out/Release", "opt") or \
         package_ios(out_dir, "out/Debug", "dbg")


if __name__ == '__main__':
  sys.exit(main())
