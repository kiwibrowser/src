#!/usr/bin/env python
#
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import print_function

import collections
import functools
import multiprocessing
import optparse
import os
import platform
import re
import shutil
import signal
import subprocess
import sys

SCRIPTS_DIR = os.path.abspath(os.path.dirname(__file__))
FFMPEG_DIR = os.path.abspath(os.path.join(SCRIPTS_DIR, '..', '..'))
CHROMIUM_ROOT_DIR = os.path.abspath(os.path.join(FFMPEG_DIR, '..', '..'))
NDK_ROOT_DIR = os.path.abspath(
    os.path.join(CHROMIUM_ROOT_DIR, 'third_party', 'android_ndk'))

sys.path.append(os.path.join(CHROMIUM_ROOT_DIR, 'build'))
import gn_helpers

BRANDINGS = [
    'Chrome',
    'ChromeOS',
    'Chromium',
]

ARCH_MAP = {
    'android': ['ia32', 'x64', 'mipsel', 'mips64el', 'arm-neon', 'arm64'],
    'linux': [
        'ia32', 'x64', 'mipsel', 'mips64el', 'noasm-x64', 'arm', 'arm-neon',
        'arm64'
    ],
    'mac': ['x64'],
    'win': ['ia32', 'x64'],
}

USAGE_BEGIN = """Usage: %prog TARGET_OS TARGET_ARCH [options] -- [configure_args]"""
USAGE_END = """
Valid combinations are android     [%(android)s]
                       linux       [%(linux)s]
                       mac         [%(mac)s]
                       win         [%(win)s]

If no target architecture is specified all will be built.

Platform specific build notes:
  android:
    Script can be run on a normal x64 Ubuntu box with an Android-ready Chromium
    checkout: https://chromium.googlesource.com/chromium/src/+/master/docs/android_build_instructions.md

  linux ia32/x64:
    Script can run on a normal Ubuntu box.

  linux arm/arm-neon/arm64/mipsel/mips64el:
    Script can run on a normal Ubuntu with ARM/ARM64 or MIPS32/MIPS64 ready Chromium checkout:
      build/linux/sysroot_scripts/install-sysroot.py --arch=arm
      build/linux/sysroot_scripts/install-sysroot.py --arch=arm64
      build/linux/sysroot_scripts/install-sysroot.py --arch=mips
      build/linux/sysroot_scripts/install-sysroot.py --arch=mips64el

  mac:
    Script must be run on OSX.  Additionally, ensure the Chromium (not Apple)
    version of clang is in the path; usually found under
    src/third_party/llvm-build/Release+Asserts/bin

  win:
    Script may be run unders Linux or Windows; if cross-compiling you will need
    to follow the Chromium instruction for Cross-compiling Chrome/win:
    https://chromium.googlesource.com/chromium/src/+/master/docs/win_cross.md

    Once you have a working Chromium build that can cross-compile, you'll also
    need to run $chrome_dir/tools/clang/scripts/download_objdump.py to pick up
    the llvm-ar and llvm-nm tools. You can then build as normal.

    If not cross-compiling, script must be run on Windows with VS2015 or higher
    under Cygwin (or MinGW, but as of 1.0.11, it has serious performance issues
    with make which makes building take hours).

    Additionally, ensure you have the correct toolchain environment for building.
    The x86 toolchain environment is required for ia32 builds and the x64 one
    for x64 builds.  This can be verified by running "cl.exe" and checking if
    the version string ends with "for x64" or "for x86."

    Building on Windows also requires some additional Cygwin packages plus a
    wrapper script for converting Cygwin paths to DOS paths.
      - Add these packages at install time: diffutils, yasm, make, python.
      - Copy chromium/scripts/cygwin-wrapper to /usr/local/bin

Resulting binaries will be placed in:
  build.TARGET_ARCH.TARGET_OS/Chrome/out/
  build.TARGET_ARCH.TARGET_OS/ChromeOS/out/
  build.TARGET_ARCH.TARGET_OS/Chromium/out/
  """


def PrintAndCheckCall(argv, *args, **kwargs):
  print('Running %s' % '\n '.join(argv))
  subprocess.check_call(argv, *args, **kwargs)


def DetermineHostOsAndArch():
  if platform.system() == 'Linux':
    host_os = 'linux'
  elif platform.system() == 'Darwin':
    host_os = 'mac'
  elif platform.system() == 'Windows' or 'CYGWIN_NT' in platform.system():
    host_os = 'win'
  else:
    return None

  if re.match(r'i.86', platform.machine()):
    host_arch = 'ia32'
  elif platform.machine() == 'x86_64' or platform.machine() == 'AMD64':
    host_arch = 'x64'
  elif platform.machine() == 'aarch64':
    host_arch = 'arm64'
  elif platform.machine() == 'mips32':
    host_arch = 'mipsel'
  elif platform.machine() == 'mips64':
    host_arch = 'mips64el'
  elif platform.machine().startswith('arm'):
    host_arch = 'arm'
  else:
    return None

  return (host_os, host_arch)


def GetDsoName(target_os, dso_name, dso_version):
  if target_os in ('linux', 'linux-noasm', 'android'):
    return 'lib%s.so.%s' % (dso_name, dso_version)
  elif target_os == 'mac':
    return 'lib%s.%s.dylib' % (dso_name, dso_version)
  elif target_os == 'win':
    return '%s-%s.dll' % (dso_name, dso_version)
  else:
    raise ValueError('Unexpected target_os %s' % target_os)


def RewriteFile(path, search_replace):
  with open(path) as f:
    contents = f.read()
  with open(path, 'w') as f:
    for search, replace in search_replace:
      contents = re.sub(search, replace, contents)
    f.write(contents)


# Extracts the Android toolchain version and api level from the Android
# config.gni.  Returns (api level, api 64 level, toolchain version).
def GetAndroidApiLevelAndToolchainVersion():
  android_config_gni = os.path.join(CHROMIUM_ROOT_DIR, 'build', 'config',
                                    'android', 'config.gni')
  with open(android_config_gni, 'r') as f:
    gni_contents = f.read()
    api64_match = re.search('android64_ndk_api_level\s*=\s*(\d{2})',
                            gni_contents)
    api_match = re.search('android32_ndk_api_level\s*=\s*(\d{2})', gni_contents)
    toolchain_match = re.search('_android_toolchain_version\s*=\s*"([.\d]+)"',
                                gni_contents)
    if not api_match or not toolchain_match or not api64_match:
      raise Exception('Failed to find the android api level or toolchain '
                      'version in ' + android_config_gni)

    return (api_match.group(1), api64_match.group(1), toolchain_match.group(1))


# Sets up cross-compilation (regardless of host arch) for compiling Android.
# Returns the necessary configure flags as a list.
def SetupAndroidToolchain(target_arch):
  api_level, api64_level, toolchain_version = (
      GetAndroidApiLevelAndToolchainVersion())

  # Toolchain prefix misery, for when just one pattern is not enough :/
  toolchain_level = api_level
  sysroot_arch = target_arch
  toolchain_dir_prefix = target_arch
  toolchain_bin_prefix = target_arch
  if target_arch == 'arm-neon' or target_arch == 'arm':
    toolchain_bin_prefix = toolchain_dir_prefix = 'arm-linux-androideabi'
    sysroot_arch = 'arm'
  elif target_arch == 'arm64':
    toolchain_level = api64_level
    toolchain_bin_prefix = toolchain_dir_prefix = 'aarch64-linux-android'
  elif target_arch == 'ia32':
    toolchain_dir_prefix = sysroot_arch = 'x86'
    toolchain_bin_prefix = 'i686-linux-android'
  elif target_arch == 'x64':
    toolchain_level = api64_level
    toolchain_dir_prefix = sysroot_arch = 'x86_64'
    toolchain_bin_prefix = 'x86_64-linux-android'
  elif target_arch == 'mipsel':
    sysroot_arch = 'mips'
    toolchain_bin_prefix = toolchain_dir_prefix = 'mipsel-linux-android'
  elif target_arch == 'mips64el':
    toolchain_level = api64_level
    sysroot_arch = 'mips64'
    toolchain_bin_prefix = toolchain_dir_prefix = 'mips64el-linux-android'

  sysroot = (
      NDK_ROOT_DIR + '/platforms/android-' + toolchain_level + '/arch-' +
      sysroot_arch)
  gcc_toolchain = (
      NDK_ROOT_DIR + '/toolchains/' + toolchain_dir_prefix + '-' +
      toolchain_version + '/prebuilt/linux-x86_64/')

  return [
      '--enable-cross-compile',
      '--sysroot=' + sysroot,

      # Android sysroot includes are now split out; try to cobble together the
      # correct tree.
      '--extra-cflags=-I' + NDK_ROOT_DIR + '/sysroot/usr/include',
      '--extra-cflags=-I' + NDK_ROOT_DIR + '/sysroot/usr/include/' +
      toolchain_bin_prefix,
      '--extra-cflags=--target=' + toolchain_bin_prefix,
      '--extra-ldflags=--target=' + toolchain_bin_prefix,
      '--extra-ldflags=--gcc-toolchain=' + gcc_toolchain,
      '--target-os=android',
  ]


def SetupWindowsCrossCompileToolchain(target_arch):
  # First retrieve various MSVC and Windows SDK paths.
  output = subprocess.check_output([
      os.path.join(CHROMIUM_ROOT_DIR, 'build', 'vs_toolchain.py'),
      'get_toolchain_dir'
  ])

  new_args = [
      '--enable-cross-compile',
      '--cc=clang-cl',
      '--ld=lld-link',
      '--nm=llvm-nm',
      '--ar=llvm-ar',

      # Separate from optflags because configure strips it from msvc builds...
      '--extra-cflags=-O2',
  ]

  if target_arch == 'ia32':
    new_args += ['--extra-cflags=-m32']
  if target_arch == 'ia32':
    target_arch = 'x86'

  # Turn this into a dictionary.
  win_dirs = gn_helpers.FromGNArgs(output)

  # Use those paths with a second script which will tell us the proper include
  # and lib paths to specify for cflags and ldflags respectively.
  output = subprocess.check_output([
      'python',
      os.path.join(CHROMIUM_ROOT_DIR, 'build', 'toolchain', 'win',
                   'setup_toolchain.py'), win_dirs['vs_path'],
      win_dirs['sdk_path'], win_dirs['runtime_dirs'], 'win', target_arch, 'none'
  ])

  flags = gn_helpers.FromGNArgs(output)
  for cflag in flags['include_flags_imsvc'].split(' '):
    new_args += ['--extra-cflags=' + cflag.strip('"')]

  # TODO(dalecurtis): Why isn't the ucrt path printed?
  flags['vc_lib_ucrt_path'] = flags['vc_lib_um_path'].replace('/um/', '/ucrt/')

  # Unlike the cflags, the lib include paths are each in a separate variable.
  for k in flags:
    if 'lib' in k:
      new_args += ['--extra-ldflags=-libpath:' + flags[k]]
  return new_args

def SetupMacCrossCompileToolchain():
  # First compute the various SDK paths.
  mac_min_ver = '10.10'
  sdk_dir = os.path.join(CHROMIUM_ROOT_DIR, 'build', 'win_files',
          'Xcode.app', 'Contents', 'Developer', 'Platforms', 'MacOSX.platform',
          'Developer', 'SDKs', 'MacOSX' + mac_min_ver + '.sdk')

  # We're guessing about the right sdk path, so warn if we don't find it.
  if not os.path.exists(sdk_dir):
      raise Exception("Can't find the mac sdk.  Please see crbug.com/841826")

  frameworks_dir = os.path.join(sdk_dir, "System", "Library", "Frameworks")
  libs_dir = os.path.join(sdk_dir, "usr", "lib")

  # ld64.lld is a symlink to clang's ld
  new_args = [
      '--enable-cross-compile',
      '--cc=clang',
      '--ld=ld64.lld',
      '--nm=llvm-nm',
      '--ar=llvm-ar',
      '--target-os=darwin',

      '--extra-cflags=--target=i686-apple-darwin-macho',
      '--extra-cflags=-F' + frameworks_dir,
      '--extra-cflags=-mmacosx-version-min=' + mac_min_ver
  ]

  new_args += [
      '--extra-cflags=-fblocks',
      '--extra-ldflags=-syslibroot', '--extra-ldflags=' + sdk_dir,
      '--extra-ldflags=' + '-L' + libs_dir,
      '--extra-ldflags=-lSystem',
      '--extra-ldflags=-macosx_version_min', '--extra-ldflags=' + mac_min_ver,
      '--extra-ldflags=-sdk_version', '--extra-ldflags=' + mac_min_ver]

  return new_args


def BuildFFmpeg(target_os, target_arch, host_os, host_arch, parallel_jobs,
                config_only, config, configure_flags):
  config_dir = 'build.%s.%s/%s' % (target_arch, target_os, config)
  shutil.rmtree(config_dir, ignore_errors=True)
  os.makedirs(os.path.join(config_dir, 'out'))

  PrintAndCheckCall(
      [os.path.join(FFMPEG_DIR, 'configure')] + configure_flags, cwd=config_dir)

  # These rewrites force disable various features and should be applied before
  # attempting the standalone ffmpeg build to make sure compilation succeeds.
  pre_make_rewrites = [
      (r'(#define HAVE_VALGRIND_VALGRIND_H [01])',
       r'#define HAVE_VALGRIND_VALGRIND_H 0 /* \1 -- forced to 0. See '
       r'https://crbug.com/590440 */')
  ]

  if target_os == 'android':
    pre_make_rewrites += [
        (r'(#define HAVE_POSIX_MEMALIGN [01])',
         r'#define HAVE_POSIX_MEMALIGN 0 /* \1 -- forced to 0. See '
         r'https://crbug.com/604451 */')
    ]

  # Linux configs is also used on Fuchsia. They are mostly compatible with
  # Fuchsia except that Fuchsia doesn't support sysctl(). On Linux sysctl()
  # isn't actually used, so it's safe to set HAVE_SYSCTL to 0.
  if target_os == 'linux':
    pre_make_rewrites += [
        (r'(#define HAVE_SYSCTL [01])',
         r'#define HAVE_SYSCTL 0 /* \1 -- forced to 0 for Fuchsia */')
    ]

  # Turn off bcrypt, since we don't have it on Windows builders, but it does
  # get detected when cross-compiling.
  if target_os == 'win':
    pre_make_rewrites += [
        (r'(#define HAVE_BCRYPT [01])',
         r'#define HAVE_BCRYPT 0')
    ]

  RewriteFile(os.path.join(config_dir, 'config.h'), pre_make_rewrites)

  # Windows linking resolves external symbols. Since generate_gn.py does not
  # need a functioning set of libraries, ignore unresolved symbols here.
  # This is especially useful here to avoid having to build a local libopus for
  # windows. We munge the output of configure here to avoid this LDFLAGS setting
  # triggering mis-detection during configure execution.
  if target_os == 'win':
    RewriteFile(
        os.path.join(config_dir, 'ffbuild/config.mak'), [(r'(LDFLAGS=.*)',
        r'\1 -FORCE:UNRESOLVED')])

  # TODO(https://crbug.com/840976): Linking when targetting mac on linux is
  # currently broken.
  # Replace the linker step with something that just creates the target.
  if target_os == 'mac' and host_os == 'linux':
    RewriteFile(
        os.path.join(config_dir, 'ffbuild/config.mak'), [(r'LD=ld64.lld',
        r'LD=' + os.path.join(SCRIPTS_DIR, 'fake_linker.py'))])

  if target_os in (host_os, host_os + '-noasm', 'android',
                   'win', 'mac') and not config_only:
    libraries = [
        os.path.join('libavcodec', GetDsoName(target_os, 'avcodec', 58)),
        os.path.join('libavformat', GetDsoName(target_os, 'avformat', 58)),
        os.path.join('libavutil', GetDsoName(target_os, 'avutil', 56)),
    ]
    PrintAndCheckCall(
        ['make', '-j%d' % parallel_jobs] + libraries, cwd=config_dir)
    for lib in libraries:
      shutil.copy(
          os.path.join(config_dir, lib), os.path.join(config_dir, 'out'))
  elif config_only:
    print('Skipping build step as requested.')
  else:
    print('Skipping compile as host configuration differs from target.\n'
          'Please compare the generated config.h with the previous version.\n'
          'You may also patch the script to properly cross-compile.\n'
          'Host OS : %s\n'
          'Target OS : %s\n'
          'Host arch : %s\n'
          'Target arch : %s\n' % (host_os, target_os, host_arch, target_arch))

  # These rewrites are necessary to faciliate various Chrome build options.
  post_make_rewrites = [
      (r'(#define FFMPEG_CONFIGURATION .*)',
       r'/* \1 -- elide long configuration string from binary */')
  ]

  # Sanitizers can't compile the h264 code when EBP is used.
  if target_arch == 'ia32':
    post_make_rewrites += [
        (r'(#define HAVE_EBP_AVAILABLE [01])',
         r'/* \1 -- ebp selection is done by the chrome build */')
    ]

  if target_arch in ('arm', 'arm-neon', 'arm64'):
    post_make_rewrites += [
        (r'(#define HAVE_VFP_ARGS [01])',
         r'/* \1 -- softfp/hardfp selection is done by the chrome build */')
    ]

  RewriteFile(os.path.join(config_dir, 'config.h'), post_make_rewrites)


def main(argv):
  clean_arch_map = {k: '|'.join(v) for k, v in ARCH_MAP.items()}
  formatted_usage_end = USAGE_END % clean_arch_map
  parser = optparse.OptionParser(usage=USAGE_BEGIN + formatted_usage_end)
  parser.add_option(
      '--branding',
      action='append',
      dest='brandings',
      choices=BRANDINGS,
      help='Branding to build; determines e.g. supported codecs')
  parser.add_option(
      '--config-only',
      action='store_true',
      help='Skip the build step. Useful when a given platform '
      'is not necessary for generate_gn.py')
  options, args = parser.parse_args(argv)

  if len(args) < 1:
    parser.print_help()
    return 1

  target_os = args[0]
  target_arch = ''
  if len(args) >= 2:
    target_arch = args[1]
  configure_args = args[2:]

  if target_os not in ('android', 'linux', 'linux-noasm', 'mac', 'win'):
    parser.print_help()
    return 1

  host_tuple = DetermineHostOsAndArch()
  if not host_tuple:
    print('Unrecognized host OS and architecture.', file=sys.stderr)
    return 1

  host_os, host_arch = host_tuple
  parallel_jobs = multiprocessing.cpu_count()

  if target_os == 'android' and host_os != 'linux' and host_arch != 'x64':
    print('Android cross compilation can only be done from a linux x64 host.')
    return 1

  if target_arch:
    print('System information:\n'
          'Host OS       : %s\n'
          'Target OS     : %s\n'
          'Host arch     : %s\n'
          'Target arch   : %s\n'
          'Parallel jobs : %d\n' % (host_os, target_os, host_arch, target_arch,
                                    parallel_jobs))
    ConfigureAndBuild(
        target_arch,
        target_os,
        host_os,
        host_arch,
        parallel_jobs,
        configure_args,
        options=options)
    return

  pool_size = len(ARCH_MAP[target_os])
  parallel_jobs = parallel_jobs / pool_size
  print('System information:\n'
        'Host OS       : %s\n'
        'Target OS     : %s\n'
        'Host arch     : %s\n'
        'Target arch   : %s\n'
        'Parallel jobs : %d\n' % (host_os, target_os, host_arch,
                                  ARCH_MAP[target_os], parallel_jobs))

  # Setup signal handles such that Ctrl+C works to terminate; will still result
  # in triggering a bunch of Python2 bugs and log spam, but will exit versus
  # hanging indefinitely.
  original_sigint_handler = signal.signal(signal.SIGINT, signal.SIG_IGN)
  p = multiprocessing.Pool(pool_size)
  signal.signal(signal.SIGINT, original_sigint_handler)
  try:
    result = p.map_async(
        functools.partial(
            ConfigureAndBuild,
            target_os=target_os,
            host_os=host_os,
            host_arch=host_arch,
            parallel_jobs=parallel_jobs,
            configure_args=configure_args,
            options=options), ARCH_MAP[target_os])
    # Timeout required or Ctrl+C is ignored; choose a ridiculous value so that
    # it doesn't trigger accidentally.
    result.get(1000)
  except Exception as e:
    p.terminate()
    # Re-throw the exception so that we fail if any subprocess fails.
    raise e


def ConfigureAndBuild(target_arch, target_os, host_os, host_arch, parallel_jobs,
                      configure_args, options):
  if target_os == 'linux' and target_arch == 'noasm-x64':
    target_os = 'linux-noasm'
    target_arch = 'x64'

  configure_flags = collections.defaultdict(list)

  # Common configuration.  Note: --disable-everything does not in fact disable
  # everything, just non-library components such as decoders and demuxers.
  configure_flags['Common'].extend([
      '--disable-everything',
      '--disable-all',
      '--disable-doc',
      '--disable-htmlpages',
      '--disable-manpages',
      '--disable-podpages',
      '--disable-txtpages',
      '--disable-static',
      '--enable-avcodec',
      '--enable-avformat',
      '--enable-avutil',
      '--enable-fft',
      '--enable-rdft',
      '--enable-static',
      '--enable-libopus',

      # Disable features.
      '--disable-debug',
      '--disable-bzlib',
      '--disable-error-resilience',
      '--disable-iconv',
      '--disable-lzo',
      '--disable-network',
      '--disable-schannel',
      '--disable-sdl2',
      '--disable-symver',
      '--disable-xlib',
      '--disable-zlib',
      '--disable-securetransport',
      '--disable-faan',
      '--disable-alsa',

      # Disable automatically detected external libraries. This prevents
      # automatic inclusion of things like hardware decoders. Each roll should
      # audit new [autodetect] configure options and add any desired options to
      # this file.
      '--disable-autodetect',

      # Common codecs.
      '--enable-decoder=vorbis,libopus,flac',
      '--enable-decoder=pcm_u8,pcm_s16le,pcm_s24le,pcm_s32le,pcm_f32le,mp3',
      '--enable-decoder=pcm_s16be,pcm_s24be,pcm_mulaw,pcm_alaw',
      '--enable-demuxer=ogg,matroska,wav,flac,mp3,mov',
      '--enable-parser=opus,vorbis,flac,mpegaudio',

      # Setup include path so Chromium's libopus can be used.
      '--extra-cflags=-I' + os.path.join(CHROMIUM_ROOT_DIR,
                                         'third_party/opus/src/include'),

      # Disable usage of Linux Performance API. Not used in production code, but
      # missing system headers break some Android builds.
      '--disable-linux-perf',

      # Force usage of yasm
      '--x86asmexe=yasm',
  ])

  if target_os == 'android':
    configure_flags['Common'].extend([
        # This replaces --optflags="-Os" since it implies it and since if it is
        # also specified, configure ends up dropping all optflags :/
        '--enable-small',
    ])

    configure_flags['Common'].extend(SetupAndroidToolchain(target_arch))
  else:
    configure_flags['Common'].extend([
        # --optflags doesn't append multiple entries, so set all at once.
        '--optflags="-O2"',
        '--enable-decoder=theora,vp8',
        '--enable-parser=vp3,vp8',
    ])

  if target_os in ('linux', 'linux-noasm', 'android'):
    if target_arch == 'x64':
      if target_os == 'android':
        configure_flags['Common'].extend([
            '--arch=x86_64',
        ])
      if target_os != 'android':
        configure_flags['Common'].extend(['--enable-lto'])
      pass
    elif target_arch == 'ia32':
      configure_flags['Common'].extend([
          '--arch=i686',
          '--extra-cflags="-m32"',
          '--extra-ldflags="-m32"',
      ])
      # Android ia32 can't handle textrels and ffmpeg can't compile without
      # them.  http://crbug.com/559379
      if target_os == 'android':
        configure_flags['Common'].extend([
            '--disable-x86asm',
        ])
    elif target_arch == 'arm' or target_arch == 'arm-neon':
      # TODO(ihf): ARM compile flags are tricky. The final options
      # overriding everything live in chroot /build/*/etc/make.conf
      # (some of them coming from src/overlays/overlay-<BOARD>/make.conf).
      # We try to follow these here closely. In particular we need to
      # set ffmpeg internal #defines to conform to make.conf.
      # TODO(ihf): For now it is not clear if thumb or arm settings would be
      # faster. I ran experiments in other contexts and performance seemed
      # to be close and compiler version dependent. In practice thumb builds are
      # much smaller than optimized arm builds, hence we go with the global
      # CrOS settings.
      configure_flags['Common'].extend([
          '--arch=arm',
          '--enable-armv6',
          '--enable-armv6t2',
          '--enable-vfp',
          '--enable-thumb',
          '--extra-cflags=-march=armv7-a',
      ])

      if target_os == 'android':
        configure_flags['Common'].extend([
            # Runtime neon detection requires /proc/cpuinfo access, so ensure
            # av_get_cpu_flags() is run outside of the sandbox when enabled.
            '--enable-neon',
            '--extra-cflags=-mtune=generic-armv7-a',
            # Enabling softfp lets us choose either softfp or hardfp when doing
            # the chrome build.
            '--extra-cflags=-mfloat-abi=softfp',
        ])
        if target_arch == 'arm':
          print('arm-neon is the only supported arm arch for Android.\n')
          return 1

        if target_arch == 'arm-neon':
          configure_flags['Common'].extend([
              '--extra-cflags=-mfpu=neon',
          ])
        else:
          configure_flags['Common'].extend([
              '--extra-cflags=-mfpu=vfpv3-d16',
          ])
      else:
        if host_arch != 'arm':
          configure_flags['Common'].extend([
              '--enable-cross-compile',
              '--target-os=linux',
              '--extra-cflags=--target=arm-linux-gnueabihf',
              '--extra-ldflags=--target=arm-linux-gnueabihf',
              '--sysroot=' + os.path.join(CHROMIUM_ROOT_DIR,
                                          'build/linux/debian_sid_arm-sysroot'),
              '--extra-cflags=-mtune=cortex-a8',
              # NOTE: we don't need softfp for this hardware.
              '--extra-cflags=-mfloat-abi=hard',
              # For some reason configure drops this...
              '--extra-cflags=-O2',
          ])

        if target_arch == 'arm-neon':
          configure_flags['Common'].extend([
              '--enable-neon',
              '--extra-cflags=-mfpu=neon',
          ])
        else:
          configure_flags['Common'].extend([
              '--disable-neon',
              '--extra-cflags=-mfpu=vfpv3-d16',
          ])
    elif target_arch == 'arm64':
      if target_os != 'android':
        configure_flags['Common'].extend([
            '--enable-cross-compile',
            '--cross-prefix=/usr/bin/aarch64-linux-gnu-',
            '--target-os=linux',
            '--extra-cflags=--target=aarch64-linux-gnu',
            '--extra-ldflags=--target=aarch64-linux-gnu',
            '--sysroot=' + os.path.join(CHROMIUM_ROOT_DIR,
                                        'build/linux/debian_sid_arm64-sysroot'),
        ])
      configure_flags['Common'].extend([
          '--arch=aarch64',
          '--enable-armv8',
          '--extra-cflags=-march=armv8-a',
      ])
    elif target_arch == 'mipsel':
      # These flags taken from android chrome build with target_cpu='mipsel'
      configure_flags['Common'].extend([
          '--arch=mipsel',
          '--disable-mips32r6',
          '--disable-mips32r5',
          '--disable-mips32r2',
          '--disable-mipsdsp',
          '--disable-mipsdspr2',
          '--disable-msa',
          '--enable-mipsfpu',
          '--extra-cflags=-march=mipsel',
          '--extra-cflags=-mcpu=mips32',
          # Required to avoid errors about dynamic relocation w/o -fPIC.
          '--extra-ldflags=-z notext',
      ])
      if target_os == 'linux':
        configure_flags['Common'].extend([
            '--enable-cross-compile',
            '--target-os=linux',
            '--sysroot=' + os.path.join(CHROMIUM_ROOT_DIR,
                                        'build/linux/debian_sid_mips-sysroot'),
            '--extra-cflags=--target=mipsel-linux-gnu',
            '--extra-ldflags=--target=mipsel-linux-gnu',
        ])
    elif target_arch == 'mips64el':
      # These flags taken from android chrome build with target_cpu='mips64el'
      configure_flags['Common'].extend([
          '--arch=mips64el',
          '--enable-mipsfpu',
          '--disable-mipsdsp',
          '--disable-mipsdspr2',
          '--extra-cflags=-march=mips64el',
          # Required to avoid errors about dynamic relocation w/o -fPIC.
          '--extra-ldflags=-z notext',
      ])
      if target_os == 'android':
        configure_flags['Common'].extend([
            '--enable-mips64r6',
            '--extra-cflags=-mcpu=mips64r6',
            '--disable-mips64r2',
            '--enable-msa',
        ])
      if target_os == 'linux':
        configure_flags['Common'].extend([
            '--enable-cross-compile',
            '--target-os=linux',
            '--sysroot=' + os.path.join(
                CHROMIUM_ROOT_DIR, 'build/linux/debian_sid_mips64el-sysroot'),
            '--enable-mips64r2',
            '--disable-mips64r6',
            '--disable-msa',
            '--extra-cflags=-mcpu=mips64r2',
            '--extra-cflags=--target=mips64el-linux-gnuabi64',
            '--extra-ldflags=--target=mips64el-linux-gnuabi64',
        ])
    else:
      print(
          'Error: Unknown target arch %r for target OS %r!' % (target_arch,
                                                               target_os),
          file=sys.stderr)
      return 1

  if target_os == 'linux-noasm':
    configure_flags['Common'].extend([
        '--disable-asm',
        '--disable-inline-asm',
    ])

  if 'win' not in target_os:
    configure_flags['Common'].extend([
        '--enable-pic',
        '--cc=clang',
        '--cxx=clang++',
        '--ld=clang',
    ])

    # Clang Linux will use the first 'ld' it finds on the path, which will
    # typically be the system one, so explicitly configure use of Clang's
    # ld.lld, to ensure that things like cross-compilation and LTO work.
    # This does not work for arm64, ia32 and is always used on mac.
    if target_arch not in ['arm64', 'ia32', 'mipsel'] and target_os != 'mac':
      configure_flags['Common'].append('--extra-ldflags=-fuse-ld=lld')

  # Should be run on Mac, unless we're cross-compiling on Linux.
  if target_os == 'mac':
    if host_os != 'mac' and host_os != 'linux':
      print(
          'Script should be run on a Mac or Linux host.\n',
          file=sys.stderr)
      return 1

    if host_os != 'mac':
      configure_flags['Common'].extend(SetupMacCrossCompileToolchain())
    else:
      # Mac dylib building resolves external symbols. We need to explicitly
      # include -lopus to point to the system libopus so we can build
      # libavcodec.XX.dylib.A
      # For cross-compiling, this doesn't work, and doesn't seem to be needed.
      configure_flags['Common'].extend([
          '--extra-libs=-lopus',
      ])

    if target_arch == 'x64':
      configure_flags['Common'].extend([
          '--arch=x86_64',
          '--extra-cflags=-m64',
          '--extra-ldflags=-m64',
      ])
    else:
      print(
          'Error: Unknown target arch %r for target OS %r!' % (target_arch,
                                                               target_os),
          file=sys.stderr)

  # Should be run on Windows.
  if target_os == 'win':
    configure_flags['Common'].extend([
        '--toolchain=msvc',
        '--extra-cflags=-I' + os.path.join(FFMPEG_DIR, 'chromium/include/win'),
    ])

    if target_arch == 'x64':
      configure_flags['Common'].extend(['--target-os=win64'])
    else:
      configure_flags['Common'].extend(['--target-os=win32'])

    if host_os != 'win':
      configure_flags['Common'].extend(
          SetupWindowsCrossCompileToolchain(target_arch))

    if 'CYGWIN_NT' in platform.system():
      configure_flags['Common'].extend([
          '--cc=cygwin-wrapper cl',
          '--ld=cygwin-wrapper link',
          '--nm=cygwin-wrapper dumpbin -symbols',
          '--ar=cygwin-wrapper lib',
      ])

  # Google Chrome & ChromeOS specific configuration.
  configure_flags['Chrome'].extend([
      '--enable-decoder=aac,h264',
      '--enable-demuxer=aac',
      '--enable-parser=aac,h264',
  ])

  # Google ChromeOS specific configuration.
  # We want to make sure to play everything Android generates and plays.
  # http://developer.android.com/guide/appendix/media-formats.html
  configure_flags['ChromeOS'].extend([
      # Enable playing avi files.
      '--enable-decoder=mpeg4',
      '--enable-parser=h263,mpeg4video',
      '--enable-demuxer=avi',
      # Enable playing Android 3gp files.
      '--enable-demuxer=amr',
      '--enable-decoder=amrnb,amrwb',
      # Wav files for playing phone messages.
      '--enable-decoder=gsm_ms',
      '--enable-parser=gsm',
  ])

  configure_flags['ChromeAndroid'].extend([
      '--enable-demuxer=aac',
      '--enable-parser=aac',
      '--enable-decoder=aac',

      # TODO(dalecurtis, watk): Figure out if we need h264 parser for now?
  ])

  def do_build_ffmpeg(branding, configure_flags):
    if options.brandings and branding not in options.brandings:
      print('%s skipped' % branding)
      return

    print('%s configure/build:' % branding)
    BuildFFmpeg(target_os, target_arch, host_os, host_arch, parallel_jobs,
                options.config_only, branding, configure_flags)

  # Only build Chromium, Chrome for ia32, x86 non-android platforms.
  if target_os != 'android':
    do_build_ffmpeg(
        'Chromium', configure_flags['Common'] + configure_flags['Chromium'] +
        configure_args)
    do_build_ffmpeg(
        'Chrome',
        configure_flags['Common'] + configure_flags['Chrome'] + configure_args)
  else:
    do_build_ffmpeg('Chromium', configure_flags['Common'] + configure_args)
    do_build_ffmpeg(
        'Chrome', configure_flags['Common'] + configure_flags['ChromeAndroid'] +
        configure_args)

  if target_os in ['linux', 'linux-noasm']:
    # ChromeOS enables MPEG4 which requires error resilience :(
    chrome_os_flags = (
        configure_flags['Common'] + configure_flags['Chrome'] +
        configure_flags['ChromeOS'] + configure_args)
    chrome_os_flags.remove('--disable-error-resilience')
    do_build_ffmpeg('ChromeOS', chrome_os_flags)

  print('Done. If desired you may copy config.h/config.asm into the '
        'source/config tree using copy_config.sh.')
  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
