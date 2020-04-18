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
#
"""Creates a toolchain installation for a given Android target.

The output of this tool is a more typical cross-compiling toolchain. It is
indended to be used with existing build systems such as autotools.
"""
import argparse
import atexit
import inspect
import logging
import platform
import os
import shutil
import stat
import sys
import tempfile
import textwrap


THIS_DIR = os.path.realpath(os.path.dirname(__file__))
NDK_DIR = os.path.realpath(os.path.join(THIS_DIR, '../..'))


def logger():
    """Return the main logger for this module."""
    return logging.getLogger(__name__)


def check_ndk_or_die():
    """Verify that our NDK installation is sane or die."""
    checks = [
        'build/core',
        'prebuilt',
        'platforms',
        'toolchains',
    ]

    for check in checks:
        check_path = os.path.join(NDK_DIR, check)
        if not os.path.exists(check_path):
            sys.exit('Failed sanity check: missing {}'.format(check_path))


def get_triple(arch):
    """Return the triple for the given architecture."""
    return {
        'arm': 'arm-linux-androideabi',
        'arm64': 'aarch64-linux-android',
        'mips': 'mipsel-linux-android',
        'mips64': 'mips64el-linux-android',
        'x86': 'i686-linux-android',
        'x86_64': 'x86_64-linux-android',
    }[arch]


def get_abis(arch):
    """Return the ABIs supported for the given architecture."""
    return {
        'arm': ['armeabi', 'armeabi-v7a'],
        'arm64': ['arm64-v8a'],
        'mips': ['mips'],
        'mips64': ['mips64'],
        'x86': ['x86'],
        'x86_64': ['x86_64'],
    }[arch]


def get_host_tag_or_die():
    """Return the host tag for this platform. Die if not supported."""
    if platform.system() == 'Linux':
        return 'linux-x86_64'
    elif platform.system() == 'Darwin':
        return 'darwin-x86_64'
    elif platform.system() == 'Windows':
        host_tag = 'windows-x86_64'
        if not os.path.exists(os.path.join(NDK_DIR, 'prebuilt', host_tag)):
            host_tag = 'windows'
        return host_tag
    sys.exit('Unsupported platform: ' + platform.system())


def get_sysroot_path_or_die(arch, api_level):
    """Return the sysroot path for our architecture and API level or die."""
    platforms_root_path = os.path.join(NDK_DIR, 'platforms')
    platform_path = os.path.join(
        platforms_root_path, 'android-{}'.format(api_level))

    if not os.path.exists(platform_path):
        valid_platforms = os.listdir(platforms_root_path)
        sys.exit('Could not find {}. Valid platforms:\n{}'.format(
            platform_path, '\n'.join(valid_platforms)))

    sysroot_path = os.path.join(platform_path, 'arch-' + arch)
    if not os.path.exists(sysroot_path):
        sys.exit('Could not find {}'.format(sysroot_path))

    return sysroot_path


def get_gcc_path_or_die(arch, host_tag):
    """Return the GCC path for our host and architecture or die."""
    toolchain = {
        'arm': 'arm-linux-androideabi',
        'arm64': 'aarch64-linux-android',
        'mips': 'mipsel-linux-android',
        'mips64': 'mips64el-linux-android',
        'x86': 'x86',
        'x86_64': 'x86_64',
    }[arch] + '-4.9'

    gcc_toolchain_path = os.path.join(
        NDK_DIR, 'toolchains', toolchain, 'prebuilt', host_tag)
    if not os.path.exists(gcc_toolchain_path):
        sys.exit('Could not find GCC/binutils: {}'.format(gcc_toolchain_path))
    return gcc_toolchain_path


def get_clang_path_or_die(host_tag):
    """Return the Clang path for our host or die."""
    clang_toolchain_path = os.path.join(
        NDK_DIR, 'toolchains/llvm/prebuilt', host_tag)
    if not os.path.exists(clang_toolchain_path):
        sys.exit('Could not find Clang: {}'.format(clang_toolchain_path))
    return clang_toolchain_path


def copy_directory_contents(src, dst):
    """Copies the contents of a directory, merging with the destination.

    shutil.copytree requires that the destination does not exist. This function
    behaves like `cp -r`. That is, it merges the source and destination
    directories if appropriate.
    """
    for root, dirs, files in os.walk(src):
        subdir = os.path.relpath(root, src)
        dst_dir = os.path.join(dst, subdir)
        if not os.path.exists(dst_dir):
            os.makedirs(dst_dir)

        # This makes sure we copy even empty directories. We don't actually
        # need it, but for now it lets us diff between our result and the
        # legacy tool.
        for d in dirs:
            d_path = os.path.join(root, d)
            if os.path.islink(d_path):
                linkto = os.readlink(d_path)
                dst_file = os.path.join(dst_dir, d)
                logger().debug('Symlinking %s to %s', dst_file, linkto)
                os.symlink(linkto, dst_file)
            else:
                new_dir = os.path.join(dst_dir, d)
                if not os.path.exists(new_dir):
                    logger().debug('Making directory %s', new_dir)
                    os.makedirs(new_dir)

        for f in files:
            src_file = os.path.join(root, f)
            if os.path.islink(src_file):
                linkto = os.readlink(src_file)
                dst_file = os.path.join(dst_dir, f)
                logger().debug('Symlinking %s to %s', dst_file, linkto)
                os.symlink(linkto, dst_file)
            else:
                logger().debug('Copying %s', src_file)
                shutil.copy2(src_file, dst_dir)


def make_clang_scripts(install_dir, triple, api, windows):
    """Creates Clang wrapper scripts.

    The Clang in standalone toolchains historically was designed to be used as
    a drop-in replacement for GCC for better compatibility with existing
    projects. Since a large number of projects are not set up for cross
    compiling (and those that are expect the GCC style), our Clang should
    already know what target it is building for.

    Create wrapper scripts that invoke Clang with `-target` and `--sysroot`
    preset.
    """
    with open(os.path.join(install_dir, 'AndroidVersion.txt')) as version_file:
        major, minor, _build = version_file.read().strip().split('.')

    version_number = major + minor

    exe = ''
    if windows:
        exe = '.exe'

    bin_dir = os.path.join(install_dir, 'bin')
    shutil.move(os.path.join(bin_dir, 'clang' + exe),
                os.path.join(bin_dir, 'clang{}'.format(version_number) + exe))
    shutil.move(os.path.join(bin_dir, 'clang++' + exe),
                os.path.join(bin_dir, 'clang{}++'.format(
                    version_number) + exe))

    arch, os_name, env = triple.split('-')
    if arch == 'arm':
        arch = 'armv7a'  # Target armv7, not armv5.

    target = '-'.join([arch, 'none', os_name, env])
    flags = '-target {} --sysroot `dirname $0`/../sysroot'.format(target)
    flags += ' -D__ANDROID_API__={}'.format(api)

    clang_path = os.path.join(install_dir, 'bin/clang')
    with open(clang_path, 'w') as clang:
        clang.write(textwrap.dedent("""\
            #!/bin/bash
            if [ "$1" != "-cc1" ]; then
                `dirname $0`/clang{version} {flags} "$@"
            else
                # target/triple already spelled out.
                `dirname $0`/clang{version} "$@"
            fi
        """.format(version=version_number, flags=flags)))

    mode = os.stat(clang_path).st_mode
    os.chmod(clang_path, mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

    clangpp_path = os.path.join(install_dir, 'bin/clang++')
    with open(clangpp_path, 'w') as clangpp:
        clangpp.write(textwrap.dedent("""\
            #!/bin/bash
            if [ "$1" != "-cc1" ]; then
                `dirname $0`/clang{version}++ {flags} "$@"
            else
                # target/triple already spelled out.
                `dirname $0`/clang{version}++ "$@"
            fi
        """.format(version=version_number, flags=flags)))

    mode = os.stat(clangpp_path).st_mode
    os.chmod(clangpp_path, mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

    shutil.copy2(os.path.join(install_dir, 'bin/clang'),
                 os.path.join(install_dir, 'bin', triple + '-clang'))
    shutil.copy2(os.path.join(install_dir, 'bin/clang++'),
                 os.path.join(install_dir, 'bin', triple + '-clang++'))

    if windows:
        flags = '-target {} --sysroot %~dp0\\..\\sysroot'.format(target)
        flags += ' -D__ANDROID_API__={}'.format(api)

        clangbat_path = os.path.join(install_dir, 'bin/clang.cmd')
        with open(clangbat_path, 'w') as clangbat:
            clangbat.write(textwrap.dedent("""\
                @echo off
                if "%1" == "-cc1" goto :L
                %~dp0\\clang{version}.exe {flags} %*
                if ERRORLEVEL 1 exit /b 1
                goto :done
                :L
                rem target/triple already spelled out.
                %~dp0\\clang{version}.exe %*
                if ERRORLEVEL 1 exit /b 1
                :done
            """.format(version=version_number, flags=flags)))

        clangbatpp_path = os.path.join(install_dir, 'bin/clang++.cmd')
        with open(clangbatpp_path, 'w') as clangbatpp:
            clangbatpp.write(textwrap.dedent("""\
                @echo off
                if "%1" == "-cc1" goto :L
                %~dp0\\clang{version}++.exe {flags} %*
                if ERRORLEVEL 1 exit /b 1
                goto :done
                :L
                rem target/triple already spelled out.
                %~dp0\\clang{version}++.exe %*
                if ERRORLEVEL 1 exit /b 1
                :done
            """.format(version=version_number, flags=flags)))

        shutil.copy2(os.path.join(install_dir, 'bin/clang.cmd'),
                     os.path.join(install_dir, 'bin', triple + '-clang.cmd'))
        shutil.copy2(os.path.join(install_dir, 'bin/clang++.cmd'),
                     os.path.join(install_dir, 'bin', triple + '-clang++.cmd'))


def copy_gnustl_abi_headers(src_dir, dst_dir, gcc_ver, triple, abi,
                            thumb=False):
    """Copy the ABI specific headers for gnustl."""
    abi_src_dir = os.path.join(
        src_dir, 'libs', abi, 'include/bits')

    # Most ABIs simply install to bits, but armeabi-v7a needs to be
    # installed to armv7-a/bits.
    bits_dst_dir = 'bits'
    if thumb:
        bits_dst_dir = os.path.join('thumb', bits_dst_dir)
    if abi == 'armeabi-v7a':
        bits_dst_dir = os.path.join('armv7-a', bits_dst_dir)
    abi_dst_dir = os.path.join(
        dst_dir, 'include/c++', gcc_ver, triple, bits_dst_dir)

    shutil.copytree(abi_src_dir, abi_dst_dir)


def get_src_libdir(src_dir, abi):
    """Gets the ABI specific lib directory for an NDK project."""
    return os.path.join(src_dir, 'libs', abi)


def get_dest_libdir(dst_dir, triple, abi):
    """Get the ABI specific library directory for the toolchain."""
    libdir_name = 'lib'
    if abi in ('mips64', 'x86_64'):
        # ARM64 isn't a real multilib target, so it's just installed to lib.
        libdir_name = 'lib64'
    dst_libdir = os.path.join(dst_dir, triple, libdir_name)
    if abi.startswith('armeabi-v7a'):
        dst_libdir = os.path.join(dst_libdir, 'armv7-a')
    return dst_libdir


def copy_gnustl_libs(src_dir, dst_dir, triple, abi, thumb=False):
    """Copy the gnustl libraries to the toolchain."""
    src_libdir = get_src_libdir(src_dir, abi)
    dst_libdir = get_dest_libdir(dst_dir, triple, abi)
    if thumb:
        dst_libdir = os.path.join(dst_libdir, 'thumb')

    logger().debug('Copying %s libs to %s', abi, dst_libdir)

    if not os.path.exists(dst_libdir):
        os.makedirs(dst_libdir)

    shutil.copy2(os.path.join(src_libdir, 'libgnustl_shared.so'), dst_libdir)
    shutil.copy2(os.path.join(src_libdir, 'libsupc++.a'), dst_libdir)

    # Copy libgnustl_static.a to libstdc++.a since that's what the world
    # expects. Can't do this reliably with libgnustl_shared.so because the
    # SONAME is wrong.
    shutil.copy2(os.path.join(src_libdir, 'libgnustl_static.a'),
                 os.path.join(dst_libdir, 'libstdc++.a'))


def copy_stlport_libs(src_dir, dst_dir, triple, abi, thumb=False):
    """Copy the stlport libraries to the toolchain."""
    src_libdir = get_src_libdir(src_dir, abi)
    dst_libdir = get_dest_libdir(dst_dir, triple, abi)
    if thumb:
        dst_libdir = os.path.join(dst_libdir, 'thumb')

    if not os.path.exists(dst_libdir):
        os.makedirs(dst_libdir)

    shutil.copy2(os.path.join(src_libdir, 'libstlport_shared.so'), dst_libdir)
    shutil.copy2(os.path.join(src_libdir, 'libstlport_static.a'),
                 os.path.join(dst_libdir, 'libstdc++.a'))


def copy_libcxx_libs(src_dir, dst_dir, include_libunwind):
    shutil.copy2(os.path.join(src_dir, 'libc++_shared.so'), dst_dir)
    shutil.copy2(os.path.join(src_dir, 'libc++_static.a'), dst_dir)
    shutil.copy2(os.path.join(src_dir, 'libandroid_support.a'), dst_dir)
    shutil.copy2(os.path.join(src_dir, 'libc++abi.a'), dst_dir)

    if include_libunwind:
        shutil.copy2(os.path.join(src_dir, 'libunwind.a'), dst_dir)

    # libc++ is different from the other STLs. It has a libc++.(a|so) that is a
    # linker script which automatically pulls in the necessary libraries. This
    # way users don't have to do `-lc++abi -lunwind -landroid_support` on their
    # own.
    #
    # As with the other STLs, we still copy this as libstdc++.a so the compiler
    # will pick it up by default.
    #
    # Unlike the other STLs, also copy libc++.so (another linker script) over
    # as libstdc++.so.  Since it's a linker script, the linker will still get
    # the right DT_NEEDED from the SONAME of the actual linked object.
    #
    # TODO(danalbert): We should add linker scripts for the other STLs too
    # since it lets the user avoid the current mess of having to always
    # manually add `-lstlport_shared` (or whichever STL).
    shutil.copy2(os.path.join(src_dir, 'libc++.a'),
                 os.path.join(dst_dir, 'libstdc++.a'))
    shutil.copy2(os.path.join(src_dir, 'libc++.so'),
                 os.path.join(dst_dir, 'libstdc++.so'))


def create_toolchain(install_path, arch, api, gcc_path, clang_path,
                     sysroot_path, stl, host_tag):
    """Create a standalone toolchain."""
    copy_directory_contents(gcc_path, install_path)
    copy_directory_contents(clang_path, install_path)
    triple = get_triple(arch)
    make_clang_scripts(
        install_path, triple, api, host_tag.startswith('windows'))

    sysroot = os.path.join(NDK_DIR, 'sysroot')
    install_sysroot = os.path.join(install_path, 'sysroot')
    shutil.copytree(sysroot, install_sysroot)

    arch_headers = os.path.join(sysroot, 'usr/include', triple)
    copy_directory_contents(
        arch_headers, os.path.join(install_sysroot, 'usr/include'))

    lib_path = os.path.join(sysroot_path, 'usr/lib')
    lib_install = os.path.join(install_sysroot, 'usr/lib')
    if os.path.exists(lib_path):
        shutil.copytree(lib_path, lib_install)

    lib64_path = os.path.join(sysroot_path, 'usr/lib64')
    lib64_install = os.path.join(install_sysroot, 'usr/lib64')
    if os.path.exists(lib64_path):
        shutil.copytree(lib64_path, lib64_install)

    prebuilt_path = os.path.join(NDK_DIR, 'prebuilt', host_tag)
    copy_directory_contents(prebuilt_path, install_path)

    toolchain_lib_dir = os.path.join(gcc_path, 'lib/gcc', triple)
    dirs = os.listdir(toolchain_lib_dir)
    assert len(dirs) == 1
    gcc_ver = dirs[0]

    cxx_headers = os.path.join(install_path, 'include/c++', gcc_ver)

    # Historically these were installed to the same directory as the C++
    # headers, but with the updated libc++ we have copies of a lot of those
    # headers in libc++ itself that we end up clobbering.
    #
    # This problem should go away with unified headers, but those aren't ready
    # yet. For the time being, install the libandroid_support headers to a
    # different builtin include path. usr/local/include seems to be the least
    # objectionable option.
    support_headers = os.path.join(install_path, 'sysroot/usr/local/include')

    if stl == 'gnustl':
        gnustl_dir = os.path.join(NDK_DIR, 'sources/cxx-stl/gnu-libstdc++/4.9')
        shutil.copytree(os.path.join(gnustl_dir, 'include'), cxx_headers)

        for abi in get_abis(arch):
            copy_gnustl_abi_headers(gnustl_dir, install_path, gcc_ver, triple,
                                    abi)
            copy_gnustl_libs(gnustl_dir, install_path, triple, abi)
            if arch == 'arm':
                copy_gnustl_abi_headers(gnustl_dir, install_path, gcc_ver,
                                        triple, abi, thumb=True)
                copy_gnustl_libs(gnustl_dir, install_path, triple, abi,
                                 thumb=True)
    elif stl == 'libc++':
        libcxx_dir = os.path.join(NDK_DIR, 'sources/cxx-stl/llvm-libc++')
        libcxxabi_dir = os.path.join(NDK_DIR, 'sources/cxx-stl/llvm-libc++abi')
        support_dir = os.path.join(NDK_DIR, 'sources/android/support')
        copy_directory_contents(os.path.join(libcxx_dir, 'include'),
                                cxx_headers)
        copy_directory_contents(os.path.join(support_dir, 'include'),
                                support_headers)

        # I have no idea why we need this, but the old one does it too.
        copy_directory_contents(
            os.path.join(libcxxabi_dir, 'include'),
            os.path.join(install_path, 'include/llvm-libc++abi/include'))

        headers = [
            'cxxabi.h',
            '__cxxabi_config.h',
        ]
        for header in headers:
            shutil.copy2(
                os.path.join(libcxxabi_dir, 'include', header),
                os.path.join(cxx_headers, header))

        for abi in get_abis(arch):
            src_libdir = get_src_libdir(libcxx_dir, abi)
            dest_libdir = get_dest_libdir(install_path, triple, abi)
            include_libunwind = arch == 'arm'
            copy_libcxx_libs(src_libdir, dest_libdir, include_libunwind)
            if arch == 'arm':
                thumb_libdir = os.path.join(dest_libdir, 'thumb')
                copy_libcxx_libs(src_libdir, thumb_libdir, include_libunwind)
    elif stl == 'stlport':
        stlport_dir = os.path.join(NDK_DIR, 'sources/cxx-stl/stlport')
        gabixx_dir = os.path.join(NDK_DIR, 'sources/cxx-stl/gabi++')

        copy_directory_contents(
            os.path.join(stlport_dir, 'stlport'), cxx_headers)

        # Same as for libc++. Not sure why we have this extra directory, but
        # keep the cruft for diff.
        copy_directory_contents(
            os.path.join(gabixx_dir, 'include'),
            os.path.join(install_path, 'include/gabi++/include'))

        headers = [
            'cxxabi.h',
            'unwind.h',
            'unwind-arm.h',
            'unwind-itanium.h',
            'gabixx_config.h',
        ]
        for header in headers:
            shutil.copy2(
                os.path.join(gabixx_dir, 'include', header),
                os.path.join(cxx_headers, header))

        for abi in get_abis(arch):
            copy_stlport_libs(stlport_dir, install_path, triple, abi)
            if arch == 'arm':
                copy_stlport_libs(stlport_dir, install_path, triple, abi,
                                  thumb=True)
    else:
        raise ValueError(stl)

    # Not needed for every STL, but the old one does this. Keep it for the sake
    # of diff. Done at the end so copytree works.
    cxx_target_headers = os.path.join(cxx_headers, triple)
    if not os.path.exists(cxx_target_headers):
        os.makedirs(cxx_target_headers)


def parse_args():
    """Parse command line arguments from sys.argv."""
    parser = argparse.ArgumentParser(
        description=inspect.getdoc(sys.modules[__name__]))

    parser.add_argument(
        '--arch', required=True,
        choices=('arm', 'arm64', 'mips', 'mips64', 'x86', 'x86_64'))
    parser.add_argument(
        '--api', type=int,
        help='Target the given API version (example: "--api 24").')
    parser.add_argument(
        '--stl', choices=('gnustl', 'libc++', 'stlport'), default='gnustl',
        help='C++ STL to use.')

    parser.add_argument(
        '--force', action='store_true',
        help='Remove existing installation directory if it exists.')
    parser.add_argument(
        '-v', '--verbose', action='count', help='Increase output verbosity.')

    output_group = parser.add_mutually_exclusive_group()
    output_group.add_argument(
        '--package-dir', type=os.path.realpath, default=os.getcwd(),
        help=('Build a tarball and install it to the given directory. If '
              'neither --package-dir nor --install-dir is specified, a '
              'tarball will be created and installed to the current '
              'directory.'))
    output_group.add_argument(
        '--install-dir', type=os.path.realpath,
        help='Install toolchain to the given directory instead of packaging.')

    return parser.parse_args()


def main():
    """Program entry point."""
    args = parse_args()

    if args.verbose is None:
        logging.basicConfig(level=logging.WARNING)
    elif args.verbose == 1:
        logging.basicConfig(level=logging.INFO)
    elif args.verbose >= 2:
        logging.basicConfig(level=logging.DEBUG)

    if args.arch.startswith('mips'):
        logger().warning(
            '%s is deprecated and will be removed in the next release.',
            args.arch)

    check_ndk_or_die()

    lp32 = args.arch in ('arm', 'mips', 'x86')
    min_api = 14 if lp32 else 21
    api = args.api
    if api is None:
        logger().warning(
            'Defaulting to target API %d (minimum supported target for %s)',
            min_api, args.arch)
        api = min_api
    elif api < min_api:
        sys.exit('{} is less than minimum platform for {} ({})'.format(
            api, args.arch, min_api))

    host_tag = get_host_tag_or_die()
    triple = get_triple(args.arch)
    sysroot_path = get_sysroot_path_or_die(args.arch, api)
    gcc_path = get_gcc_path_or_die(args.arch, host_tag)
    clang_path = get_clang_path_or_die(host_tag)

    if args.install_dir is not None:
        install_path = args.install_dir
        if os.path.exists(install_path):
            if args.force:
                logger().info('Cleaning installation directory %s',
                              install_path)
                shutil.rmtree(install_path)
            else:
                sys.exit('Installation directory already exists. Use --force.')
    else:
        tempdir = tempfile.mkdtemp()
        atexit.register(shutil.rmtree, tempdir)
        install_path = os.path.join(tempdir, triple)

    create_toolchain(install_path, args.arch, api, gcc_path, clang_path,
                     sysroot_path, args.stl, host_tag)

    if args.install_dir is None:
        if host_tag.startswith('windows'):
            package_format = 'zip'
        else:
            package_format = 'bztar'

        package_basename = os.path.join(args.package_dir, triple)
        shutil.make_archive(
            package_basename, package_format,
            root_dir=os.path.dirname(install_path),
            base_dir=os.path.basename(install_path))


if __name__ == '__main__':
    main()
