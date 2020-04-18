#!/usr/bin/env python
#
# Copyright (C) 2016 The Android Open Source Project
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

"""Builds the glslc, spirv-as, spirv-dis, and spirv-val host executables."""

from __future__ import print_function

import os
import site
import shutil
import subprocess

site.addsitedir(os.path.join(os.path.dirname(__file__), '../lib'))

import build_support
from build_support import ArgParser


def main(args):
    host_tag = build_support.host_to_tag(args.host)
    build_host_tag = build_support.get_default_host() + "-x86"

    package_dir = args.dist_dir

    # TODO(danalbert): use ndk/sources/third_party/googletest/googletest
    # after it has been updated to a version with CMakeLists
    gtest_cmd = ('-DSHADERC_GOOGLE_TEST_DIR=' +
                 os.path.join(build_support.android_path(),
                              'external', 'googletest'))

    obj_out = os.path.join(args.out_dir, 'shader_tools/obj')
    install_dir = os.path.join(args.out_dir, 'shader_tools/install')

    package_src = '-'.join([os.path.join(args.out_dir,
                                         'shader_tools/shader-tools'),
                            host_tag])
    package_name = '-'.join(['shader-tools', host_tag])

    source_root = os.path.join(build_support.android_path(), 'external',
                               'shaderc')
    shaderc_shaderc_dir = os.path.join(source_root, 'shaderc')
    spirv_headers_dir = os.path.join(source_root, 'spirv-headers')

    cmake = os.path.join(build_support.android_path(),
                         'prebuilts', 'cmake', build_host_tag, 'bin', 'cmake')
    ctest = os.path.join(build_support.android_path(),
                         'prebuilts', 'cmake', build_host_tag, 'bin', 'ctest')
    ninja = os.path.join(build_support.android_path(),
                         'prebuilts', 'ninja', build_host_tag, 'ninja')
    file_extension = ''

    additional_args = []
    if args.host.startswith("windows"):
        # Use MinGW to cross compile.
        # Use the stock MinGW-w64 from Ubuntu.
        # The prebuilts MinGW GCC 4.8.3 appears to be broken.
        mingw_toolchain = os.path.join(source_root, 'shaderc',
                                       'cmake', 'linux-mingw-toolchain.cmake')
        # Turn off pthreads support in gtest. Otherwise I get an error in
        # gtest-port.h for use of type AutoHandle in gtest-port.h without a
        # definition.
        additional_args = ['-DCMAKE_TOOLCHAIN_FILE=' + mingw_toolchain,
                           '-Dgtest_disable_pthreads=ON']
        file_extension = '.exe'
        if args.host == "windows64":
            additional_args.extend(
                ['-DCMAKE_CXX_FLAGS=-fno-rtti -fno-exceptions'])
        else:
            additional_args.extend(
                ['-DCMAKE_CXX_FLAGS=-m32 -fno-rtti -fno-exceptions',
                 '-DCMAKE_C_FLAGS=-m32'])

    for d in [package_src, obj_out, install_dir]:
        try:
            os.makedirs(d)
        except:
            pass

    # Create the NOTICE file.
    license_files = [
        os.path.join(shaderc_shaderc_dir, 'LICENSE'),
        os.path.join(shaderc_shaderc_dir,
                     'third_party',
                     'LICENSE.spirv-tools'),
        os.path.join(shaderc_shaderc_dir,
                     'third_party',
                     'LICENSE.glslang'),
    ]
    # The SPIRV-Headers might not have landed just yet.  Use its
    # license file if it exists.
    spirv_headers_license = os.path.join(spirv_headers_dir, 'LICENSE')
    if os.path.exists(spirv_headers_license):
        license_files.append(spirv_headers_license)

    build_support.merge_license_files(os.path.join(package_src, 'NOTICE'),
                                      license_files)

    cmake_command = [cmake, '-GNinja', '-DCMAKE_MAKE_PROGRAM=' + ninja,
                     '-DCMAKE_BUILD_TYPE=Release',
                     '-DCMAKE_INSTALL_PREFIX=' + install_dir,
                     '-DSHADERC_THIRD_PARTY_ROOT_DIR=' + source_root,
                     '-DSPIRV-Headers_SOURCE_DIR=' + spirv_headers_dir,
                     gtest_cmd,
                     shaderc_shaderc_dir]

    cmake_command.extend(additional_args)

    subprocess.check_call(cmake_command, cwd=obj_out)
    subprocess.check_call([cmake, '--build', obj_out, '--', '-v'])
    subprocess.check_call([cmake, '--build', obj_out,
                           '--target', 'install/strip'])

    files_to_copy = ['glslc' + file_extension,
                     'spirv-as' + file_extension,
                     'spirv-dis' + file_extension,
                     'spirv-val' + file_extension,
                     'spirv-cfg' + file_extension,
                     'spirv-opt' + file_extension]
    scripts_to_copy = ['spirv-lesspipe.sh',]
    files_to_copy.extend(scripts_to_copy)

    # Test, except on windows.
    if (not args.host.startswith('windows')):
        subprocess.check_call([ctest, '--verbose'], cwd=obj_out)

    # Copy to install tree.
    for src in files_to_copy:
        shutil.copy2(os.path.join(install_dir, 'bin', src),
                     os.path.join(package_src, src))
    if args.host.startswith('windows'):
        for src in scripts_to_copy:
            # Convert line endings on scripts.
            # Do it in place to preserve executable permissions.
            subprocess.check_call(['unix2dos', '-o',
                                   os.path.join(package_src, src)])

    build_support.make_package(package_name, package_src, package_dir)

if __name__ == '__main__':
    build_support.run(main, ArgParser)
