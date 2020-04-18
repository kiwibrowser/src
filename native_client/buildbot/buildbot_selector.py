#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import os
import subprocess
import sys
import tempfile

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import pynacl.platform

python = sys.executable
bash = '/bin/bash'
echo = 'echo'


BOT_ASSIGNMENT = {
    ######################################################################
    # Buildbots.
    ######################################################################
    'xp-newlib-opt':
        python + ' buildbot\\buildbot_standard.py opt 32 newlib',
    'xp-glibc-opt':
        python + ' buildbot\\buildbot_standard.py opt 32 glibc',

    'xp-bare-newlib-opt':
        python + ' buildbot\\buildbot_standard.py opt 32 newlib',
    'xp-bare-glibc-opt':
        python + ' buildbot\\buildbot_standard.py opt 32 glibc',

    'linux-64-validator-opt':
        python + ' buildbot/buildbot_standard.py opt 64 glibc --validator',

    # ASan.
    'linux_64-newlib-dbg-asan':
        python + ' buildbot/buildbot_standard.py opt 64 newlib --asan',
    'mac-newlib-dbg-asan':
        python + ' buildbot/buildbot_standard.py opt 32 newlib --asan',

    # Sanitizer Pnacl toolchain buildbot.
    'asan':
        python +
        ' buildbot/buildbot_pnacl_toolchain.py --buildbot --tests-arch x86-64 '
        ' --sanitize address --skip-tests',

    # PNaCl.
    'oneiric_32-newlib-arm_hw-pnacl-panda-dbg':
        bash + ' buildbot/buildbot_pnacl.sh mode-buildbot-arm-hw-dbg',
    'oneiric_32-newlib-arm_hw-pnacl-panda-opt':
        bash + ' buildbot/buildbot_pnacl.sh mode-buildbot-arm-hw-opt',
    'linux_64-newlib-arm_qemu-pnacl-dbg':
        bash + ' buildbot/buildbot_pnacl.sh mode-buildbot-arm-dbg',
    'linux_64-newlib-arm_qemu-pnacl-opt':
        bash + ' buildbot/buildbot_pnacl.sh mode-buildbot-arm-opt',
    'linux_64-newlib-x86_32-pnacl':
        python + ' buildbot/buildbot_pnacl.py opt 32 pnacl',
    'linux_64-newlib-x86_64-pnacl':
        python + ' buildbot/buildbot_pnacl.py opt 64 pnacl',
    'mac-newlib-opt-pnacl':
        python + ' buildbot/buildbot_pnacl.py opt 64 pnacl',
    'win7-64-newlib-opt-pnacl':
        python + ' buildbot/buildbot_pnacl.py opt 64 pnacl',
    # TODO: Use buildbot_pnacl.py instead of buildbot_pnacl.sh once
    # the gyp_build is moved to buildbot_pnacl.py.
    'linux_64-newlib-mips-pnacl':
        bash + ' buildbot/buildbot_pnacl.sh mode-trybot-qemu mips32',
    # PNaCl Spec
    'linux_64-newlib-arm_qemu-pnacl-buildonly-spec':
        bash + ' buildbot/buildbot_spec2k.sh pnacl-arm-buildonly',
    'oneiric_32-newlib-arm_hw-pnacl-panda-spec':
        bash + ' buildbot/buildbot_spec2k.sh pnacl-arm-hw',
    'linux_64-newlib-x86_32-pnacl-spec':
        bash + ' buildbot/buildbot_spec2k.sh pnacl-x8632',
    'linux_64-newlib-x86_64-pnacl-spec':
        bash + ' buildbot/buildbot_spec2k.sh pnacl-x8664',
    # NaCl Spec
    'linux_64-newlib-x86_32-spec':
        bash + ' buildbot/buildbot_spec2k.sh nacl-x8632',
    'linux_64-newlib-x86_64-spec':
        bash + ' buildbot/buildbot_spec2k.sh nacl-x8664',

    # Valgrind bots.
    'linux-64-newlib-dbg-valgrind':
        echo + ' "Valgrind bots are disabled: see '
            'https://code.google.com/p/nativeclient/issues/detail?id=3158"',
    'linux-64-glibc-dbg-valgrind':
        echo + ' "Valgrind bots are disabled: see '
            'https://code.google.com/p/nativeclient/issues/detail?id=3158"',

    ######################################################################
    # Trybots.
    ######################################################################
    'nacl-precise64_validator_opt':
        python + ' buildbot/buildbot_standard.py opt 64 glibc --validator',
    # Android trybots.
    'nacl-precise64-newlib-dbg-android':
        echo + ' "Android bots are disabled and going away"',
    'nacl-precise64-newlib-opt-android':
        echo + ' "Android bots are disabled and going away"',
    # ASan.
    'nacl-precise_64-newlib-dbg-asan':
        python + ' buildbot/buildbot_standard.py opt 64 newlib --asan',
    'nacl-mac-newlib-dbg-asan':
        python + ' buildbot/buildbot_standard.py opt 32 newlib --asan',
    # Pnacl main trybots
    'nacl-precise_64-newlib-arm_qemu-pnacl':
        bash + ' buildbot/buildbot_pnacl.sh mode-trybot-qemu arm',
    'nacl-precise_64-newlib-x86_32-pnacl':
         python + ' buildbot/buildbot_pnacl.py opt 32 pnacl',
    'nacl-precise_64-newlib-x86_64-pnacl':
         python + ' buildbot/buildbot_pnacl.py opt 64 pnacl',
    'nacl-precise_64-newlib-mips-pnacl':
        bash + ' buildbot/buildbot_pnacl.sh mode-trybot-qemu mips32',
    'nacl-arm_opt_panda':
        bash + ' buildbot/buildbot_pnacl.sh mode-buildbot-arm-try',
    'nacl-arm_hw_opt_panda':
        bash + ' buildbot/buildbot_pnacl.sh mode-buildbot-arm-hw-try',
    'nacl-mac_newlib_opt_pnacl':
        python + ' buildbot/buildbot_pnacl.py opt 64 pnacl',
    'nacl-win7_64_newlib_opt_pnacl':
        python + ' buildbot/buildbot_pnacl.py opt 64 pnacl',
    # Pnacl spec2k trybots
    'nacl-precise_64-newlib-x86_32-pnacl-spec':
        bash + ' buildbot/buildbot_spec2k.sh pnacl-trybot-x8632',
    'nacl-precise_64-newlib-x86_64-pnacl-spec':
        bash + ' buildbot/buildbot_spec2k.sh pnacl-trybot-x8664',
    'nacl-arm_perf_panda':
        bash + ' buildbot/buildbot_spec2k.sh pnacl-trybot-arm-buildonly',
    'nacl-arm_hw_perf_panda':
        bash + ' buildbot/buildbot_spec2k.sh pnacl-trybot-arm-hw',
    # Toolchain glibc.
    'precise64-glibc': bash + ' buildbot/buildbot_linux-glibc-makefile.sh',
    'mac-glibc': bash + ' buildbot/buildbot_mac-glibc-makefile.sh',
    'win7-glibc': 'buildbot\\buildbot_windows-glibc-makefile.bat',
    # Toolchain (glibc) ARM.
    'win7-toolchain_arm':
        python +
        ' buildbot/buildbot_toolchain_build.py'
        ' --buildbot'
        ' toolchain_build',
    'mac-toolchain_arm':
        python +
        ' buildbot/buildbot_toolchain_build.py'
        ' --buildbot'
        ' toolchain_build',
    'linux64-toolchain_arm':
        python +
        ' buildbot/buildbot_toolchain_build.py'
        ' --buildbot'
        ' --test_toolchain nacl_arm_glibc_raw'
        ' toolchain_build',

    # Pnacl toolchain builders.
    'linux-pnacl-x86_32':
        python +
        ' buildbot/buildbot_pnacl_toolchain.py --buildbot --tests-arch x86-32',
    'linux-pnacl-x86_64':
        python +
        ' buildbot/buildbot_pnacl_toolchain.py --buildbot --tests-arch x86-64',
    'mac-pnacl-x86_32':
        python +
        ' buildbot/buildbot_pnacl_toolchain.py --buildbot',
    'win-pnacl-x86_32':
        python +
        ' buildbot/buildbot_pnacl_toolchain.py --buildbot',

    # Pnacl toolchain testers
    'linux-pnacl-x86_64-tests-x86_64':
        bash + ' buildbot/buildbot_pnacl_toolchain_tests.sh tc-test-bot x86-64',
    'linux-pnacl-x86_64-tests-x86_32':
        bash + ' buildbot/buildbot_pnacl_toolchain_tests.sh tc-test-bot x86-32',
    'linux-pnacl-x86_64-tests-arm':
        bash + ' buildbot/buildbot_pnacl_toolchain_tests.sh tc-test-bot arm',

    # MIPS toolchain buildbot.
    'linux-pnacl-x86_32-tests-mips':
        bash + ' buildbot/buildbot_pnacl.sh mode-trybot-qemu mips32',

    # Toolchain trybots.
    'nacl-toolchain-precise64-newlib-arm': # TODO(bradnelson): rename
        python +
        ' buildbot/buildbot_toolchain_build.py'
        ' --trybot'
        ' --test_toolchain nacl_arm_glibc_raw'
        ' toolchain_build',
    'nacl-toolchain-mac-newlib-arm': # TODO(bradnelson): rename
        python +
        ' buildbot/buildbot_toolchain_build.py'
        ' --trybot'
        ' toolchain_build',
    'nacl-toolchain-win7-newlib-arm': # TODO(bradnelson): rename
        python +
        ' buildbot/buildbot_toolchain_build.py'
        ' --trybot'
        ' toolchain_build',
    'nacl-toolchain-precise64-glibc':
        bash + ' buildbot/buildbot_linux-glibc-makefile.sh',
    'nacl-toolchain-mac-glibc':
        bash + ' buildbot/buildbot_mac-glibc-makefile.sh',
    'nacl-toolchain-win7-glibc':
        'buildbot\\buildbot_windows-glibc-makefile.bat',

    # Pnacl toolchain trybots.
    'nacl-toolchain-linux-pnacl-x86_32':
        python +
        ' buildbot/buildbot_pnacl_toolchain.py --trybot --tests-arch x86-32',
    'nacl-toolchain-linux-pnacl-x86_64':
        python +
        ' buildbot/buildbot_pnacl_toolchain.py --trybot --tests-arch x86-64',
    'nacl-toolchain-mac-pnacl-x86_32':
        python + ' buildbot/buildbot_pnacl_toolchain.py --trybot',
    'nacl-toolchain-win7-pnacl-x86_64':
        python + ' buildbot/buildbot_pnacl_toolchain.py --trybot',

    # Sanitizer Pnacl toolchain trybots.
    'nacl-toolchain-asan':
        python +
        ' buildbot/buildbot_pnacl_toolchain.py --trybot --tests-arch x86-64 '
        ' --sanitize address --skip-tests',
    # TODO(kschimpf): Bot is currently broken: --sanitize memory not understood.
    'nacl-toolchain-msan':
        python +
        ' buildbot/buildbot_pnacl_toolchain.py --trybot --tests-arch x86-64 '
        ' --sanitize memory --skip-tests',
    # TODO(kschimpf): Bot is currently broken: --sanitize thread not understood.
    'nacl-toolchain-tsan':
        python +
        ' buildbot/buildbot_pnacl_toolchain.py --trybot --tests-arch x86-64 '
        ' --sanitize thread --skip-tests',
    # TODO(kschimpf): Bot is currently broken: --sanitize undefined not understood.
    'nacl-toolchain-ubsan':
        python +
        ' buildbot/buildbot_pnacl_toolchain.py --trybot --tests-arch x86-64 '
        ' --sanitize undefined --skip-tests',

}

special_for_arm = [
    'win7_64',
    'win7-64',
    'linux-64',
    'linux64'
]
for platform in [
    'vista', 'win7', 'win8', 'win',
    'mac', 'linux'] + special_for_arm:
  if platform in special_for_arm:
    arch_variants = ['arm']
  else:
    arch_variants = ['', '32', '64', 'arm']
  for arch in arch_variants:
    arch_flags = ''
    real_arch = arch
    arch_part = '-' + arch
    if platform == 'win7' and arch == '32':
      arch_flags += ' --no-goma'
    if arch == '':
      arch_part = ''
      real_arch = '32'
    # Test with Breakpad tools only on basic Linux builds.
    if sys.platform.startswith('linux'):
      arch_flags += ' --use-breakpad-tools'
    # GN executable can no longer run on Vista.
    if platform == 'vista':
      arch_flags += ' --no-gn'
    for mode in ['dbg', 'opt']:
      for libc in ['newlib', 'glibc']:
        # Buildbots.
        for bare in ['', '-bare']:
          for test in ['', '-test']:
            name = platform + arch_part + bare + '-' + libc + '-' + mode + test
            assert name not in BOT_ASSIGNMENT, name
            BOT_ASSIGNMENT[name] = (
                python + ' buildbot/buildbot_standard.py ' +
                mode + ' ' + real_arch + ' ' + libc + arch_flags)
        # Trybots
        for arch_sep in ['', '-', '_']:
          p = platform.replace('linux', 'precise')
          name = 'nacl-' + p + arch_sep + arch + '_' + libc + '_' + mode
          assert name not in BOT_ASSIGNMENT, name
          BOT_ASSIGNMENT[name] = (
              python + ' buildbot/buildbot_standard.py ' +
              mode + ' ' + real_arch + ' ' + libc + arch_flags)


def EscapeJson(data):
  return '"' + json.dumps(data).replace('"', r'\"') + '"'


def HasNoPerfResults(builder):
  if 'pnacl-buildonly-spec' in builder:
    return True
  if 'android' in builder:
    return True
  return builder in [
      'asan',
      'mac-toolchain_arm',
      'win-pnacl-x86_32',
      'linux-pnacl-x86_32-tests-mips',
  ]


def Main():
  builder = os.environ.get('BUILDBOT_BUILDERNAME')
  build_number = os.environ.get('BUILDBOT_BUILDNUMBER')
  build_revision = os.environ.get('BUILDBOT_GOT_REVISION',
                                  os.environ.get('BUILDBOT_REVISION'))
  slave_type = os.environ.get('BUILDBOT_SLAVE_TYPE')
  cmd = BOT_ASSIGNMENT.get(builder)
  if not cmd:
    sys.stderr.write('ERROR - unset/invalid builder name\n')
    sys.exit(1)

  env = os.environ.copy()

  # Don't write out .pyc files because in cases in which files move around or
  # the PYTHONPATH / sys.path change, old .pyc files can be mistakenly used.
  # This avoids the need for admin changes on the bots in this case.
  env['PYTHONDONTWRITEBYTECODE'] = '1'

  # Use .boto file from home-dir instead of buildbot supplied one.
  if 'AWS_CREDENTIAL_FILE' in env:
    del env['AWS_CREDENTIAL_FILE']
  alt_boto = os.path.expanduser('~/.boto')
  if os.path.exists(alt_boto):
    env['BOTO_CONFIG'] = alt_boto
  cwd_drive = os.path.splitdrive(os.getcwd())[0]
  env['GSUTIL'] = cwd_drive + '/b/depot_tools/gsutil.py'

  # When running from cygwin, we sometimes want to use a native python.
  # The native python will use the depot_tools version by invoking python.bat.
  if pynacl.platform.IsWindows():
    env['NATIVE_PYTHON'] = 'python.bat'
  else:
    env['NATIVE_PYTHON'] = 'python'

  if sys.platform == 'win32':
    # If the temp directory is not on the same drive as the working directory,
    # there can be random failures when cleaning up temp directories, so use
    # a directory on the current drive. Use __file__ here instead of os.getcwd()
    # because toolchain_main picks its working directories relative to __file__
    filedrive, _ = os.path.splitdrive(__file__)
    tempdrive, _ = os.path.splitdrive(env['TEMP'])
    if tempdrive != filedrive:
      env['TEMP'] = filedrive + '\\temp'
      if not os.path.exists(env['TEMP']):
        os.mkdir(env['TEMP'])

  # Ensure a temp directory exists.
  if 'TEMP' not in env:
    env['TEMP'] = tempfile.gettempdir()

  # Isolate build's temp directory to a particular location so we can clobber
  # the whole thing predictably and so we have a record of who's leaking
  # temporary files.
  nacl_tmp = os.path.join(env['TEMP'], 'nacl_tmp')
  if not os.path.exists(nacl_tmp):
    os.mkdir(nacl_tmp)
  env['TEMP'] = os.path.join(nacl_tmp, builder)
  if not os.path.exists(env['TEMP']):
    os.mkdir(env['TEMP'])

  # Set all temp directory variants to the same thing.
  env['TMPDIR'] = env['TEMP']
  env['TMP'] = env['TEMP']
  print 'TEMP=%s' % env['TEMP']
  sys.stdout.flush()

  # Run through runtest.py to get upload of perf data.
  build_properties = {
      'buildername': builder,
      'mastername': 'client.nacl',
      'buildnumber': str(build_number),
  }
  factory_properties = {
      'perf_id': builder,
      'show_perf_results': True,
      'step_name': 'naclperf',  # Seems unused, but is required.
      'test_name': 'naclperf',  # Really "Test Suite"
  }
  # Get runtest from the environment, is available.
  runtest = os.environ.get('RUNTEST')
  if runtest is None:
    # Locate the buildbot build directory by relative path, as it's absolute
    # location varies by platform and configuration.
    buildbot_build_dir = os.path.join(* [os.pardir] * 4)
    runtest = os.path.join(buildbot_build_dir, 'scripts', 'slave', 'runtest.py')
  # For builds with an actual build number, require that the script is present
  # (i.e. that we're run from an actual buildbot).
  if build_number is not None and not os.path.exists(runtest):
    raise Exception('runtest.py script not found at: %s\n' % runtest)
  cmd_exe = cmd.split(' ')[0]
  cmd_exe_ext = os.path.splitext(cmd_exe)[1]
  # Do not wrap these types of builds with runtest.py:
  # - tryjobs
  # - commands beginning with 'echo '
  # - batch files
  # - debug builders
  # - builds with no perf tests
  if not (slave_type == 'Trybot' or
          cmd_exe == echo or
          cmd_exe_ext == '.bat' or
          '-dbg' in builder or
          HasNoPerfResults(builder)):
    # Perf dashboards are now generated by output scraping that occurs in the
    # script runtest.py, which lives in the buildbot repository.
    # Non-trybot builds should be run through runtest, allowing it to upload
    # perf data if relevant.
    cmd = ' '.join([
        python, runtest,
        '--revision=' + build_revision,
        '--build-dir=src/out',
        '--results-url=https://chromeperf.appspot.com',
        '--annotate=graphing',
        '--no-xvfb',  # We provide our own xvfb invocation.
        '--factory-properties', EscapeJson(factory_properties),
        '--build-properties', EscapeJson(build_properties),
        cmd,
    ])

  print "%s runs: %s\n" % (builder, cmd)
  sys.stdout.flush()
  retcode = subprocess.call(cmd, env=env, shell=True)
  sys.exit(retcode)


if __name__ == '__main__':
  Main()
