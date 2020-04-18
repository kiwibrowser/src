#!/usr/bin/env python
#
# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import argparse
import multiprocessing
import os
import shutil
import subprocess
import sys
import time

from subprocess import PIPE, STDOUT

def install_file(file_name, src_dir, dst_dir):
    src_file = os.path.join(src_dir, file_name)
    dst_file = os.path.join(dst_dir, file_name)

    print('Copying {} to {}...'.format(src_file, dst_file))
    if os.path.isdir(src_file):
        _install_dir(src_file, dst_file)
    elif os.path.islink(src_file):
        _install_symlink(src_file, dst_file)
    else:
        _install_file(src_file, dst_file)


def _install_dir(src_dir, dst_dir):
    parent_dir = os.path.normpath(os.path.join(dst_dir, '..'))
    if not os.path.exists(parent_dir):
        os.makedirs(parent_dir)
    shutil.copytree(src_dir, dst_dir, symlinks=True)


def _install_symlink(src_file, dst_file):
    dirname = os.path.dirname(dst_file)
    if not os.path.exists(dirname):
        os.makedirs(dirname)
    link_target = os.readlink(src_file)
    os.symlink(link_target, dst_file)


def _install_file(src_file, dst_file):
    dirname = os.path.dirname(dst_file)
    if not os.path.exists(dirname):
        os.makedirs(dirname)
    # copy2 is just copy followed by copystat (preserves file metadata).
    shutil.copy2(src_file, dst_file)

THIS_DIR = os.path.realpath(os.path.dirname(__file__))

ALL_ARCHITECTURES = (
  'arm',
  'arm64',
  'mips',
  'mips64',
  'x86',
  'x86_64',
)

# According to vk_platform.h, armeabi is not supported for Vulkan
# so remove it from the abis list.
ALL_ABIS = (
  'armeabi-v7a',
  'arm64-v8a',
  'mips',
  'mips64',
  'x86',
  'x86_64',
)

def jobs_arg():
  return '-j{}'.format(multiprocessing.cpu_count() * 2)

def arch_to_abis(arch):
  return {
    'arm': ['armeabi-v7a'],
    'arm64': ['arm64-v8a'],
    'mips': ['mips'],
    'mips64': ['mips64'],
    'x86': ['x86'],
    'x86_64': ['x86_64'],
  }[arch]

class ArgParser(argparse.ArgumentParser):
  def __init__(self):
    super(ArgParser, self).__init__()

    self.add_argument(
      '--out-dir', help='Directory to place temporary build files.',
      type=os.path.realpath, default=os.path.join(THIS_DIR, 'out'))

    self.add_argument(
      '--arch', choices=ALL_ARCHITECTURES,
      help='Architectures to build. Builds all if not present.')

    self.add_argument('--installdir', dest='installdir', required=True,
      help='Installation directory. Required.')

    # The default for --dist-dir has to be handled after parsing all
    # arguments because the default is derived from --out-dir. This is
    # handled in run().
    self.add_argument(
      '--dist-dir', help='Directory to place the packaged artifact.',
      type=os.path.realpath)


def main():
  print('THIS_DIR: %s' % THIS_DIR)
  parser = ArgParser()
  args = parser.parse_args()

  arches = ALL_ARCHITECTURES
  if args.arch is not None:
    arches = [args.arch]

  # Make paths absolute, and ensure directories exist.
  installdir = os.path.abspath(args.installdir)

  abis = []
  for arch in arches:
    abis.extend(arch_to_abis(arch))

  shaderc_path = installdir + '/shaderc/android_test'
  print('shaderc_path = %s' % shaderc_path)

  if os.path.isdir('/buildbot/android-ndk'):
    ndk_dir = '/buildbot/android-ndk'
  elif os.path.isdir(os.environ['NDK_PATH']):
    ndk_dir = os.environ['NDK_PATH'];
  else:
    print('Error: No NDK environment found')
    return

  ndk_build = os.path.join(ndk_dir, 'ndk-build')
  platforms_root = os.path.join(ndk_dir, 'platforms')
  toolchains_root = os.path.join(ndk_dir, 'toolchains')
  build_dir = THIS_DIR

  print('installdir: %s' % installdir)
  print('ndk_dir: %s' % ndk_dir)
  print('ndk_build: %s' % ndk_build)
  print('platforms_root: %s' % platforms_root)

  compiler = 'clang'
  stl = 'gnustl_static'
  obj_out = os.path.join(THIS_DIR, stl, 'obj')
  lib_out = os.path.join(THIS_DIR, 'jniLibs')

  print('obj_out: %s' % obj_out)
  print('lib_out: %s' % lib_out)

  print('Constructing shaderc build tree...')
  shaderc_root_dir = os.path.join(THIS_DIR, '../../shaderc')

  copies = [
      {
          'source_dir': os.path.join(shaderc_root_dir, 'shaderc'),
          'dest_dir': 'third_party/shaderc',
          'files': [
              'Android.mk', 'libshaderc/Android.mk',
              'libshaderc_util/Android.mk',
              'third_party/Android.mk',
              'utils/update_build_version.py',
              'CHANGES',
          ],
          'dirs': [
              'libshaderc/include', 'libshaderc/src',
              'libshaderc_util/include', 'libshaderc_util/src',
              'android_test'
          ],
      },
      {
          'source_dir': os.path.join(shaderc_root_dir, 'spirv-tools'),
          'dest_dir': 'third_party/shaderc/third_party/spirv-tools',
          'files': [
              'utils/generate_grammar_tables.py',
              'utils/generate_registry_tables.py',
              'utils/update_build_version.py',
              'CHANGES',
          ],
          'dirs': ['include', 'source'],
      },
      {
          'source_dir': os.path.join(shaderc_root_dir, 'spirv-headers'),
          'dest_dir':
              'third_party/shaderc/third_party/spirv-tools/external/spirv-headers',
          'dirs': ['include',],
          'files': [
              'include/spirv/1.0/spirv.py',
              'include/spirv/1.1/spirv.py'
          ],
      },
      {
          'source_dir': os.path.join(shaderc_root_dir, 'glslang'),
          'dest_dir': 'third_party/shaderc/third_party/glslang',
          'files': ['glslang/OSDependent/osinclude.h'],
          'dirs': [
              'SPIRV',
              'OGLCompilersDLL',
              'glslang/GenericCodeGen',
              'hlsl',
              'glslang/Include',
              'glslang/MachineIndependent',
              'glslang/OSDependent/Unix',
              'glslang/Public',
          ],
      },
  ]

  default_ignore_patterns = shutil.ignore_patterns(
      "*CMakeLists.txt",
      "*.py",
      "*test.h",
      "*test.cc")

  for properties in copies:
      source_dir = properties['source_dir']
      dest_dir = os.path.join(installdir, properties['dest_dir'])
      for d in properties['dirs']:
          src = os.path.join(source_dir, d)
          dst = os.path.join(dest_dir, d)
          print(src, " -> ", dst)
          shutil.copytree(src, dst,
                          ignore=default_ignore_patterns)
      for f in properties['files']:
          print(source_dir, ':', dest_dir, ":", f)
          # Only copy if the source file exists.  That way
          # we can update this script in anticipation of
          # source files yet-to-come.
          if os.path.exists(os.path.join(source_dir, f)):
              install_file(f, source_dir, dest_dir)
          else:
              print(source_dir, ':', dest_dir, ":", f, "SKIPPED")

  print('Constructing Vulkan validation layer source...')

  build_cmd = [
    'bash', THIS_DIR + '/android-generate.sh'
  ]
  print('Generating generated layers...')
  subprocess.check_call(build_cmd)
  print('Generation finished')

  build_cmd = [
    'bash', ndk_build, '-C', build_dir,
    jobs_arg(),
    'APP_ABI=' + ' '.join(abis),
    # Use the prebuilt platforms and toolchains.
    'NDK_PLATFORMS_ROOT=' + platforms_root,
    'NDK_TOOLCHAINS_ROOT=' + toolchains_root,
    'NDK_MODULE_PATH=' + installdir,
    'GNUSTL_PREFIX=',
    'APP_STL=' + stl,
    'NDK_TOOLCHAIN_VERSION=' + compiler,

    # Tell ndk-build where to put the results
    'NDK_OUT=' + obj_out,
    'NDK_LIBS_OUT=' + lib_out,
  ]

  print('Building Vulkan validation layers for ABIs:' +
    ' {}'.format(', '.join(abis)) + "...")
  print(' '.join(build_cmd))

  subprocess.check_call(build_cmd)

  print('Finished building Vulkan validation layers')
  out_package = os.path.join(installdir, 'vulkan_validation_layers.zip')
  os.chdir(lib_out)
  build_cmd = [
      'zip', '-9qr', out_package, "."
  ]

  print('Packaging Vulkan validation layers')
  subprocess.check_call(build_cmd)
  print('Finished Packaging Vulkan validation layers')

  for properties in copies:
      dest_dir = os.path.join(installdir, properties['dest_dir'])
      for d in properties['dirs']:
          dst = os.path.join(dest_dir, d)
          print('Remove: %s' % dst)
          shutil.rmtree(dst)

  return 0


if __name__ == '__main__':
  main()
