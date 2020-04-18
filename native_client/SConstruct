#! -*- python -*-
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import atexit
import json
import os
import platform
import re
import subprocess
import sys
import zlib
sys.path.append("./common")
sys.path.append('../third_party')

from SCons.Errors import UserError
from SCons.Script import GetBuildFailures

import SCons.Warnings
import SCons.Util

SCons.Warnings.warningAsException()

sys.path.append("tools")
import command_tester
import test_lib

import pynacl.platform

# turning garbage collection off reduces startup time by 10%
import gc
gc.disable()

# REPORT
CMD_COUNTER = {}
ENV_COUNTER = {}
def PrintFinalReport():
  """This function is run just before scons exits and dumps various reports.
  """
  # Note, these global declarations are not strictly necessary
  global pre_base_env
  global CMD_COUNTER
  global ENV_COUNTER

  if pre_base_env.Bit('target_stats'):
    print
    print '*' * 70
    print 'COMMAND EXECUTION REPORT'
    print '*' * 70
    for k in sorted(CMD_COUNTER.keys()):
      print "%4d %s" % (CMD_COUNTER[k], k)

    print
    print '*' * 70
    print 'ENVIRONMENT USAGE REPORT'
    print '*' * 70
    for k in sorted(ENV_COUNTER.keys()):
      print "%4d  %s" % (ENV_COUNTER[k], k)

  failures = []
  for failure in GetBuildFailures():
    for node in Flatten(failure.node):
      failures.append({
          # If this wasn't a test, "GetTestName" will return raw_name.
          'test_name': GetTestName(node),
          'raw_name': str(node.path),
          'errstr': failure.errstr
      })

  json_path = ARGUMENTS.get('json_build_results_output_file')
  if json_path:
    with open(json_path, 'w') as f:
      json.dump(failures, f, sort_keys=True, indent=2)

  if not failures:
    return

  print
  print '*' * 70
  print 'ERROR REPORT: %d failures' % len(failures)
  print '*' * 70
  print
  for failure in failures:
    test_name = failure['test_name']
    if test_name != failure['raw_name']:
      test_name = '%s (%s)' % (test_name, failure['raw_name'])
    print "%s failed: %s\n" % (test_name, failure['errstr'])


def VerboseConfigInfo(env):
  "Should we print verbose config information useful for bug reports"
  if '--help' in sys.argv: return False
  if env.Bit('prebuilt') or env.Bit('built_elsewhere'): return False
  return env.Bit('sysinfo')


# SANITY CHECKS

# NOTE BitFromArgument(...) implicitly defines additional ACCEPTABLE_ARGUMENTS.
ACCEPTABLE_ARGUMENTS = set([
    # TODO: add comments what these mean
    # TODO: check which ones are obsolete
    ####  ASCII SORTED ####
    # Use a destination directory other than the default "scons-out".
    'DESTINATION_ROOT',
    'MODE',
    'SILENT',
    # Limit bandwidth of browser tester
    'browser_tester_bw',
    # Location to download Chromium binaries to and/or read them from.
    'chrome_binaries_dir',
    # used for chrome_browser_tests: path to the browser
    'chrome_browser_path',
    # A comma-separated list of test names to disable by excluding the
    # tests from a test suite.  For example, 'small_tests
    # disable_tests=run_hello_world_test' will run small_tests without
    # including hello_world_test.  Note that if a test listed here
    # does not exist you will not get an error or a warning.
    'disable_tests',
    # used for chrome_browser_tests: path to a pre-built browser plugin.
    'force_ppapi_plugin',
    # force emulator use by tests
    'force_emulator',
    # force sel_ldr use by tests
    'force_sel_ldr',
    # force nacl_helper_bootstrap used by tests
    'force_bootstrap',
    # force irt image used by tests
    'force_irt',
    # generate_ninja=FILE enables a Ninja backend for SCons.  This writes a
    # .ninja build file to FILE describing all of SCons' build targets.
    'generate_ninja',
    # Path to a JSON file for machine-readable output.
    'json_build_results_output_file',
    # Replacement memcheck command for overriding the DEPS-in memcheck
    # script.  May have commas to separate separate shell args.  There
    # is no quoting, so this implies that this mechanism will fail if
    # the args actually need to have commas.  See
    # http://code.google.com/p/nativeclient/issues/detail?id=3158 for
    # the discussion of why this argument is needed.
    'memcheck_command',
    # If the replacement memcheck command only works for trusted code,
    # set memcheck_trusted_only to non-zero.
    'memcheck_trusted_only',
    # When building with MSan, this can be set to values 0 (fastest, least
    # useful reports) through 2 (slowest, most useful reports). Default is 1.
    'msan_track_origins',
    # colon-separated list of linker flags, e.g. "-lfoo:-Wl,-u,bar".
    'nacl_linkflags',
    # prefix to add in-front of perf tracking trace labels.
    'perf_prefix',
    # colon-separated list of pnacl bcld flags, e.g. "-lfoo:-Wl,-u,bar".
    # Not using nacl_linkflags since that gets clobbered in some tests.
    'pnacl_bcldflags',
    'platform',
    # Run tests under this tool (e.g. valgrind, tsan, strace, etc).
    # If the tool has options, pass them after comma: 'tool,--opt1,--opt2'.
    # NB: no way to use tools the names or the args of
    # which contains a comma.
    'run_under',
    # More args for the tool.
    'run_under_extra_args',
    # Multiply timeout values by this number.
    'scale_timeout',
    # test_wrapper specifies a wrapper program such as
    # tools/run_test_via_ssh.py, which runs tests on a remote host
    # using rsync and SSH.  Example usage:
    #   ./scons run_hello_world_test platform=arm force_emulator= \
    #     test_wrapper="./tools/run_test_via_ssh.py --host=armbox --subdir=tmp"
    'test_wrapper',
    # Replacement tsan command for overriding the DEPS-in tsan
    # script.  May have commas to separate separate shell args.  There
    # is no quoting, so this implies that this mechanism will fail if
    # the args actually need to have commas.  See
    # http://code.google.com/p/nativeclient/issues/detail?id=3158 for
    # the discussion of why this argument is needed.
    'tsan_command',
    # Run browser tests under this tool. See
    # tools/browser_tester/browsertester/browserlauncher.py for tool names.
    'browser_test_tool',
    # Where to install header files for public consumption.
    'includedir',
    # Where to install libraries for public consumption.
    'libdir',
    # Where to install trusted-code binaries for public (SDK) consumption.
    'bindir',
    # Where a Breakpad build output directory is for optional Breakpad testing.
    'breakpad_tools_dir',
    # Allows overriding of the nacl newlib toolchain directory.
    'nacl_newlib_dir',
    # Allows override of the nacl glibc toolchain directory.
    'nacl_glibc_dir',
    # Allows override of the pnacl newlib toolchain directory.
    'pnacl_newlib_dir',
    # Allows overriding the version number in the toolchain's
    # FEATURE_VERSION file.  This is used for PNaCl ABI compatibility
    # testing.
    'toolchain_feature_version',
  ])


# Overly general to provide compatibility with existing build bots, etc.
# In the future it might be worth restricting the values that are accepted.
_TRUE_STRINGS = set(['1', 'true', 'yes'])
_FALSE_STRINGS = set(['0', 'false', 'no'])


# Converts a string representing a Boolean value, of some sort, into an actual
# Boolean value. Python's built in type coercion does not work because
# bool('False') == True
def StringValueToBoolean(value):
  # ExpandArguments may stick non-string values in ARGUMENTS. Be accommodating.
  if isinstance(value, bool):
    return value

  if not isinstance(value, basestring):
    raise Exception("Expecting a string but got a %s" % repr(type(value)))

  if value.lower() in _TRUE_STRINGS:
    return True
  elif value.lower() in _FALSE_STRINGS:
    return False
  else:
    raise Exception("Cannot convert '%s' to a Boolean value" % value)


def GetBinaryArgumentValue(arg_name, default):
  if not isinstance(default, bool):
    raise Exception("Default value for '%s' must be a Boolean" % arg_name)
  if arg_name not in ARGUMENTS:
    return default
  return StringValueToBoolean(ARGUMENTS[arg_name])


# name is the name of the bit
# arg_name is the name of the command-line argument, if it differs from the bit
def BitFromArgument(env, name, default, desc, arg_name=None):
  # In most cases the bit name matches the argument name
  if arg_name is None:
    arg_name = name

  DeclareBit(name, desc)
  assert arg_name not in ACCEPTABLE_ARGUMENTS, repr(arg_name)
  ACCEPTABLE_ARGUMENTS.add(arg_name)

  if GetBinaryArgumentValue(arg_name, default):
    env.SetBits(name)
  else:
    env.ClearBits(name)


# SetUpArgumentBits declares binary command-line arguments and converts them to
# bits. For example, one of the existing declarations would result in the
# argument "bitcode=1" causing env.Bit('bitcode') to evaluate to true.
# NOTE Command-line arguments are a SCons-ism that is separate from
# command-line options.  Options are prefixed by "-" or "--" whereas arguments
# are not.  The function SetBitFromOption can be used for options.
# NOTE This function must be called before the bits are used
# NOTE This function must be called after all modifications of ARGUMENTS have
# been performed. See: ExpandArguments
def SetUpArgumentBits(env):
  BitFromArgument(env, 'bitcode', default=False,
    desc='We are building bitcode')

  BitFromArgument(env, 'pnacl_native_clang_driver', default=False,
    desc='Use the (experimental) native PNaCl Clang driver')

  BitFromArgument(env, 'nacl_clang', default=(not env.Bit('bitcode') and
                                              not env.Bit('nacl_glibc')),
    desc='Use the native nacl-clang newlib compiler instead of nacl-gcc')

  BitFromArgument(env, 'translate_fast', default=False,
    desc='When using pnacl TC (bitcode=1) use accelerated translation step')

  BitFromArgument(env, 'use_sz', default=False,
    desc='When using pnacl TC (bitcode=1) use Subzero for fast translation')

  BitFromArgument(env, 'built_elsewhere', default=False,
    desc='The programs have already been built by another system')

  BitFromArgument(env, 'skip_trusted_tests', default=False,
    desc='Only run untrusted tests - useful for translator testing'
      ' (also skips tests of the IRT itself')

  BitFromArgument(env, 'nacl_pic', default=False,
    desc='generate position indepent code for (P)NaCl modules')

  BitFromArgument(env, 'nacl_static_link', default=not env.Bit('nacl_glibc'),
    desc='Whether to use static linking instead of dynamic linking '
      'for building NaCl executables during tests. '
      'For nacl-newlib, the default is 1 (static linking). '
      'For nacl-glibc, the default is 0 (dynamic linking).')

  BitFromArgument(env, 'nacl_disable_shared', default=not env.Bit('nacl_glibc'),
    desc='Do not build shared versions of libraries. '
      'For nacl-newlib, the default is 1 (static libraries only). '
      'For nacl-glibc, the default is 0 (both static and shared libraries).')

  # Defaults on when --verbose is specified.
  # --verbose sets 'brief_comstr' to False, so this looks a little strange
  BitFromArgument(env, 'target_stats', default=not GetOption('brief_comstr'),
    desc='Collect and display information about which commands are executed '
      'during the build process')

  BitFromArgument(env, 'werror', default=True,
    desc='Treat warnings as errors (-Werror)')

  BitFromArgument(env, 'disable_nosys_linker_warnings', default=False,
    desc='Disable warning mechanism in src/untrusted/nosys/warning.h')

  BitFromArgument(env, 'naclsdk_validate', default=True,
    desc='Verify the presence of the SDK')

  # TODO(mseaborn): Remove this, since this is always False -- Valgrind is
  # no longer supported.  This will require removing some Chromium-side
  # references.
  BitFromArgument(env, 'running_on_valgrind', default=False,
    desc='Compile and test using valgrind')

  BitFromArgument(env, 'pp', default=False,
    desc='Enable pretty printing')

  # Defaults on when --verbose is specified
  # --verbose sets 'brief_comstr' to False, so this looks a little strange
  BitFromArgument(env, 'sysinfo', default=not GetOption('brief_comstr'),
    desc='Print verbose system information')

  BitFromArgument(env, 'disable_flaky_tests', default=False,
    desc='Do not run potentially flaky tests - used on Chrome bots')

  BitFromArgument(env, 'use_sandboxed_translator', default=False,
    desc='use pnacl sandboxed translator for linking (not available for arm)')

  BitFromArgument(env, 'pnacl_generate_pexe', default=env.Bit('bitcode'),
    desc='use pnacl to generate pexes and translate in a separate step')

  BitFromArgument(env, 'translate_in_build_step', default=True,
    desc='Run translation during build phase (e.g. if do_not_run_tests=1)')

  BitFromArgument(env, 'pnacl_unsandboxed', default=False,
    desc='Translate pexe to an unsandboxed, host executable')

  BitFromArgument(env, 'nonsfi_nacl', default=False,
    desc='Use Non-SFI Mode instead of the original SFI Mode.  This uses '
      'nonsfi_loader instead of sel_ldr, and it tells the PNaCl toolchain '
      'to translate pexes to Non-SFI nexes.')

  BitFromArgument(env, 'use_newlib_nonsfi_loader', default=True,
    desc='Test nonsfi_loader linked against NaCl newlib instead of the one '
      'linked against host libc. This flag makes sense only with '
      'nonsfi_nacl=1.')

  BitFromArgument(env, 'browser_headless', default=False,
    desc='Where possible, set up a dummy display to run the browser on '
      'when running browser tests.  On Linux, this runs the browser through '
      'xvfb-run.  This Scons does not need to be run with an X11 display '
      'and we do not open a browser window on the user\'s desktop.  '
      'Unfortunately there is no equivalent on Mac OS X.')

  BitFromArgument(env, 'disable_crash_dialog', default=True,
    desc='Disable Windows\' crash dialog box, which Windows pops up when a '
      'process exits with an unhandled fault.  Windows enables this by '
      'default for processes launched from the command line or from the '
      'GUI.  Our default is to disable it, because the dialog turns crashes '
      'into hangs on Buildbot, and our test suite includes various crash '
      'tests.')

  BitFromArgument(env, 'do_not_run_tests', default=False,
    desc='Prevents tests from running.  This lets SCons build the files needed '
      'to run the specified test(s) without actually running them.  This '
      'argument is a counterpart to built_elsewhere.')

  BitFromArgument(env, 'no_gdb_tests', default=False,
    desc='Prevents GDB tests from running.  If GDB is not available, you can '
      'test everything else by specifying this flag.')

  # TODO(shcherbina): add support for other golden-based tests, not only
  # run_x86_*_validator_testdata_tests.
  BitFromArgument(env, 'regenerate_golden', default=False,
    desc='When running golden-based tests, instead of comparing results '
         'save actual output as golden data.')

  BitFromArgument(env, 'x86_64_zero_based_sandbox', default=False,
    desc='Use the zero-address-based x86-64 sandbox model instead of '
      'the r15-based model.')

  BitFromArgument(env, 'android', default=False,
                  desc='Build for Android target')

  BitFromArgument(env, 'skip_nonstable_bitcode', default=False,
                  desc='Skip tests involving non-stable bitcode')

  #########################################################################
  # EXPERIMENTAL
  # This is for generating a testing library for use within private test
  # enuminsts, where we want to compare and test different validators.
  #
  BitFromArgument(env, 'ncval_testing', default=False,
    desc='EXPERIMENTAL: Compile validator code for testing within enuminsts')

  # PNaCl sanity checks
  if not env.Bit('bitcode'):
    pnacl_only_flags = ('nonsfi_nacl',
                        'pnacl_generate_pexe',
                        'pnacl_unsandboxed',
                        'skip_nonstable_bitcode',
                        'translate_fast',
                        'use_sz',
                        'use_sandboxed_translator')

    for flag_name in pnacl_only_flags:
      if env.Bit(flag_name):
        raise UserError('The option %r only makes sense when using the '
                        'PNaCl toolchain (i.e. with bitcode=1)'
                        % flag_name)
  else:
    pnacl_incompatible_flags = ('nacl_clang',
                                'nacl_glibc')
    for flag_name in pnacl_incompatible_flags:
      if env.Bit(flag_name):
        raise UserError('The option %r cannot be used when building with '
                        'PNaCl (i.e. with bitcode=1)' % flag_name)

def CheckArguments():
  for key in ARGUMENTS:
    if key not in ACCEPTABLE_ARGUMENTS:
      raise UserError('bad argument: %s' % key)


def GetTargetPlatform():
  return pynacl.platform.GetArch3264(ARGUMENTS.get('platform', 'x86-32'))

def GetBuildPlatform():
  return pynacl.platform.GetArch3264()


environment_list = []

# Base environment for both nacl and non-nacl variants.
kwargs = {}
if ARGUMENTS.get('DESTINATION_ROOT') is not None:
  kwargs['DESTINATION_ROOT'] = ARGUMENTS.get('DESTINATION_ROOT')
pre_base_env = Environment(
    # Use the environment that scons was run in to run scons invoked commands.
    # This allows in things like externally provided PATH, PYTHONPATH.
    ENV = os.environ.copy(),
    tools = ['component_setup'],
    # SOURCE_ROOT is one leave above the native_client directory.
    SOURCE_ROOT = Dir('#/..').abspath,
    # Publish dlls as final products (to staging).
    COMPONENT_LIBRARY_PUBLISH = True,

    # Use workaround in special scons version.
    LIBS_STRICT = True,
    LIBS_DO_SUBST = True,

    # Select where to find coverage tools.
    COVERAGE_MCOV = '../third_party/lcov/bin/mcov',
    COVERAGE_GENHTML = '../third_party/lcov/bin/genhtml',
    **kwargs
)


if 'generate_ninja' in ARGUMENTS:
  import pynacl.scons_to_ninja
  pynacl.scons_to_ninja.GenerateNinjaFile(
      pre_base_env, dest_file=ARGUMENTS['generate_ninja'])


breakpad_tools_dir = ARGUMENTS.get('breakpad_tools_dir')
if breakpad_tools_dir is not None:
  pre_base_env['BREAKPAD_TOOLS_DIR'] = pre_base_env.Dir(
      os.path.abspath(breakpad_tools_dir))


# CLANG
DeclareBit('clang', 'Use clang to build trusted code')
pre_base_env.SetBitFromOption('clang', False)

DeclareBit('asan',
           'Use AddressSanitizer to build trusted code (implies --clang)')
pre_base_env.SetBitFromOption('asan', False)
if pre_base_env.Bit('asan'):
  pre_base_env.SetBits('clang')

DeclareBit('msan',
           'Use MemorySanitizer to build trusted code (implies --clang)')
pre_base_env.SetBitFromOption('msan', False)
if pre_base_env.Bit('msan'):
  pre_base_env.SetBits('clang')

# CODE COVERAGE
DeclareBit('coverage_enabled', 'The build should be instrumented to generate'
           'coverage information')

# If the environment variable BUILDBOT_BUILDERNAME is set, we can determine
# if we are running in a VM by the lack of a '-bare-' (aka bare metal) in the
# bot name.  Otherwise if the builder name is not set, then assume real HW.
DeclareBit('running_on_vm', 'Returns true when environment is running in a VM')
builder = os.environ.get('BUILDBOT_BUILDERNAME')
if builder and builder.find('-bare-') == -1:
  pre_base_env.SetBits('running_on_vm')
else:
  pre_base_env.ClearBits('running_on_vm')

DeclareBit('nacl_glibc', 'Use nacl-glibc for building untrusted code')
pre_base_env.SetBitFromOption('nacl_glibc', False)

# This function should be called ASAP after the environment is created, but
# after ExpandArguments.
SetUpArgumentBits(pre_base_env)

# Register PrintFinalReport only after SetUpArgumentBits since it references
# bits that get declared in SetUpArgumentBits
atexit.register(PrintFinalReport)

def DisableCrashDialog():
  if sys.platform == 'win32':
    import win32api
    import win32con
    # The double call is to preserve existing flags, as discussed at
    # http://blogs.msdn.com/oldnewthing/archive/2004/07/27/198410.aspx
    new_flags = win32con.SEM_NOGPFAULTERRORBOX
    existing_flags = win32api.SetErrorMode(new_flags)
    win32api.SetErrorMode(existing_flags | new_flags)

if pre_base_env.Bit('disable_crash_dialog'):
  DisableCrashDialog()

# We want to pull CYGWIN setup in our environment or at least set flag
# nodosfilewarning. It does not do anything when CYGWIN is not involved
# so let's do it in all cases.
pre_base_env['ENV']['CYGWIN'] = os.environ.get('CYGWIN', 'nodosfilewarning')

# Note: QEMU_PREFIX_HOOK may influence test runs and sb translator invocations
pre_base_env['ENV']['QEMU_PREFIX_HOOK'] = os.environ.get('QEMU_PREFIX_HOOK', '')

# Allow the zero-based sandbox model to run insecurely.
# TODO(arbenson): remove this once binutils bug is fixed (see
# src/trusted/service_runtime/arch/x86_64/sel_addrspace_posix_x86_64.c)
if pre_base_env.Bit('x86_64_zero_based_sandbox'):
  pre_base_env['ENV']['NACL_ENABLE_INSECURE_ZERO_BASED_SANDBOX'] = 1

if pre_base_env.Bit('werror'):
  werror_flags = ['-Werror']
else:
  werror_flags = []

# Allow variadic macros
werror_flags = werror_flags + ['-Wno-variadic-macros']

if pre_base_env.Bit('clang'):
  # Allow 'default' label in switch even when all enumeration cases
  # have been covered.
  werror_flags += ['-Wno-covered-switch-default']
  # Allow C++11 extensions (for "override")
  werror_flags += ['-Wno-c++11-extensions']


# Method to make sure -pedantic, etc, are not stripped from the
# default env, since occasionally an engineer will be tempted down the
# dark -- but wide and well-trodden -- path of expediency and stray
# from the path of correctness.

def EnsureRequiredBuildWarnings(env):
  if (env.Bit('linux') or env.Bit('mac')) and not env.Bit('android'):
    required_env_flags = set(['-pedantic', '-Wall'] + werror_flags)
    ccflags = set(env.get('CCFLAGS'))

    if not required_env_flags.issubset(ccflags):
      raise UserError('required build flags missing: '
                      + ' '.join(required_env_flags.difference(ccflags)))
  else:
    # windows get a pass for now
    pass

pre_base_env.AddMethod(EnsureRequiredBuildWarnings)

# Expose MakeTempDir and MakeTempFile to scons scripts
def MakeEmptyFile(env, **kwargs):
  fd, path = test_lib.MakeTempFile(env, **kwargs)
  os.close(fd)
  return path

pre_base_env.AddMethod(test_lib.MakeTempDir)
pre_base_env.AddMethod(MakeEmptyFile)

# Method to add target suffix to name.
def NaClTargetArchSuffix(env, name):
  return name + '_' + env['TARGET_FULLARCH'].replace('-', '_')

pre_base_env.AddMethod(NaClTargetArchSuffix)


# Generic Test Wrapper

# Add list of Flaky or Bad tests to skip per platform.  A
# platform is defined as build type
# <BUILD_TYPE>-<SUBARCH>
bad_build_lists = {
    'arm': [],
}

# This is a list of tests that do not yet pass when using nacl-glibc.
# TODO(mseaborn): Enable more of these tests!
nacl_glibc_skiplist = set([
    # Struct layouts differ.
    'run_abi_test',
    # Syscall wrappers not implemented yet.
    'run_sysbasic_test',
    'run_sysbrk_test',
    # Fails because clock() is not hooked up.
    'run_timefuncs_test',
    # Needs further investigation.
    'sdk_minimal_test',
    # This test fails with nacl-glibc: glibc reports an internal
    # sanity check failure in free().
    # TODO(robertm): This needs further investigation.
    'run_ppapi_event_test',
    'run_ppapi_geturl_valid_test',
    'run_ppapi_geturl_invalid_test',
    # http://code.google.com/p/chromium/issues/detail?id=108131
    # we would need to list all of the glibc components as
    # web accessible resources in the extensions's manifest.json,
    # not just the nexe and nmf file.
    'run_ppapi_extension_mime_handler_browser_test',
    ])
nacl_glibc_skiplist.update(['%s_irt' % test for test in nacl_glibc_skiplist])

# Whitelist of tests to run for Non-SFI Mode.  Note that typos here will
# not be caught automatically!
# TODO(mseaborn): Eventually we should run all of small_tests instead of
# this whitelist.
nonsfi_test_whitelist = set([
    'run_arm_float_abi_test',
    'run_clock_get_test',
    'run_clone_test',
    'run_directory_test',
    'run_dup_test',
    'run_example_irt_caller_test',
    'run_exception_test',
    'run_fcntl_test',
    'run_file_descriptor_test',
    'run_float_test',
    'run_fork_test',
    'run_getpid_test',
    'run_hello_world_test',
    'run_icache_test',
    'run_irt_futex_test',
    'run_malloc_realloc_calloc_free_test',
    'run_mmap_test',
    'run_nanosleep_test',
    'run_nonsfi_syscall_test',
    'run_prctl_test',
    'run_printf_test',
    'run_pwrite_test',
    'run_random_test',
    'run_rlimit_test',
    'run_sigaction_test',
    'run_signal_sigbus_test',
    'run_signal_test',
    'run_socket_test',
    'run_stack_alignment_asm_test',
    'run_stack_alignment_test',
    'run_syscall_test',
    'run_thread_test',
    'run_user_async_signal_test',
    ])


# If a test is not in one of these suites, it will probally not be run on a
# regular basis.  These are the suites that will be run by the try bot or that
# a large number of users may run by hand.
MAJOR_TEST_SUITES = set([
  'small_tests',
  'medium_tests',
  'large_tests',
  # Tests using the pepper plugin, only run with chrome
  # TODO(ncbray): migrate pepper_browser_tests to chrome_browser_tests
  'pepper_browser_tests',
  # Lightweight browser tests
  'chrome_browser_tests',
  'huge_tests',
  'memcheck_bot_tests',
  'tsan_bot_tests',
  # Special testing environment for testing comparing x86 validators.
  'ncval_testing',
  # Environment for validator difference testing
  'validator_diff_tests',
  # Subset of tests enabled for Non-SFI Mode.
  'nonsfi_tests',
])

# These are the test suites we know exist, but aren't run on a regular basis.
# These test suites are essentially shortcuts that run a specific subset of the
# test cases.
ACCEPTABLE_TEST_SUITES = set([
  'barebones_tests',
  'dynamic_load_tests',
  'eh_tests',  # Tests for C++ exception handling
  'exception_tests',  # Tests for hardware exception handling
  'exit_status_tests',
  'gdb_tests',
  'mmap_race_tests',
  'nonpexe_tests',
  'pnacl_abi_tests',
  'sel_ldr_sled_tests',
  'sel_ldr_tests',
  'toolchain_tests',
  'validator_modeling',
  'validator_tests',
  # Special testing of the decoder for the ARM validator.
  'arm_decoder_tests',
])

# Under --mode=nacl_irt_test we build variants of numerous tests normally
# built under --mode=nacl.  The test names and suite names for these
# variants are set (in IrtTestAddNodeToTestSuite, below) by appending _irt
# to the names used for the --mode=nacl version of the same tests.
MAJOR_TEST_SUITES |= set([name + '_irt'
                          for name in MAJOR_TEST_SUITES])
ACCEPTABLE_TEST_SUITES |= set([name + '_irt'
                               for name in ACCEPTABLE_TEST_SUITES])

# The major test suites are also acceptable names.  Suite names are checked
# against this set in order to catch typos.
ACCEPTABLE_TEST_SUITES.update(MAJOR_TEST_SUITES)


def ValidateTestSuiteNames(suite_name, node_name):
  if node_name is None:
    node_name = '<unknown>'

  # Prevent a silent failiure - strings are iterable!
  if not isinstance(suite_name, (list, tuple)):
    raise Exception("Test suites for %s should be specified as a list, "
      "not as a %s: %s" % (node_name, type(suite_name).__name__,
      repr(suite_name)))

  if not suite_name:
    raise Exception("No test suites are specified for %s. Set the 'broken' "
      "parameter on AddNodeToTestSuite in the cases where there's a known "
      "issue and you don't want the test to run" % (node_name,))

  # Make sure each test is in at least one test suite we know will run
  major_suites = set(suite_name).intersection(MAJOR_TEST_SUITES)
  if not major_suites:
    raise Exception("None of the test suites %s for %s are run on a "
    "regular basis" % (repr(suite_name), node_name))

  # Make sure a wierd test suite hasn't been inadvertantly specified
  for s in suite_name:
    if s not in ACCEPTABLE_TEST_SUITES:
      raise Exception("\"%s\" is not a known test suite. Either this is "
      "a typo for %s, or it should be added to ACCEPTABLE_TEST_SUITES in "
      "SConstruct" % (s, node_name))

BROKEN_TEST_COUNT = 0


def GetPlatformString(env):
  build = env['BUILD_TYPE']

  # If we are testing 'NACL' we really need the trusted info
  if build=='nacl' and 'TRUSTED_ENV' in env:
    trusted_env = env['TRUSTED_ENV']
    build = trusted_env['BUILD_TYPE']
    subarch = trusted_env['BUILD_SUBARCH']
  else:
    subarch = env['BUILD_SUBARCH']

  # Build the test platform string
  return build + '-' + subarch

pre_base_env.AddMethod(GetPlatformString)


tests_to_disable_qemu = set([
    # These tests do not work under QEMU but do work on ARM hardware.
    #
    # You should use the is_broken argument in preference to adding
    # tests to this list.
    #
    # See: http://code.google.com/p/nativeclient/issues/detail?id=2437
    # Note, for now these tests disable both the irt and non-irt variants
    'run_egyptian_cotton_test',
    'run_many_threads_sequential_test',
    # subprocess needs to also have qemu prefix, which isn't supported
    'run_subprocess_test',
    'run_thread_suspension_test',
    'run_dynamic_modify_test',
])

tests_to_disable = set()
if ARGUMENTS.get('disable_tests', '') != '':
  tests_to_disable.update(ARGUMENTS['disable_tests'].split(','))


def ShouldSkipTest(env, node_name):
  if (env.Bit('skip_trusted_tests')
      and (env['NACL_BUILD_FAMILY'] == 'TRUSTED'
           or env['NACL_BUILD_FAMILY'] == 'UNTRUSTED_IRT')):
    return True

  if env.Bit('do_not_run_tests'):
    # This hack is used for pnacl testing where we might build tests
    # without running them on one bot and then transfer and run them on another.
    # The skip logic only takes the first bot into account e.g. qemu
    # restrictions, while it really should be skipping based on the second
    # bot. By simply disabling the skipping completely we work around this.
    return False

  # There are no known-to-fail tests any more, but this code is left
  # in so that if/when we port to a new architecture or add a test
  # that is known to fail on some platform(s), we can continue to have
  # a central location to disable tests from running.  NB: tests that
  # don't *build* on some platforms need to be omitted in another way.

  if node_name in tests_to_disable:
    return True

  if env.UsingEmulator():
    if node_name in tests_to_disable_qemu:
      return True
    # For now also disable the irt variant
    if node_name.endswith('_irt') and node_name[:-4] in tests_to_disable_qemu:
      return True

  # Retrieve list of tests to skip on this platform
  skiplist = bad_build_lists.get(env.GetPlatformString(), [])
  if node_name in skiplist:
    return True

  if env.Bit('nacl_glibc') and node_name in nacl_glibc_skiplist:
    return True

  return False

pre_base_env.AddMethod(ShouldSkipTest)


def AddImplicitTestSuites(suite_list, node_name):
  if node_name in nonsfi_test_whitelist:
    suite_list = suite_list + ['nonsfi_tests']
  return suite_list


def AddNodeToTestSuite(env, node, suite_name, node_name, is_broken=False,
                       is_flaky=False):
  global BROKEN_TEST_COUNT

  # CommandTest can return an empty list when it silently discards a test
  if not node:
    return

  assert node_name is not None
  test_name_regex = r'run_.*_(unit)?test.*$'
  assert re.match(test_name_regex, node_name), (
      'test %r does not match "run_..._test" naming convention '
      '(precise regex is %s)' % (node_name, test_name_regex))

  suite_name = AddImplicitTestSuites(suite_name, node_name)
  ValidateTestSuiteNames(suite_name, node_name)

  AlwaysBuild(node)

  if is_broken or is_flaky and env.Bit('disable_flaky_tests'):
    # Only print if --verbose is specified
    if not GetOption('brief_comstr'):
      print '*** BROKEN ', node_name
    BROKEN_TEST_COUNT += 1
    env.Alias('broken_tests', node)
  elif env.ShouldSkipTest(node_name):
    print '*** SKIPPING ', env.GetPlatformString(), ':', node_name
    env.Alias('broken_tests', node)
  else:
    env.Alias('all_tests', node)

    for s in suite_name:
      env.Alias(s, node)

  if node_name:
    env.ComponentTestOutput(node_name, node)
    test_name = node_name
  else:
    # This is rather shady, but the tests need a name without dots so they match
    # what gtest does.
    # TODO(ncbray) node_name should not be optional.
    test_name = os.path.basename(str(node[0].path))
    if test_name.endswith('.out'):
      test_name = test_name[:-4]
    test_name = test_name.replace('.', '_')
  SetTestName(node, test_name)

pre_base_env.AddMethod(AddNodeToTestSuite)


def TestBindsFixedTcpPort(env, node):
  # This tells Scons that tests that bind a fixed TCP port should not
  # run concurrently, because they would interfere with each other.
  # These tests are typically tests for NaCl's GDB debug stub.  The
  # dummy filename used below is an arbitrary token that just has to
  # match across the tests.
  SideEffect(env.File('${SCONSTRUCT_DIR}/test_binds_fixed_tcp_port'), node)

pre_base_env.AddMethod(TestBindsFixedTcpPort)


# Convenient testing aliases
# NOTE: work around for scons non-determinism in the following two lines
Alias('sel_ldr_sled_tests', [])

Alias('small_tests', [])
Alias('medium_tests', [])
Alias('large_tests', [])

Alias('small_tests_irt', [])
Alias('medium_tests_irt', [])
Alias('large_tests_irt', [])

Alias('pepper_browser_tests', [])
Alias('chrome_browser_tests', [])

Alias('unit_tests', 'small_tests')
Alias('smoke_tests', ['small_tests', 'medium_tests'])

if pre_base_env.Bit('nacl_glibc'):
  Alias('memcheck_bot_tests', ['small_tests'])
  Alias('tsan_bot_tests', ['small_tests'])
else:
  Alias('memcheck_bot_tests', ['small_tests', 'medium_tests', 'large_tests'])
  Alias('tsan_bot_tests', [])


def Banner(text):
  print '=' * 70
  print text
  print '=' * 70

pre_base_env.AddMethod(Banner)


# PLATFORM LOGIC
# Define the platforms, and use them to define the path for the
# scons-out directory (aka TARGET_ROOT)
#
# Various variables in the scons environment are related to this, e.g.
#
# BUILD_ARCH: (arm, mips, x86)
# BUILD_SUBARCH: (32, 64)
#

DeclareBit('build_x86_32', 'Building binaries for the x86-32 architecture',
           exclusive_groups='build_arch')
DeclareBit('build_x86_64', 'Building binaries for the x86-64 architecture',
           exclusive_groups='build_arch')
DeclareBit('build_mips32', 'Building binaries for the MIPS architecture',
           exclusive_groups='build_arch')
DeclareBit('build_arm_arm', 'Building binaries for the ARM architecture',
           exclusive_groups='build_arch')

# Shorthand for either the 32 or 64 bit version of x86.
DeclareBit('build_x86', 'Building binaries for the x86 architecture')

DeclareBit('build_arm', 'Building binaries for the arm architecture')


def MakeArchSpecificEnv(platform=None):
  env = pre_base_env.Clone()
  if platform is None:
    platform = GetTargetPlatform()

  arch = pynacl.platform.GetArch(platform)
  if pynacl.platform.IsArch64Bit(platform):
    subarch = '64'
  else:
    subarch = '32'

  env.Replace(BUILD_FULLARCH=platform)
  env.Replace(BUILD_ARCHITECTURE=arch)
  env.Replace(BUILD_SUBARCH=subarch)
  env.Replace(TARGET_FULLARCH=platform)
  env.Replace(TARGET_ARCHITECTURE=arch)
  env.Replace(TARGET_SUBARCH=subarch)

  env.SetBits('build_%s' % platform.replace('-', '_'))

  if env.Bit('build_x86_32') or env.Bit('build_x86_64'):
    env.SetBits('build_x86')
  if env.Bit('build_arm_arm'):
    env.SetBits('build_arm')

  env.Replace(BUILD_ISA_NAME=platform)

  # Determine where the object files go
  env.Replace(BUILD_TARGET_NAME=platform)
  # This may be changed later; see target_variant_map, below.
  env.Replace(TARGET_VARIANT='')
  env.Replace(TARGET_ROOT=
      '${DESTINATION_ROOT}/${BUILD_TYPE}-${BUILD_TARGET_NAME}${TARGET_VARIANT}')
  return env


# Valgrind
pre_base_env.AddMethod(lambda self: ARGUMENTS.get('running_on_valgrind'),
                       'IsRunningUnderValgrind')

DeclareBit('with_leakcheck', 'Running under Valgrind leak checker')

def RunningUnderLeakCheck():
  run_under = ARGUMENTS.get('run_under')
  if run_under:
    extra_args = ARGUMENTS.get('run_under_extra_args')
    if extra_args:
      run_under += extra_args
    if run_under.find('leak-check=full') > 0:
      return True
  return False

if RunningUnderLeakCheck():
  pre_base_env.SetBits('with_leakcheck')


def StripSuffix(string, suffix):
  assert string.endswith(suffix)
  return string[:-len(suffix)]


# TODO(mseaborn): Change code to use ComponentLibrary() directly.
# DualLibrary() is left over from when we built libraries twice, with and
# without "-fPIC", for building plugins as DSOs.
def DualLibrary(env, lib_name, *args, **kwargs):
  return env.ComponentLibrary(lib_name, *args, **kwargs)

def DualObject(env, *args, **kwargs):
  return env.ComponentObject(*args, **kwargs)


def AddDualLibrary(env):
  env.AddMethod(DualLibrary)
  env.AddMethod(DualObject)


# In prebuild mode we ignore the dependencies so that stuff does
# NOT get build again
# Optionally ignore the build process.
DeclareBit('prebuilt', 'Disable all build steps, only support install steps')
pre_base_env.SetBitFromOption('prebuilt', False)


# HELPERS FOR TEST INVOLVING TRUSTED AND UNTRUSTED ENV
def GetEmulator(env):
  emulator = ARGUMENTS.get('force_emulator')
  if emulator is None and 'TRUSTED_ENV' in env:
    emulator = env['TRUSTED_ENV'].get('EMULATOR')
  return emulator

pre_base_env.AddMethod(GetEmulator)

def UsingEmulator(env):
  return bool(env.GetEmulator())

pre_base_env.AddMethod(UsingEmulator)


def GetValidator(env, validator):
  # NOTE: that the variable TRUSTED_ENV is set by ExportSpecialFamilyVars()
  if 'TRUSTED_ENV' not in env:
    return None

  # TODO(shyamsundarr): rename ncval_new to ncval.
  if validator is None:
    if env.Bit('build_arm'):
      validator = 'arm-ncval-core'
    elif env.Bit('build_mips32'):
      validator = 'mips-ncval-core'
    else:
      validator = 'ncval_new'

  trusted_env = env['TRUSTED_ENV']
  return trusted_env.File('${STAGING_DIR}/${PROGPREFIX}%s${PROGSUFFIX}' %
                    validator)

pre_base_env.AddMethod(GetValidator)


# Perform os.path.abspath rooted at the directory SConstruct resides in.
def SConstructAbsPath(env, path):
  return os.path.normpath(os.path.join(env['MAIN_DIR'], path))

pre_base_env.AddMethod(SConstructAbsPath)


def GetPlatformBuildTargetDir(env):
  # Currently we do not support any cross OS compiles, eventually the OS name
  # will probably be passed in through arguments.
  os_name = pynacl.platform.GetOS()

  # Currently 32/64 share the same tool build target directory. When we have
  # separate toolchains for each the architectures will probably have to use
  # the Arch3264() variant.
  build_arch = pynacl.platform.GetArch(GetBuildPlatform())

  return '%s_%s' % (os_name, build_arch)

pre_base_env.AddMethod(GetPlatformBuildTargetDir)


def GetToolchainDir(env, platform_build_dir=None, toolchain_name=None,
                    target_arch=None, is_pnacl=None, lib_name=None):
  if platform_build_dir is None:
    platform_build_dir = env.GetPlatformBuildTargetDir()

  if toolchain_name is None:
    # Fill in default arguments based on environment.
    if is_pnacl is None:
      # For the purposes of finding the toolchain dir, nacl_clang is PNaCl.
      is_pnacl = env.Bit('bitcode') or env.Bit('nacl_clang')
      if lib_name is None:
        if is_pnacl or not env.Bit('nacl_glibc'):
          lib_name = 'newlib'
        else:
          lib_name = 'glibc'

    if target_arch is None:
      target_arch = pynacl.platform.GetArch(GetTargetPlatform())

    if is_pnacl:
      target_env = 'pnacl'
    else:
      target_env = 'nacl_%s' % target_arch

    # See if we have a custom toolchain directory set.
    if is_pnacl:
      toolchain_arg = 'pnacl_%s_dir' % lib_name
    else:
      assert lib_name == 'glibc'
      toolchain_arg = 'nacl_%s_dir' % lib_name

    custom_toolchain_dir = ARGUMENTS.get(toolchain_arg, None)
    if custom_toolchain_dir:
      return env.SConstructAbsPath(custom_toolchain_dir)

    # Get the standard toolchain name since no directory custom was found.
    if is_pnacl:
      target_env = 'pnacl'
    else:
      target_env = 'nacl_%s' % target_arch
    toolchain_name = '%s_%s_raw' % (target_env, lib_name)

  # Get the absolute path for the platform build directory and toolchain.
  toolchain_sub_dir = os.path.join('toolchain',
                                   platform_build_dir,
                                   toolchain_name)
  return env.SConstructAbsPath(toolchain_sub_dir)

pre_base_env.AddMethod(GetToolchainDir)


def GetSelLdr(env):
  sel_ldr = ARGUMENTS.get('force_sel_ldr')
  if sel_ldr:
    return env.File(env.SConstructAbsPath(sel_ldr))

  # NOTE: that the variable TRUSTED_ENV is set by ExportSpecialFamilyVars()
  if 'TRUSTED_ENV' not in env:
    return None

  trusted_env = env['TRUSTED_ENV']
  return trusted_env.File('${STAGING_DIR}/${PROGPREFIX}sel_ldr${PROGSUFFIX}')

pre_base_env.AddMethod(GetSelLdr)


def GetSelLdrSeccomp(env):
  # NOTE: that the variable TRUSTED_ENV is set by ExportSpecialFamilyVars()
  if 'TRUSTED_ENV' not in env:
    return None

  if not (env.Bit('linux') and env.Bit('build_x86_64')):
    return None

  trusted_env = env['TRUSTED_ENV']
  return trusted_env.File('${STAGING_DIR}/${PROGPREFIX}'
                          'sel_ldr_seccomp${PROGSUFFIX}')

pre_base_env.AddMethod(GetSelLdrSeccomp)


def SupportsSeccompBpfSandbox(env):
  if not (env.Bit('linux') and env.Bit('build_x86_64')):
    return False

  # The gcov runtime does some extra calls (such as 'access') that
  # are not permitted by the policy.
  if env.Bit('coverage_enabled'):
    return False

  # This is a lame detection if seccomp bpf filters are supported by the kernel.
  # We suppose that any Linux kernel v3.2+ supports it, but it is only true
  # for Ubuntu kernels. Seccomp BPF filters reached the mainline at 3.5,
  # so this check will be wrong on some relatively old non-Ubuntu Linux distros.
  kernel_version = map(int, platform.release().split('.', 2)[:2])
  return kernel_version >= [3, 2]

pre_base_env.AddMethod(SupportsSeccompBpfSandbox)


def GetBootstrap(env):
  if env.Bit('msan'):
    # Bootstrap doens't currently work with MSan. However, MSan is only
    # available on x86_64 where we don't need bootstrap anyway.
    return None, None
  bootstrap = ARGUMENTS.get('force_bootstrap')
  if bootstrap:
    bootstrap = env.File(env.SConstructAbsPath(bootstrap))
  else:
    if 'TRUSTED_ENV' in env:
      trusted_env = env['TRUSTED_ENV']
      if trusted_env.Bit('linux'):
        bootstrap = trusted_env.File('${STAGING_DIR}/nacl_helper_bootstrap')
  if bootstrap:
    template_digits = 'X' * 16
    return (bootstrap,
            ['--r_debug=0x' + template_digits,
             '--reserved_at_zero=0x' + template_digits])
  return None, None

pre_base_env.AddMethod(GetBootstrap)

def AddBootstrap(env, executable, args):
  bootstrap, bootstrap_args = env.GetBootstrap()
  if bootstrap is None:
    return [executable] + args
  else:
    return [bootstrap, executable] + bootstrap_args + args

pre_base_env.AddMethod(AddBootstrap)


def GetNonSfiLoader(env):
  if env.Bit('use_newlib_nonsfi_loader'):
    return nacl_env.GetTranslatedNexe(nacl_env.File(
        '${STAGING_DIR}/${PROGPREFIX}nonsfi_loader${PROGSUFFIX}'))

  if 'TRUSTED_ENV' not in env:
    return None
  return env['TRUSTED_ENV'].File(
      '${STAGING_DIR}/${PROGPREFIX}nonsfi_loader${PROGSUFFIX}')

pre_base_env.AddMethod(GetNonSfiLoader)


def GetIrtNexe(env, chrome_irt=False):
  image = ARGUMENTS.get('force_irt')
  if image:
    return env.SConstructAbsPath(image)

  if chrome_irt:
    return nacl_irt_env.File('${STAGING_DIR}/irt.nexe')
  else:
    return nacl_irt_env.File('${STAGING_DIR}/irt_core.nexe')

pre_base_env.AddMethod(GetIrtNexe)


# Note that we build elf_loader in the nacl_irt_env, not because it is
# actually built like the IRT per se, but just because we need it always to
# be built against newlib.
def GetElfLoaderNexe(env):
  elf_loader_env = nacl_env
  if env.Bit('nacl_glibc'):
    elf_loader_env = nacl_irt_env
  return elf_loader_env.File('${STAGING_DIR}/elf_loader.nexe')

pre_base_env.AddMethod(GetElfLoaderNexe)


def ApplyTLSEdit(env, nexe_name, raw_nexe):
  # If the environment was built elsewhere, we do not need to apply tls_edit
  # since it only needs to be done during building.
  if env.Bit('built_elsewhere'):
    return env.File(nexe_name)

  tls_edit_exe = env['BUILD_ENV'].File('${STAGING_DIR}/tls_edit${PROGSUFFIX}')
  return env.Command(
      nexe_name,
      [tls_edit_exe, raw_nexe],
      '${SOURCES[0]} --verbose ${SOURCES[1:]} ${TARGET}')

pre_base_env.AddMethod(ApplyTLSEdit)

def CommandValidatorTestNacl(env, name, image,
                             validator_flags=None,
                             validator=None,
                             size='medium',
                             **extra):
  validator = env.GetValidator(validator)
  if validator is None:
    print 'WARNING: no validator found. Skipping test %s' % name
    return []

  if validator_flags is None:
    validator_flags = []

  if env.Bit('pnacl_generate_pexe'):
    return []

  command = [validator] + validator_flags + [image]
  return env.CommandTest(name, command, size, **extra)

pre_base_env.AddMethod(CommandValidatorTestNacl)


def ExtractPublishedFiles(env, target_name):
  run_files = ['$STAGING_DIR/' + os.path.basename(published_file.path)
               for published_file in env.GetPublished(target_name, 'run')]
  nexe = '$STAGING_DIR/%s${PROGSUFFIX}' % target_name
  return [env.File(file) for file in run_files + [nexe]]

pre_base_env.AddMethod(ExtractPublishedFiles)


# Only include the chrome side of the build if present.
if os.path.exists(pre_base_env.File(
    '#/../ppapi/native_client/chrome_main.scons').abspath):
  SConscript('#/../ppapi/native_client/chrome_main.scons',
      exports=['pre_base_env'])
  enable_chrome = True
else:
  def AddChromeFilesFromGroup(env, file_group):
    pass
  pre_base_env.AddMethod(AddChromeFilesFromGroup)
  enable_chrome = False
DeclareBit('enable_chrome_side',
           'Is the chrome side present.')
pre_base_env.SetBitFromOption('enable_chrome_side', enable_chrome)

def ProgramNameForNmf(env, basename):
  """ Create an architecture-specific filename that can be used in an NMF URL.
  """
  if env.Bit('pnacl_generate_pexe'):
    return basename
  else:
    return '%s_%s' % (basename, env.get('TARGET_FULLARCH'))

pre_base_env.AddMethod(ProgramNameForNmf)


def MakeNaClLogOption(env, target):
  """ Make up a filename related to the [target], for use with NACLLOG.
  The file should end up in the build directory (scons-out/...).
  """
  # NOTE: to log to the source directory use file.srcnode().abspath instead.
  # See http://www.scons.org/wiki/File%28%29
  return env.File(target + '.nacllog').abspath

pre_base_env.AddMethod(MakeNaClLogOption)

def MakeVerboseExtraOptions(env, target, log_verbosity, extra):
  """ Generates **extra options that will give access to service runtime logs,
  at a given log_verbosity. Slips the options into the given extra dict. """
  log_file = env.MakeNaClLogOption(target)
  extra['log_file'] = log_file
  extra_env = ['NACLLOG=%s' % log_file,
               'NACLVERBOSITY=%d' % log_verbosity]
  extra['osenv'] = extra.get('osenv', []) + extra_env

pre_base_env.AddMethod(MakeVerboseExtraOptions)

def ShouldUseVerboseOptions(env, extra):
  """ Heuristic for setting up Verbose NACLLOG options. """
  return ('process_output_single' in extra or
          'log_golden' in extra)

pre_base_env.AddMethod(ShouldUseVerboseOptions)


DeclareBit('tests_use_irt', 'Non-browser tests also load the IRT image', False)

# Bit to be set by individual test/nacl.scons files that need to opt out.
DeclareBit('nonstable_bitcode', 'Tests use non-stable bitcode features', False)


def GetFinalizedPexe(env, pexe):
  """ Prep and finalize the ABI for a given pexe if needed.
  """
  if not env.Bit('pnacl_generate_pexe') or env.Bit('nonstable_bitcode'):
    return pexe

  # We can remove this once we move all CommandSelLdrTestNacl to a nacl.scons
  # file instead.  There are currently some canned nexe tests in build.scons.
  if env['NACL_BUILD_FAMILY'] == 'TRUSTED':
    return pexe

  # Otherwise, finalize during the build step, since there is no finalize tool
  # that can run on triggered bots such as the ARM HW bots.
  pexe_name = pexe.abspath
  final_name = StripSuffix(pexe_name, '.nonfinal.pexe') + '.final.pexe'
  # Make sure the pexe doesn't get removed by the fake builders when
  # built_elsewhere=1
  env.Precious(pexe)
  node = env.Command(target=final_name, source=[pexe_name],
                     action=[Action('${PNACLFINALIZECOM}',
                                    '${PNACLFINALIZECOMSTR}')])
  env.Alias('all_programs', node)
  assert len(node) == 1, node
  return node[0]

pre_base_env.AddMethod(GetFinalizedPexe)


# Translate the given pexe.
def GetTranslatedNexe(env, pexe):
  # First finalize the pexe.
  pexe = GetFinalizedPexe(env, pexe)

  # Then check if we need to translate.
  # Check if we started with a pexe, so there is actually a translation step.
  if not env.Bit('pnacl_generate_pexe'):
    return pexe

  # We can remove this once we move all CommandSelLdrTestNacl to a nacl.scons
  # file instead.  There are currently some canned nexe tests in build.scons.
  if env['NACL_BUILD_FAMILY'] == 'TRUSTED':
    return pexe

  # Often there is a build step (do_not_run_tests=1) and a test step
  # (which is run with -j1). Normally we want to translate in the build step
  # so we can translate in parallel. However when we do sandboxed translation
  # on arm hw, we do the build step on x86 and translation on arm, so we have
  # to force the translation to be done in the test step. Hence,
  # we check the bit 'translate_in_build_step' / check if we are
  # in the test step.
  if not env.Bit('translate_in_build_step') and env.Bit('do_not_run_tests'):
    return pexe

  pexe_name = pexe.abspath
  # Tidy up the suffix (remove the .final.pexe or .nonfinal.pexe),
  # depending on whether or not the pexe was finalized.
  suffix_to_strip = '.final.pexe'
  if not pexe_name.endswith(suffix_to_strip):
    suffix_to_strip = '.nonfinal.pexe'
  nexe_name = StripSuffix(pexe_name, suffix_to_strip) + '.nexe'
  # Make sure the pexe doesn't get removed by the fake builders when
  # built_elsewhere=1
  env.Precious(pexe)
  command = '${TRANSLATECOM}'
  if env.Bit('nonstable_bitcode'):
    command += ' --allow-llvm-bitcode-input'
  node = env.Command(target=nexe_name, source=[pexe_name],
                     action=[Action(command, '${TRANSLATECOMSTR}')])
  env.Alias('all_programs', node)
  assert len(node) == 1, node
  return node[0]

pre_base_env.AddMethod(GetTranslatedNexe)


def CommandTestFileDumpCheck(env,
                             name,
                             target,
                             check_file,
                             objdump_flags):
  """Create a test that disassembles a binary (|target|) and checks for
  patterns in the |check_file|.  Disassembly is done using |objdump_flags|.
  """

  # Do not try to run OBJDUMP if 'built_elsewhere', since that *might* mean
  # that a toolchain is not even present.  E.g., the arm hw buildbots do
  # not have the pnacl toolchain. We should be able to look for the host
  # ARM objdump though... a TODO(jvoung) for when there is time.
  if env.Bit('built_elsewhere'):
    return []
  target = env.GetTranslatedNexe(target)
  return env.CommandTestFileCheck(name,
                                  ['${OBJDUMP}', objdump_flags, target],
                                  check_file)

pre_base_env.AddMethod(CommandTestFileDumpCheck)

def CommandTestFileCheck(env, name, cmd, check_file):
  """Create a test that runs a |cmd| (array of strings),
  which is expected to print to stdout.  The results
  of stdout will then be piped to the file_check.py tool which
  will search for the regexes specified in |check_file|. """

  return env.CommandTest(
          name,
          ['${PYTHON}',
          env.File('${SCONSTRUCT_DIR}/tools/llvm_file_check_wrapper.py'),
          '${FILECHECK}',
          check_file] + cmd,
          direct_emulation=False)

pre_base_env.AddMethod(CommandTestFileCheck)

def CommandSelLdrTestNacl(env, name, nexe,
                          args = None,
                          log_verbosity=2,
                          sel_ldr_flags=None,
                          loader=None,
                          size='medium',
                          # True for *.nexe statically linked with glibc
                          glibc_static=False,
                          skip_bootstrap=False,
                          wrapper_program_prefix=None,
                          # e.g., [ 'python', 'time_check.py', '--' ]
                          **extra):
  # Disable all sel_ldr tests for windows under coverage.
  # Currently several .S files block sel_ldr from being instrumented.
  # See http://code.google.com/p/nativeclient/issues/detail?id=831
  if ('TRUSTED_ENV' in env and
      env['TRUSTED_ENV'].Bit('coverage_enabled') and
      env['TRUSTED_ENV'].Bit('windows')):
    return []

  # The nexe might be a pexe that needs finalization, and translation.
  nexe = env.GetTranslatedNexe(nexe)

  command = [nexe]
  if args is not None:
    command += args

  if env.Bit('pnacl_unsandboxed') or (env.Bit('nonsfi_nacl') and
                                      not env.Bit('tests_use_irt')):
    # Run unsandboxed executable directly, without sel_ldr.
    return env.CommandTest(name, command, size, **extra)

  if loader is None:
    if env.Bit('nonsfi_nacl'):
      loader = env.GetNonSfiLoader()
    else:
      loader = env.GetSelLdr()
    if loader is None:
      print 'WARNING: no sel_ldr found. Skipping test %s' % name
      return []

  # Avoid problems with [] as default arguments
  if sel_ldr_flags is None:
    sel_ldr_flags = []
  else:
    # Avoid modifying original list
    sel_ldr_flags = list(sel_ldr_flags)

  # Disable the validator if running a GLibC test under Valgrind.
  # http://code.google.com/p/nativeclient/issues/detail?id=1799
  if env.IsRunningUnderValgrind() and env.Bit('nacl_glibc'):
    sel_ldr_flags += ['-cc']
    # https://code.google.com/p/nativeclient/issues/detail?id=3158
    # We don't currently have valgrind.so for LD_PRELOAD to use.  That .so
    # is not used for newlib.
    # TODO(sehr): add valgrind.so built for NaCl.
    return []

  # Skip platform qualification checks on configurations with known issues.
  if env.GetEmulator() or env.IsRunningUnderValgrind():
    sel_ldr_flags += ['-Q']

  # Skip validation if we are using the x86-64 zero-based sandbox.
  # TODO(arbenson): remove this once the validator supports the x86-64
  # zero-based sandbox model.
  if env.Bit('x86_64_zero_based_sandbox'):
    sel_ldr_flags += ['-c']

  # The glibc modifications only make sense for nacl_env tests.
  # But this function gets used by some base_env (i.e. src/trusted/...)
  # tests too.  Don't add the --nacl_glibc changes to the command
  # line for those cases.
  if env.Bit('nacl_glibc') and env['NACL_BUILD_FAMILY'] != 'TRUSTED':
    if not glibc_static and not env.Bit('nacl_static_link'):
      # Enable file access so shared libraries can be loaded.
      sel_ldr_flags.append('-a')
      # Locally-built shared libraries come from ${LIB_DIR} while
      # toolchain-provided ones come from ${NACL_SDK_LIB}.
      library_path = '${LIB_DIR}:${NACL_SDK_LIB}'
      if env.Bit('build_x86'):
        # In the old glibc, we run via runnable-ld.so (the dynamic linker).
        command = ['${NACL_SDK_LIB}/runnable-ld.so',
                   '--library-path', library_path] + command
      else:
        # In the new glibc, we run via elf_loader and direct it where to
        # find the dynamic linker in the toolchain.
        command = [env.GetElfLoaderNexe(),
                   '--interp-prefix',
                   os.path.dirname(env.subst('${NACL_SDK_LIB}'))] + command
        sel_ldr_flags.extend(['-E', 'LD_LIBRARY_PATH=' + library_path])

  # Turn off sandbox for mac so coverage files can be written out.
  if ('TRUSTED_ENV' in env and
      env['TRUSTED_ENV'].Bit('coverage_enabled') and
      env.Bit('host_mac') and
      '-a' not in sel_ldr_flags):
    sel_ldr_flags += ['-a']

  if env.Bit('tests_use_irt'):
    sel_ldr_flags += ['-B', nacl_env.GetIrtNexe()]

  if skip_bootstrap:
    loader_cmd = [loader]
  else:
    loader_cmd = env.AddBootstrap(loader, [])

  if env.Bit('nonsfi_nacl'):
    # nonsfi_loader does not accept the same flags as sel_ldr yet, so
    # we ignore sel_ldr_flags here.
    command = [loader] + command
  else:
    command = loader_cmd + sel_ldr_flags + ['--'] + command

  if env.Bit('host_linux'):
    extra['using_nacl_signal_handler'] = True

  if env.ShouldUseVerboseOptions(extra):
    env.MakeVerboseExtraOptions(name, log_verbosity, extra)

  node = env.CommandTest(name, command, size, posix_path=True,
                         wrapper_program_prefix=wrapper_program_prefix, **extra)
  if env.Bit('tests_use_irt'):
    env.Alias('irt_tests', node)
  return node

pre_base_env.AddMethod(CommandSelLdrTestNacl)


TEST_EXTRA_ARGS = ['stdin', 'log_file',
                   'stdout_golden', 'stderr_golden', 'log_golden',
                   'filter_regex', 'filter_inverse', 'filter_group_only',
                   'osenv', 'arch', 'subarch', 'exit_status',
                   'num_runs', 'process_output_single',
                   'process_output_combined', 'using_nacl_signal_handler',
                   'declares_exit_status', 'time_warning', 'time_error']

TEST_TIME_THRESHOLD = {
    'small':   2,
    'medium': 10,
    'large':  60,
    'huge': 1800,
    }

# Valgrind handles SIGSEGV in a way our testing tools do not expect.
UNSUPPORTED_VALGRIND_EXIT_STATUS = ['trusted_sigabrt',
                                    'untrusted_sigill' ,
                                    'untrusted_segfault',
                                    'untrusted_sigsegv_or_equivalent',
                                    'trusted_segfault',
                                    'trusted_sigsegv_or_equivalent']


def GetPerfEnvDescription(env):
  """Return a string describing architecture, library, etc. options.

  This function attempts to gather a string that might inform why a performance
  change has occurred.
  """
  if env['NACL_BUILD_FAMILY'] == 'TRUSTED':
    # Trusted tests do not depend on the untrusted toolchain, untrusted libc,
    # whether or not the IRT is used, etc.
    description_list = ['trusted',
                        env['TARGET_PLATFORM'].lower(),
                        env['TARGET_FULLARCH']]
    return ARGUMENTS.get('perf_prefix', '') + '_'.join(description_list)
  description_list = [env['TARGET_FULLARCH']]
  # Using a list to keep the order consistent.
  bit_to_description = [ ('tests_use_irt', ('with_irt', '')),
                         ('bitcode', ('pnacl', 'nnacl')),
                         ('translate_fast', ('fast', '')),
                         ('use_sz', ('sz', '')),
                         ('nacl_glibc', ('glibc', 'newlib')),
                         ('nacl_static_link', ('static', 'dynamic')),
                         ]
  for (bit, (descr_yes, descr_no)) in bit_to_description:
    if env.Bit(bit):
      additional = descr_yes
    else:
      additional = descr_no
    if additional:
      description_list.append(additional)
  return ARGUMENTS.get('perf_prefix', '') + '_'.join(description_list)

pre_base_env.AddMethod(GetPerfEnvDescription)


TEST_NAME_MAP = {}

def GetTestName(target):
  key = str(target.path)
  return TEST_NAME_MAP.get(key, key)

pre_base_env['GetTestName'] = GetTestName


def SetTestName(node, name):
  for target in Flatten(node):
    TEST_NAME_MAP[str(target.path)] = name


def ApplyTestWrapperCommand(command_args, extra_deps):
  new_args = ARGUMENTS['test_wrapper'].split()
  for input_file in extra_deps:
    new_args.extend(['-F', input_file])
  for arg in command_args:
    if isinstance(arg, str):
      new_args.extend(['-a', arg])
    else:
      new_args.extend(['-f', arg])
  return new_args


def CommandTest(env, name, command, size='small', direct_emulation=True,
                extra_deps=[], posix_path=False, capture_output=True,
                capture_stderr=True, wrapper_program_prefix=None,
                scale_timeout=None, **extra):
  if not name.endswith('.out') or name.startswith('$'):
    raise Exception('ERROR: bad test filename for test output %r' % name)

  if env.IsRunningUnderValgrind():
    skip = 'Valgrind'
  elif env.Bit('asan'):
    skip = 'AddressSanitizer'
  else:
    skip = None
  # Valgrind tends to break crash tests by changing the exit status.
  # So far, tests using declares_exit_status are crash tests.  If this
  # changes, we will have to find a way to make declares_exit_status
  # work with Valgrind.
  if (skip is not None and
      (extra.get('exit_status') in UNSUPPORTED_VALGRIND_EXIT_STATUS or
       bool(int(extra.get('declares_exit_status', 0))))):
    print 'Skipping death test "%s" under %s' % (name, skip)
    return []

  if env.Bit('asan'):
    extra.setdefault('osenv', [])
    # Ensure that 'osenv' is a list.
    if isinstance(extra['osenv'], str):
      extra['osenv'] = [extra['osenv']]
    # ASan normally intercepts SIGSEGV and SIGFPE and disables our signal
    # handlers, which interferes with various NaCl tests, including the
    # platform qualification test built into sel_ldr.  We fix this by telling
    # ASan not to mess with SIGSEGV and SIGFPE.
    asan_options = ['handle_segv=0', 'handle_sigfpe=0']
    # ASan aborts on errors rather than exits. This changes the expected exit
    # codes for some tests.
    asan_options.append('abort_on_error=0')

    if env.Bit('host_mac') and int(platform.mac_ver()[0].split('.')[1]) < 7:
      # MacOS 10.6 has a bug in the libsandbox system library where it
      # makes a memcmp call that reads off the end of a malloc'd block.
      # The bug appears to be harmless, but trips an ASan report.  So
      # tell ASan to suppress memcmp checks.
      asan_options.append('strict_memcmp=0')
    # TODO(mcgrathr): Remove this when we clean up all the crufty old
    # code to be leak-free.
    # https://code.google.com/p/nativeclient/issues/detail?id=3874
    asan_options.append('detect_leaks=0')
    # Note that the ASan runtime doesn't use : specifically as a separator.
    # It actually just looks for "foo=" anywhere in the string with strstr,
    # so any separator will do.  The most obvious choices, ' ', ',', and ';'
    # all cause command_tester.py to split things up and get confused.
    extra['osenv'].append('ASAN_OPTIONS=' + ':'.join(asan_options))

  name = '${TARGET_ROOT}/test_results/' + name
  # NOTE: using the long version of 'name' helps distinguish opt vs dbg
  max_time = TEST_TIME_THRESHOLD[size]
  if 'scale_timeout' in ARGUMENTS:
    max_time = max_time * int(ARGUMENTS['scale_timeout'])
  if scale_timeout:
    max_time = max_time * scale_timeout

  if env.Bit('nacl_glibc'):
    suite = 'nacl_glibc'
  else:
    suite = 'nacl_newlib'
  if env.Bit('bitcode'):
    suite = 'p' + suite

  script_flags = ['--name', '%s.${GetTestName(TARGET)}' % suite,
                  '--time_warning', str(max_time),
                  '--time_error', str(10 * max_time),
                  ]

  run_under = ARGUMENTS.get('run_under')
  if run_under:
    run_under_extra_args = ARGUMENTS.get('run_under_extra_args')
    if run_under_extra_args:
      run_under = run_under + ',' + run_under_extra_args
    script_flags.append('--run_under')
    script_flags.append(run_under)

  emulator = env.GetEmulator()
  if emulator and direct_emulation:
    command = [emulator] + command

  # test wrapper should go outside of emulators like qemu, since the
  # test wrapper code is not emulated.
  if wrapper_program_prefix is not None:
    command = wrapper_program_prefix + command

  script_flags.append('--perf_env_description')
  script_flags.append(env.GetPerfEnvDescription())

  # Add architecture info.
  extra['arch'] = env['BUILD_ARCHITECTURE']
  extra['subarch'] = env['BUILD_SUBARCH']

  for flag_name, flag_value in extra.iteritems():
    assert flag_name in TEST_EXTRA_ARGS, repr(flag_name)
    if isinstance(flag_value, list):
      # Options to command_tester.py which are actually lists must not be
      # separated by whitespace. This stringifies the lists with a separator
      # char to satisfy command_tester.
      flag_value =  command_tester.StringifyList(flag_value)
    # do not add --flag + |flag_name| |flag_value| if
    # |flag_value| is false (empty).
    if flag_value:
      script_flags.append('--' + flag_name)
      # Make sure flag values are strings (or SCons objects) when building
      # up the command. Right now, this only means convert ints to strings.
      if isinstance(flag_value, int):
        flag_value = str(flag_value)
      script_flags.append(flag_value)

  # Other extra flags
  if not capture_output:
    script_flags.extend(['--capture_output', '0'])
  if not capture_stderr:
    script_flags.extend(['--capture_stderr', '0'])

  # Set command_tester.py's output filename.  We skip this when using
  # test_wrapper because the run_test_via_ssh.py wrapper does not have
  # the ability to copy result files back from the remote host.
  if 'test_wrapper' not in ARGUMENTS:
    script_flags.extend(['--output_stamp', name])

  test_script = env.File('${SCONSTRUCT_DIR}/tools/command_tester.py')
  extra_deps = extra_deps + [env.File('${SCONSTRUCT_DIR}/tools/test_lib.py')]
  command = ['${PYTHON}', test_script] + script_flags + command
  if 'test_wrapper' in ARGUMENTS:
    command = ApplyTestWrapperCommand(command, extra_deps)
  return env.AutoDepsCommand(name, command,
                             extra_deps=extra_deps, posix_path=posix_path,
                             disabled=env.Bit('do_not_run_tests'))

pre_base_env.AddMethod(CommandTest)


def FileSizeTest(env, name, envFile, max_size=None):
  """FileSizeTest() returns a scons node like the other XYZTest generators.
  It logs the file size of envFile in a perf-buildbot-recognizable format.
  Optionally, it can cause a test failure if the file is larger than max_size.
  """
  def doSizeCheck(target, source, env):
    filepath = source[0].abspath
    actual_size = os.stat(filepath).st_size
    command_tester.LogPerfResult(name,
                                 env.GetPerfEnvDescription(),
                                 '%.3f' % (actual_size / 1024.0),
                                 'KB')
    # Also get zipped size.
    nexe_file = open(filepath, 'rb')
    zipped_size = len(zlib.compress(nexe_file.read()))
    nexe_file.close()
    command_tester.LogPerfResult(name,
                                 'ZIPPED_' + env.GetPerfEnvDescription(),
                                 '%.3f' % (zipped_size / 1024.0),
                                 'KB')
    # Finally, do the size check.
    if max_size is not None and actual_size > max_size:
      # NOTE: this exception only triggers a failure for this particular test,
      # just like any other test failure.
      raise Exception("File %s larger than expected: expected up to %i, got %i"
                      % (filepath, max_size, actual_size))
  # If 'built_elsewhere', the file should should have already been built.
  # Do not try to built it and/or its pieces.
  if env.Bit('built_elsewhere'):
    env.Ignore(name, envFile)
  return env.Command(name, envFile, doSizeCheck)

pre_base_env.AddMethod(FileSizeTest)

def StripExecutable(env, name, exe):
  """StripExecutable returns a node representing the stripped version of |exe|.
     The stripped version will be given the basename |name|.
     NOTE: for now this only works with the untrusted toolchain.
     STRIP does not appear to be a first-class citizen in SCons and
     STRIP has only been set to point at the untrusted toolchain.
  """
  return env.Command(
      target=name,
      source=[exe],
      action=[Action('${STRIPCOM} ${SOURCES} -o ${TARGET}', '${STRIPCOMSTR}')])

pre_base_env.AddMethod(StripExecutable)


# TODO(ncbray): pretty up the log output when running this builder.
def DisabledCommand(target, source, env):
  pass

pre_base_env['BUILDERS']['DisabledCommand'] = Builder(action=DisabledCommand)


def AutoDepsCommand(env, name, command, extra_deps=[], posix_path=False,
                    disabled=False):
  """AutoDepsCommand() takes a command as an array of arguments.  Each
  argument may either be:

   * a string, or
   * a Scons file object, e.g. one created with env.File() or as the
     result of another build target.

  In the second case, the file is automatically declared as a
  dependency of this command.
  """
  command = list(command)
  deps = []
  for index, arg in enumerate(command):
    if not isinstance(arg, str):
      if len(Flatten(arg)) != 1:
        # Do not allow this, because it would cause "deps" to get out
        # of sync with the indexes in "command".
        # See http://code.google.com/p/nativeclient/issues/detail?id=1086
        raise AssertionError('Argument to AutoDepsCommand() actually contains '
                             'multiple (or zero) arguments: %r' % arg)
      if posix_path:
        command[index] = '${SOURCES[%d].posix}' % len(deps)
      else:
        command[index] = '${SOURCES[%d].abspath}' % len(deps)
      deps.append(arg)

  # If built_elsewhere, build commands are replaced by no-ops, so make sure
  # the targets don't get removed first
  if env.Bit('built_elsewhere'):
    env.Precious(deps)
  env.Depends(name, extra_deps)

  if disabled:
    return env.DisabledCommand(name, deps)
  else:
    return env.Command(name, deps, ' '.join(command))


pre_base_env.AddMethod(AutoDepsCommand)


def GetPrintableCommandName(cmd):
  """Look at the first few elements of cmd to derive a suitable command name."""
  cmd_tokens = cmd.split()
  if "python" in cmd_tokens[0] and len(cmd_tokens) >= 2:
    cmd_name = cmd_tokens[1]
  else:
    cmd_name = cmd_tokens[0].split('(')[0]

  # undo some pretty printing damage done by hammer
  cmd_name = cmd_name.replace('________','')
  # use file name part of a path
  return cmd_name.split('/')[-1]


def GetPrintableEnvironmentName(env):
  # use file name part of a obj root path as env name
  return env.subst('${TARGET_ROOT}').split('/')[-1]

pre_base_env.AddMethod(GetPrintableEnvironmentName)


def CustomCommandPrinter(cmd, targets, source, env):
  # Abuse the print hook to count the commands that are executed
  if env.Bit('target_stats'):
    cmd_name = GetPrintableCommandName(cmd)
    env_name = env.GetPrintableEnvironmentName()
    CMD_COUNTER[cmd_name] = CMD_COUNTER.get(cmd_name, 0) + 1
    ENV_COUNTER[env_name] = ENV_COUNTER.get(env_name, 0) + 1

  if env.Bit('pp'):
    # Our pretty printer
    if targets:
      cmd_name = GetPrintableCommandName(cmd)
      env_name = env.GetPrintableEnvironmentName()
      sys.stdout.write('[%s] [%s] %s\n' % (cmd_name, env_name,
                                           targets[0].get_path()))
  else:
    # The SCons default (copied from print_cmd_line in Action.py)
    sys.stdout.write(cmd + u'\n')

if 'generate_ninja' not in ARGUMENTS:
  pre_base_env.Append(PRINT_CMD_LINE_FUNC=CustomCommandPrinter)


def GetAbsDirArg(env, argument, target):
  """Fetch the named command-line argument and turn it into an absolute
directory name.  If the argument is missing, raise a UserError saying
that the given target requires that argument be given."""
  dir = ARGUMENTS.get(argument)
  if not dir:
    raise UserError('%s must be set when invoking %s' % (argument, target))
  return os.path.join(env.Dir('$MAIN_DIR').abspath, dir)

pre_base_env.AddMethod(GetAbsDirArg)


def MakeGTestEnv(env):
  # Create an environment to run unit tests using Gtest.
  gtest_env = env.Clone()

  if gtest_env['NACL_BUILD_FAMILY'] != 'TRUSTED' or not gtest_env.Bit('mac'):
    # This became necessary for the arm cross TC v4.6 but probable applies
    # to all new gcc TCs.  MacOS does not have this switch.
    gtest_env.Append(LINKFLAGS=['-pthread'])

  # Define compile-time flag that communicates that we are compiling in the test
  # environment (rather than for the TCB).
  if gtest_env['NACL_BUILD_FAMILY'] == 'TRUSTED':
    gtest_env.Append(CCFLAGS=['-DNACL_TRUSTED_BUT_NOT_TCB'])

  # This is necessary for unittest_main.c which includes gtest/gtest.h
  # The problem is that gtest.h includes other files expecting the
  # include path to be set.
  gtest_env.Prepend(CPPPATH=['${SOURCE_ROOT}/testing/gtest/include'])

  # gtest does not compile with our stringent settings.
  if gtest_env.Bit('linux') or gtest_env.Bit('mac'):
    # "-pedantic" is because of: gtest-typed-test.h:236:46: error:
    # anonymous variadic macros were introduced in C99
    # Also, gtest does not compile successfully with "-Wundef".
    gtest_env.FilterOut(CCFLAGS=['-pedantic', '-Wundef'])
  gtest_env.FilterOut(CXXFLAGS=['-fno-rtti', '-Weffc++'])

  # gtest is incompatible with static linking due to obscure libstdc++
  # linking interactions.
  # See http://code.google.com/p/nativeclient/issues/detail?id=1987
  gtest_env.FilterOut(LINKFLAGS=['-static'])

  gtest_env.Prepend(LIBS=['gtest'])
  return gtest_env

pre_base_env.AddMethod(MakeGTestEnv)

def MakeUntrustedNativeEnv(env):
  native_env = nacl_env.Clone()
  if native_env.Bit('bitcode') and not native_env.Bit('build_mips32'):
    native_env = native_env.PNaClGetNNaClEnv()
  return native_env

pre_base_env.AddMethod(MakeUntrustedNativeEnv)

def MakeBaseTrustedEnv(platform=None):
  base_env = MakeArchSpecificEnv(platform)
  base_env.Append(
    IS_BUILD_ENV = False,
    BUILD_SUBTYPE = '',
    CPPPATH = [
      '${SOURCE_ROOT}',
    ],

    EXTRA_CFLAGS = [],
    EXTRA_CXXFLAGS = [],
    EXTRA_LIBS = [],
    CFLAGS = ['${EXTRA_CFLAGS}'],
    CXXFLAGS = ['${EXTRA_CXXFLAGS}'],
  )
  if base_env.Bit('ncval_testing'):
    base_env.Append(CPPDEFINES = ['NCVAL_TESTING'])

  base_env.Append(BUILD_SCONSCRIPTS = [
      # KEEP THIS SORTED PLEASE
      'build/package_version/build.scons',
      'pynacl/build.scons',
      'src/nonsfi/irt/build.scons',
      'src/nonsfi/loader/build.scons',
      'src/shared/gio/build.scons',
      'src/shared/imc/build.scons',
      'src/shared/platform/build.scons',
      'src/third_party/gtest/build.scons',
      'src/trusted/cpu_features/build.scons',
      'src/trusted/debug_stub/build.scons',
      'src/trusted/desc/build.scons',
      'src/trusted/fault_injection/build.scons',
      'src/trusted/interval_multiset/build.scons',
      'src/trusted/nacl_base/build.scons',
      'src/trusted/perf_counter/build.scons',
      'src/trusted/platform_qualify/build.scons',
      'src/trusted/seccomp_bpf/build.scons',
      'src/trusted/service_runtime/build.scons',
      'src/trusted/validator/build.scons',
      'src/trusted/validator/driver/build.scons',
      'src/trusted/validator_arm/build.scons',
      'src/trusted/validator_ragel/build.scons',
      'src/trusted/validator_x86/build.scons',
      'tests/common/build.scons',
      'tests/lock_manager/build.scons',
      'tests/performance/build.scons',
      'tests/python_version/build.scons',
      'tests/sel_ldr_seccomp/build.scons',
      'tests/tools/build.scons',
      'tests/unittests/shared/imc/build.scons',
      'tests/unittests/shared/platform/build.scons',
      'tests/unittests/trusted/asan/build.scons',
      'tests/unittests/trusted/bits/build.scons',
      'tests/unittests/trusted/platform_qualify/build.scons',
      'tests/unittests/trusted/service_runtime/build.scons',
      'toolchain_build/build.scons',
  ])

  base_env.AddMethod(SDKInstallBin)

  # The ARM and MIPS validators can be built for any target that doesn't use
  # ELFCLASS64.
  if not base_env.Bit('build_x86_64'):
    base_env.Append(
        BUILD_SCONSCRIPTS = [
          'src/trusted/validator_mips/build.scons',
        ])

  base_env.AddChromeFilesFromGroup('trusted_scons_files')

  base_env.Replace(
      NACL_BUILD_FAMILY = 'TRUSTED',
  )

  # Add optional scons files if present in the directory tree.
  if os.path.exists(pre_base_env.subst('${MAIN_DIR}/supplement/build.scons')):
    base_env.Append(BUILD_SCONSCRIPTS=['${MAIN_DIR}/supplement/build.scons'])

  return base_env


# Select tests to run under coverage build.
pre_base_env['COVERAGE_TARGETS'] = [
    'small_tests', 'medium_tests', 'large_tests',
    'chrome_browser_tests']


pre_base_env.Help("""\
======================================================================
Help for NaCl
======================================================================

Common tasks:
-------------

* cleaning:           scons -c
* building:           scons
* smoke test:         scons --mode=nacl,opt-linux -k pp=1 smoke_tests

* sel_ldr:            scons --mode=opt-linux sel_ldr

Targets to build trusted code destined for the SDK:
* build trusted-code tools:     scons build_bin
* install trusted-code tools:   scons install_bin bindir=...
* These default to opt build, or add --mode=dbg-host for debug build.

Targets to build untrusted code destined for the SDK:
* build just libraries:         scons build_lib
* install just headers:         scons install_headers includedir=...
* install just libraries:       scons install_lib libdir=...
* install headers and libraries:scons install includedir=... libdir=...

* dump system info:   scons --mode=nacl,opt-linux dummy

Options:
--------

--prebuilt          Do not build things, just do install steps

--verbose           Full command line logging before command execution

pp=1                Use command line pretty printing (more concise output)

sysinfo=1           Verbose system info printing

naclsdk_validate=0  Suppress presence check of sdk



Automagically generated help:
-----------------------------
""")


def SetUpClang(env):
  env['CLANG_DIR'] = '${SOURCE_ROOT}/third_party/llvm-build/Release+Asserts/bin'
  env['CLANG_OPTS'] = []
  if env.Bit('asan'):
    if not (env.Bit('host_linux') or env.Bit('host_mac')):
      raise UserError("ERROR: ASan is only available for Linux and Mac")
    env.Append(CLANG_OPTS=['-fsanitize=address',
                           '-gline-tables-only',
                           '-fno-omit-frame-pointer',
                           '-DADDRESS_SANITIZER'])
    if env.Bit('host_mac'):
      # The built executables will try to find this library at runtime
      # in the directory containing the executable itself.  In the
      # Chromium build, the library just gets copied into that
      # directory.  Here, there isn't a single directory from which
      # all the test binaries are run (sel_ldr is run from staging/
      # but other trusted test binaries are run from their respective
      # obj/.../ directories).  So instead just point the dynamic linker
      # at the right directory using an environment variable.
      # Be sure to check and update clang_lib_version whenever updating
      # tools/clang revs in DEPS.
      clang_lib_version = '4.0.0'
      clang_lib_dir = str(env.Dir('${CLANG_DIR}/../lib/clang/%s/lib/darwin' %
                                  clang_lib_version).abspath)
      env['ENV']['DYLD_LIBRARY_PATH'] = clang_lib_dir
      if 'PROPAGATE_ENV' not in env:
        env['PROPAGATE_ENV'] = []
      env['PROPAGATE_ENV'].append('DYLD_LIBRARY_PATH')

  if env.Bit('msan'):
    if not env.Bit('host_linux') or not env.Bit('build_x86_64'):
      raise UserError('ERROR: MSan is only available for x86-64 Linux')
    track_origins = ARGUMENTS.get('msan_track_origins', '1')
    env.Append(CLANG_OPTS=['-fsanitize=memory',
                           '-fsanitize-memory-track-origins=%s' % track_origins,
                           '-gline-tables-only',
                           '-fno-omit-frame-pointer',
                           '-DMEMORY_SANITIZER'])

  env['CC'] = '${CLANG_DIR}/clang ${CLANG_OPTS}'
  env['CXX'] = '${CLANG_DIR}/clang++ ${CLANG_OPTS}'
  # Make sure we find Clang-supplied libraries like -lprofile_rt
  # in the Clang build we use, rather than from the system.
  # The system-installed versions go with the system-installed Clang
  # and might not be compatible with the Clang we're running.
  env.Append(LIBPATH=['${CLANG_DIR}/../lib'])

def GenerateOptimizationLevels(env):
  if env.Bit('clang'):
    SetUpClang(env)

  # Generate debug variant.
  debug_env = env.Clone(tools = ['target_debug'])
  debug_env['OPTIMIZATION_LEVEL'] = 'dbg'
  debug_env['BUILD_TYPE'] = debug_env.subst('$BUILD_TYPE')
  debug_env['BUILD_DESCRIPTION'] = debug_env.subst('$BUILD_DESCRIPTION')
  AddDualLibrary(debug_env)
  # Add to the list of fully described environments.
  environment_list.append(debug_env)

  # Generate opt variant.
  opt_env = env.Clone(tools = ['target_optimized'])
  opt_env['OPTIMIZATION_LEVEL'] = 'opt'
  opt_env['BUILD_TYPE'] = opt_env.subst('$BUILD_TYPE')
  opt_env['BUILD_DESCRIPTION'] = opt_env.subst('$BUILD_DESCRIPTION')
  AddDualLibrary(opt_env)
  # Add to the list of fully described environments.
  environment_list.append(opt_env)

  return (debug_env, opt_env)


def SDKInstallBin(env, name, node, target=None):
  """Add the given node to the build_bin and install_bin targets.
It will be installed under the given name with the build target appended.
The optional target argument overrides the setting of what that target is."""
  env.Alias('build_bin', node)
  if 'install_bin' in COMMAND_LINE_TARGETS:
    dir = env.GetAbsDirArg('bindir', 'install_bin')
    if target is None:
      target = env['TARGET_FULLARCH'].replace('-', '_')
    file_name, file_ext = os.path.splitext(name)
    output_name = file_name + '_' + target + file_ext
    install_node = env.InstallAs(os.path.join(dir, output_name), node)
    env.Alias('install_bin', install_node)


def MakeWindowsEnv(platform=None):
  base_env = MakeBaseTrustedEnv(platform)
  windows_env = base_env.Clone(
      BUILD_TYPE = '${OPTIMIZATION_LEVEL}-win',
      BUILD_TYPE_DESCRIPTION = 'Windows ${OPTIMIZATION_LEVEL} build',
      tools = ['target_platform_windows'],
      # Windows /SAFESEH linking requires either an .sxdata section be
      # present or that @feat.00 be defined as a local, absolute symbol
      # with an odd value.
      ASCOM = ('$ASPPCOM /E /D__ASSEMBLER__ | '
               '$WINASM -defsym @feat.00=1 -o $TARGET'),
      PDB = '${TARGET.base}.pdb',
      # Strict doesn't currently work for Windows since some of the system
      # libraries like wsock32 are magical.
      LIBS_STRICT = False,
      TARGET_ARCH='x86_64' if base_env.Bit('build_x86_64') else 'x86',
  )

  windows_env.Append(
      CPPDEFINES = [
          ['_WIN32_WINNT', '0x0501'],
          ['__STDC_LIMIT_MACROS', '1'],
          ['NOMINMAX', '1'],
          # WIN32 is used by ppapi
          ['WIN32', '1'],
          # WIN32_LEAN_AND_MEAN tells windows.h to omit obsolete and rarely
          # used #include files. This allows use of Winsock 2.0 which otherwise
          # would conflict with Winsock 1.x included by windows.h.
          ['WIN32_LEAN_AND_MEAN', ''],
      ],
      LIBS = ['ws2_32', 'advapi32'],
      # TODO(bsy) remove 4355 once cross-repo
      # NACL_ALLOW_THIS_IN_INITIALIZER_LIST changes go in.
      CCFLAGS = ['/EHsc', '/WX', '/wd4355', '/wd4800']
  )

  # This linker option allows us to ensure our builds are compatible with
  # Chromium, which uses it.
  if windows_env.Bit('build_x86_32'):
    windows_env.Append(LINKFLAGS = "/safeseh")

  # We use the GNU assembler (gas) on Windows so that we can use the
  # same .S assembly files on all platforms.  Microsoft's assembler uses
  # a completely different syntax for x86 code.
  if windows_env.Bit('build_x86_64'):
    # This assembler only works for x86-64 code.
    windows_env['WINASM'] = \
        windows_env.File('$SOURCE_ROOT/third_party/mingw-w64/mingw/bin/'
                         'x86_64-w64-mingw32-as.exe').abspath
  else:
    # This assembler only works for x86-32 code.
    windows_env['WINASM'] = \
        windows_env.File('$SOURCE_ROOT/third_party/gnu_binutils/files/'
                         'as').abspath
  return windows_env

(windows_debug_env,
 windows_optimized_env) = GenerateOptimizationLevels(MakeWindowsEnv())

def MakeUnixLikeEnv(platform=None):
  unix_like_env = MakeBaseTrustedEnv(platform)
  # -Wdeclaration-after-statement is desirable because MS studio does
  # not allow declarations after statements in a block, and since much
  # of our code is portable and primarily initially tested on Linux,
  # it'd be nice to get the build error earlier rather than later
  # (building and testing on Linux is faster).
  # TODO(nfullagar): should we consider switching to -std=c99 ?
  unix_like_env.Prepend(
    CFLAGS = [
        '-std=gnu99',
        '-Wdeclaration-after-statement',
        # Require defining functions as "foo(void)" rather than
        # "foo()" because, in C (but not C++), the latter defines a
        # function with unspecified arguments rather than no
        # arguments.
        '-Wstrict-prototypes',
        ],
    CCFLAGS = [
        # '-malign-double',
        '-Wall',
        '-pedantic',
        '-Wextra',
        '-Wno-long-long',
        '-Wswitch-enum',
        '-Wsign-compare',
        '-Wundef',
        '-fdiagnostics-show-option',
        '-fvisibility=hidden',
        '-fstack-protector',
        ] + werror_flags,
    # NOTE: pthread is only neeeded for libppNaClPlugin.so and on arm
    LIBS = ['pthread'],
    CPPDEFINES = [['__STDC_LIMIT_MACROS', '1'],
                  ['__STDC_FORMAT_MACROS', '1'],
                  ],
  )
  # Android's stlport uses __STRICT_ANSI__ to exclude "long long".
  # This breaks basically all C++ code that uses stlport.
  if not unix_like_env.Bit('android'):
    unix_like_env.Prepend(CXXFLAGS=['-std=c++98'])

  if not unix_like_env.Bit('clang'):
    unix_like_env.Append(CCFLAGS=['--param', 'ssp-buffer-size=4'])

  if unix_like_env.Bit('werror'):
    unix_like_env.Append(LINKFLAGS=['-Werror'])

  return unix_like_env


def MakeMacEnv(platform=None):
  mac_env = MakeUnixLikeEnv(platform).Clone(
      BUILD_TYPE = '${OPTIMIZATION_LEVEL}-mac',
      BUILD_TYPE_DESCRIPTION = 'MacOS ${OPTIMIZATION_LEVEL} build',
      tools = ['target_platform_mac'],
      # TODO(bradnelson): this should really be able to live in unix_like_env
      #                   but can't due to what the target_platform_x module is
      #                   doing.
      LINK = '$CXX',
      PLUGIN_SUFFIX = '.bundle',
  )
  # On Mac, only the newer clang toolchains can parse some of the trusted
  # code's assembly syntax, so turn clang on by default.
  mac_env.SetBits('clang')

  # For no good reason, this all gets instantiated on every platform,
  # and then only actually used on Mac.  But the find_sdk.py script
  # will barf if run on a non-Mac.
  if pynacl.platform.IsMac():
    # mac_sdk_min must be kept in synch with mac_sdk_min in
    # chromium/src/build/config/mac/mac_sdk.gni.
    mac_sdk_min = '10.10'
    # Find the Mac SDK to use as sysroot.
    # This invocation matches the model in //build/config/mac/mac_sdk.gni.
    mac_sdk_sysroot, mac_sdk_version = subprocess.check_output([
        sys.executable,
        os.path.join(os.path.pardir, 'build', 'mac', 'find_sdk.py'),
        '--print_sdk_path',
        mac_sdk_min
        ]).splitlines()
  else:
    mac_sdk_sysroot = 'ThisIsNotAMac'

  # This should be kept in synch with mac_deployment_target
  # in build/common.gypi, which in turn should be kept in synch
  # with chromium/src/build/common.gypi.
  mac_deployment_target = '10.6'

  sdk_flags = ['-isysroot', mac_sdk_sysroot,
               '-mmacosx-version-min=' + mac_deployment_target]
  mac_env.Append(CCFLAGS=sdk_flags, ASFLAGS=sdk_flags, LINKFLAGS=sdk_flags)

  subarch_flag = '-m%s' % mac_env['BUILD_SUBARCH']
  mac_env.Append(
      # '-Wno-gnu' is required for the statement expression defining dirfd
      # for OSX -- otherwise, a warning is generated.
      CCFLAGS=[subarch_flag, '-fPIC', '-Wno-gnu'],
      ASFLAGS=[subarch_flag],
      LINKFLAGS=[subarch_flag, '-fPIC'],
      CPPDEFINES = [# defining _DARWIN_C_SOURCE breaks 10.4
                    #['_DARWIN_C_SOURCE', '1'],
                    #['__STDC_LIMIT_MACROS', '1']
                    ],
  )

  return mac_env

(mac_debug_env, mac_optimized_env) = GenerateOptimizationLevels(MakeMacEnv())


def which(cmd, paths=os.environ.get('PATH', '').split(os.pathsep)):
  for p in paths:
     if os.access(os.path.join(p, cmd), os.X_OK):
       return True
  return False


def SetUpLinuxEnvArm(env):
  jail = env.GetToolchainDir(toolchain_name='arm_trusted')
  if not platform.machine().startswith('arm'):
    # Allow emulation on non-ARM hosts.
    env.Replace(EMULATOR=jail + '/run_under_qemu_arm')
  if env.Bit('built_elsewhere'):
    def FakeInstall(dest, source, env):
      print 'Not installing', dest
      # Replace build commands with no-ops
    env.Replace(CC='true', CXX='true', LD='true',
                AR='true', RANLIB='true', INSTALL=FakeInstall)
  else:
    env.Replace(CC='arm-linux-gnueabihf-gcc',
                CXX='arm-linux-gnueabihf-g++',
                LD='arm-linux-gnueabihf-ld',
                ASFLAGS=[],
                # The -rpath-link argument is needed on Ubuntu/Precise to
                # avoid linker warnings about missing ld.linux.so.3.
                # TODO(sbc): remove this once we stop supporting Precise
                # as a build environment.
                LINKFLAGS=['-Wl,-rpath-link=' + jail +
                           '/lib/arm-linux-gnueabihf']
                )
    # Note we let the compiler choose whether it's -marm or -mthumb by
    # default.  The hope is this will have the best chance of testing
    # the prevailing compilation mode used for Chromium et al.
    env.Prepend(CCFLAGS=['-march=armv7-a'])

  # get_plugin_dirname.cc has a dependency on dladdr
  env.Append(LIBS=['dl'])

def SetUpAndroidEnv(env):
  env.FilterOut(CPPDEFINES=[['_LARGEFILE64_SOURCE', '1']])
  android_ndk_root = os.path.join('${SOURCE_ROOT}', 'third_party',
                                  'android_ndk')
  android_ndk_experimental_root = os.path.join('${SOURCE_ROOT}',
                                               'third_party', 'android_tools',
                                               'ndk_experimental')
  android_sdk_root = os.path.join('${SOURCE_ROOT}', 'third_party',
                                  'android_tools', 'sdk')
  android_sdk_version = 21
  android_stlport_root = os.path.join(android_ndk_root, 'sources', 'cxx-stl',
                                      'stlport')
  ndk_host_os_map = {
      pynacl.platform.OS_WIN : 'win',
      pynacl.platform.OS_MAC: 'darwin',
      pynacl.platform.OS_LINUX : 'linux'
      }
  host_os = ndk_host_os_map[pynacl.platform.GetOS()]
  android_sdk = os.path.join(android_sdk_root, 'platforms',
                             'android-%s' % android_sdk_version)
  arch_cflags = []
  if env.Bit('build_arm'):
    android_ndk_target_prefix = 'arm-linux-androideabi'
    android_ndk_version = '4.8'
    android_app_abi = 'armeabi-v7a'
    android_ndk_sysroot = os.path.join(android_ndk_root, 'platforms',
                                       'android-14', 'arch-arm')
    android_ndk_lib_dir = os.path.join('usr', 'lib')
    android_toolchain = os.path.join(android_ndk_root, 'toolchains',
                                     'arm-linux-androideabi-4.8', 'prebuilt',
                                     '%s-x86_64' % host_os, 'bin')
    arch_cflags += ['-march=armv7-a', '-mfloat-abi=softfp']
  elif env.Bit('build_x86_32'):
    android_ndk_target_prefix = 'i686-linux-android'
    android_ndk_version = '4.8'
    android_app_abi = 'x86'
    android_ndk_sysroot = os.path.join(android_ndk_root, 'platforms',
                                       'android-14', 'arch-x86')
    android_ndk_lib_dir = os.path.join('usr', 'lib')
    android_toolchain = os.path.join(android_ndk_root, 'toolchains',
                                     'x86-4.8', 'prebuilt',
                                     '%s-x86_64' % host_os, 'bin')
    arch_cflags += ['-m32', '-msse2']
  # TODO(sehr): add other android architecture platform settings here.
  android_ndk_include = os.path.join(android_ndk_sysroot, 'usr', 'include')
  android_ndk_lib = os.path.join(android_ndk_sysroot, android_ndk_lib_dir)
  android_sdk_jar = os.path.join(android_sdk, 'android.jar')
  android_stlport_include = os.path.join(android_stlport_root, 'stlport')
  android_stlport_libs_dir = os.path.join(android_stlport_root, 'libs',
                                          android_app_abi)
  android_ndk_libgcc_path = os.path.join(android_toolchain, '..', 'lib', 'gcc',
                                         android_ndk_target_prefix,
                                         android_ndk_version)
  env.Replace(CC=os.path.join(android_toolchain,
                              '%s-gcc' % android_ndk_target_prefix),
              CXX=os.path.join(android_toolchain,
                              '%s-g++' % android_ndk_target_prefix),
              LD=os.path.join(android_toolchain,
                              '%s-g++' % android_ndk_target_prefix),
              AR=os.path.join(android_toolchain,
                              '%s-ar' % android_ndk_target_prefix),
              RANLIB=os.path.join(android_toolchain,
                                  '%s-ranlib' % android_ndk_target_prefix),
              READELF=os.path.join(android_toolchain,
                                   '%s-readelf' % android_ndk_target_prefix),
              STRIP=os.path.join(android_toolchain,
                                 '%s-strip' % android_ndk_target_prefix),
              EMULATOR=os.path.join(android_sdk_root, 'tools', 'emulator'),
              LIBPATH=['${LIB_DIR}',
                       android_ndk_lib,
                       android_ndk_libgcc_path,
                       os.path.join(android_stlport_root, 'libs',
                                    android_app_abi),
                       ],
              LIBS=['stlport_shared',
                    'gcc',
                    'c',
                    'dl',
                    'm',
                    ],
              )
  # SHLINKFLAGS should not inherit options from LINKFLAGS.
  env.FilterOut(SHLINKFLAGS=['$LINKFLAGS'])
  env.Append(CCFLAGS=['--sysroot=' + android_ndk_sysroot,
                      '-isystem=' + os.path.join(android_ndk_sysroot, 'usr',
                                                 'include'),
                      '-I%s' % android_stlport_include,
                      '-ffunction-sections',
                      '-g',
                      '-fstack-protector',
                      '-fno-short-enums',
                      '-finline-limit=64',
                      '-Wa,--noexecstack',
                      '-DANDROID',
                      '-D__ANDROID__',
                      # Due to bogus warnings on uintptr_t formats.
                      '-Wno-format',
                      ] + arch_cflags,
             CXXFLAGS=['-I%s' % android_stlport_include,
                       '-I%s' % android_ndk_include,
                       '-fno-exceptions',
                       ],
             LINKFLAGS=['--sysroot=' + android_ndk_sysroot,
                        '-nostdlib',
                        '-Wl,--no-undefined',
                        # Don't export symbols from statically linked libraries.
                        '-Wl,--exclude-libs=ALL',
                        # crtbegin_dynamic.o should be the last item in ldflags.
                        os.path.join(android_ndk_lib, 'crtbegin_dynamic.o'),
                        ],
             LINKCOM=' $ANDROID_EXTRA_LIBS',
             ANDROID_EXTRA_LIBS=os.path.join(android_ndk_lib,
                                             'crtend_android.o'),
             SHLINKFLAGS=['--sysroot=' + android_ndk_sysroot,
                          '-nostdlib',
                          # crtbegin_so.o should be the last item in ldflags.
                          os.path.join(android_ndk_lib, 'crtbegin_so.o'),
                          ],
             SHLINKCOM=' $ANDROID_EXTRA_SHLIBS',
             ANDROID_EXTRA_SHLIBS=os.path.join(android_ndk_lib,
                                               'crtend_so.o'),
             )
  return env

def SetUpLinuxEnvMips(env):
  jail = env.GetToolchainDir(toolchain_name='mips_trusted')
  if not platform.machine().startswith('mips'):
    # Allow emulation on non-MIPS hosts.
    env.Replace(EMULATOR=jail + '/run_under_qemu_mips32')
  if env.Bit('built_elsewhere'):
    def FakeInstall(dest, source, env):
      print 'Not installing', dest
      # Replace build commands with no-ops
    env.Replace(CC='true', CXX='true', LD='true',
                AR='true', RANLIB='true', INSTALL=FakeInstall)
  else:
    tc_dir = os.path.join(jail, 'bin')
    if not which(os.path.join(tc_dir, 'mipsel-linux-gnu-gcc')):
      print ("WARNING: "
          "MIPS trusted toolchain not found - try running:\n"
          "  build/package_version/package_version.py --packages"
          " linux_x86/mips_trusted sync -x\n"
          "Or build it yourself with:\n"
          "  tools/trusted_cross_toolchains/trusted-toolchain-creator"
          ".mipsel.debian.sh nacl_sdk")
    env.Replace(CC=os.path.join(tc_dir, 'mipsel-linux-gnu-gcc'),
                CXX=os.path.join(tc_dir, 'mipsel-linux-gnu-g++'),
                LD=os.path.join(tc_dir, 'mipsel-linux-gnu-ld'),
                ASFLAGS=[],
                LIBPATH=['${LIB_DIR}',
                         jail + '/sysroot/usr/lib']
                )

    env.Append(LIBS=['rt', 'dl', 'pthread'],
                     CCFLAGS=['-march=mips32r2'])

# Makes a generic Linux development environment.
# Linux development environments are used in two different ways.
# 1) To produce trusted tools (e.g., sel_ldr), called TRUSTED_ENV
# 2) To produce build tools (e.g., tls_edit), called BUILD_ENV
def MakeGenericLinuxEnv(platform=None):
  linux_env = MakeUnixLikeEnv(platform).Clone(
      BUILD_TYPE = '${OPTIMIZATION_LEVEL}-linux',
      BUILD_TYPE_DESCRIPTION = 'Linux ${OPTIMIZATION_LEVEL} build',
      tools = ['target_platform_linux'],
      # TODO(bradnelson): this should really be able to live in unix_like_env
      #                   but can't due to what the target_platform_x module is
      #                   doing.
      LINK = '$CXX',
  )

  # Prepend so we can disable warnings via Append
  linux_env.Prepend(
      CPPDEFINES = [['_POSIX_C_SOURCE', '199506'],
                    ['_XOPEN_SOURCE', '600'],
                    ['_GNU_SOURCE', '1'],
                    ['_LARGEFILE64_SOURCE', '1'],
                    ],
      LIBS = ['rt'],
      )

  if linux_env.Bit('build_x86_32'):
    linux_env.Prepend(
        CCFLAGS = ['-m32'],
        LINKFLAGS = ['-m32'],
        )
  elif linux_env.Bit('build_x86_64'):
    linux_env.Prepend(
        CCFLAGS = ['-m64'],
        LINKFLAGS = ['-m64'],
        )
  elif linux_env.Bit('build_arm'):
    SetUpLinuxEnvArm(linux_env)
  elif linux_env.Bit('build_mips32'):
    SetUpLinuxEnvMips(linux_env)
  else:
    Banner('Strange platform: %s' % GetTargetPlatform())

  # These are desireable options for every Linux platform:
  # _FORTIFY_SOURCE: general paranoia "hardening" option for library functions
  # -fPIE/-pie: create a position-independent executable
  # relro/now: "hardening" options for linking
  # noexecstack: ensure that the executable does not get a PT_GNU_STACK
  #              header that causes the kernel to set the READ_IMPLIES_EXEC
  #              personality flag, which disables NX page protection.
  linux_env.Prepend(CPPDEFINES=[['-D_FORTIFY_SOURCE', '2']])
  # By default SHLINKFLAGS uses $LINKFLAGS, but we do not want -pie
  # in $SHLINKFLAGS, only in $LINKFLAGS.  So move LINKFLAGS over to
  # COMMON_LINKFLAGS, and add the "hardening" options there.  Then
  # make both LINKFLAGS and SHLINKFLAGS refer to that, and add -pie
  # only to LINKFLAGS.
  linux_env.Replace(COMMON_LINKFLAGS=linux_env['LINKFLAGS'],
                    LINKFLAGS=['$COMMON_LINKFLAGS'])
  linux_env.FilterOut(SHLINKFLAGS=['$LINKFLAGS'])
  linux_env.Prepend(SHLINKFLAGS=['$COMMON_LINKFLAGS'])
  linux_env.Prepend(COMMON_LINKFLAGS=['-Wl,-z,relro',
                                      '-Wl,-z,now',
                                      '-Wl,-z,noexecstack'])
  linux_env.Prepend(LINKFLAGS=['-pie'])
  # The ARM toolchain has a linker that doesn't handle the code its
  # compiler generates under -fPIE.
  if linux_env.Bit('build_arm') or linux_env.Bit('build_mips32'):
    linux_env.Prepend(CCFLAGS=['-fPIC'])
    # TODO(mcgrathr): Temporarily punt _FORTIFY_SOURCE for ARM because
    # it causes a libc dependency newer than the old bots have installed.
    linux_env.FilterOut(CPPDEFINES=[['-D_FORTIFY_SOURCE', '2']])
  else:
    linux_env.Prepend(CCFLAGS=['-fPIE'])

  # We always want to use the same flags for .S as for .c because
  # code-generation flags affect the predefines we might test there.
  linux_env.Replace(ASFLAGS=['${CCFLAGS}'])

  return linux_env

# Specializes a generic Linux development environment to be a trusted
# environment.
def MakeTrustedLinuxEnv(platform=None):
  linux_env = MakeGenericLinuxEnv(platform)
  if linux_env.Bit('android'):
    SetUpAndroidEnv(linux_env)
  return linux_env

(linux_debug_env, linux_optimized_env) = \
    GenerateOptimizationLevels(MakeTrustedLinuxEnv())


def BiasedBitcodeFlags(env):
  """ Return clang flags to use biased bitcode and generate native-ABI-compliant
      code. Does not imply pre-translation.
  """
  if env.Bit('build_x86_32'):
    return ['--target=i686-unknown-nacl']
  if env.Bit('build_x86_64'):
    return ['--target=x86_64-unknown-nacl']
  if env.Bit('build_arm'):
    return ['--target=armv7-unknown-nacl-gnueabihf', '-mfloat-abi=hard']
  if env.Bit('build_mips32'):
    return []
  raise UserError('No known target bits set')

pre_base_env.AddMethod(BiasedBitcodeFlags)

# Do this before the site_scons/site_tools/naclsdk.py stuff to pass it along.
pre_base_env.Append(
    PNACL_BCLDFLAGS = ARGUMENTS.get('pnacl_bcldflags', '').split(':'))


# The nacl_env is used to build native_client modules
# using a special tool chain which produces platform
# independent binaries
# NOTE: this loads stuff from: site_scons/site_tools/naclsdk.py
nacl_env = MakeArchSpecificEnv()
# See comment below about libc++ and libpthread in NONIRT_LIBS.
using_nacl_libcxx = nacl_env.Bit('bitcode') or nacl_env.Bit('nacl_clang')
nacl_env = nacl_env.Clone(
    tools = ['naclsdk'],
    NACL_BUILD_FAMILY = 'UNTRUSTED',
    BUILD_TYPE = 'nacl',
    BUILD_TYPE_DESCRIPTION = 'NaCl module build',

    ARFLAGS = 'rc',

    # ${SOURCE_ROOT} for #include <ppapi/...>
    CPPPATH = [
      '${SOURCE_ROOT}',
    ],

    EXTRA_CFLAGS = [],
    EXTRA_CXXFLAGS = [],
    EXTRA_LIBS = [],
    EXTRA_LINKFLAGS = ARGUMENTS.get('nacl_linkflags', '').split(':'),

    # always optimize binaries
    CCFLAGS = ['-O2',
               '-g',
               '-fomit-frame-pointer',
               # This makes sure unwind/backtrace info is available for
               # all code locations.  Note build/untrusted.gypi uses it too.
               '-fasynchronous-unwind-tables',
               '-Wall',
               '-Wundef',
               '-fdiagnostics-show-option',
               '-pedantic',
               ] +
              werror_flags,

    CFLAGS = ['-std=gnu99',
              ],
    CXXFLAGS = ['-std=gnu++98',
                '-Wno-long-long',
                ],

    # This magic is copied from scons-2.0.1/engine/SCons/Defaults.py
    # where this pattern is used for _LIBDIRFLAGS, which produces -L
    # switches.  Here we are producing a -Wl,-rpath-link,DIR for each
    # element of LIBPATH, i.e. for each -LDIR produced.
    RPATH_LINK_FLAGS = '$( ${_concat(RPATHLINKPREFIX, LIBPATH, RPATHLINKSUFFIX,'
                       '__env__, RDirs, TARGET, SOURCE)} $)',
    RPATHLINKPREFIX = '-Wl,-rpath-link,',
    RPATHLINKSUFFIX = '',

    LIBS = [],
    LINKFLAGS = ['${RPATH_LINK_FLAGS}'] + (['-Werror'] if nacl_env.Bit('werror')
                                           else []),

    # These are settings for in-tree, non-browser tests to use.
    # They use libraries that circumvent the IRT-based implementations
    # in the public libraries.
    # Note that pthread_private is part of NONIRT_LIBS for clang because
    # libc++ depends on libpthread. However we can't just add
    # libpthread_private to the link line because those libs get added before
    # the standard libs, so the references that come from libc++ itself will
    # still get satisfied from libpthread instead of libpthread_private (and
    # that code will crash because it requires the IRT). So put libc++ on the
    # user link line before libpthread_private to ensure that its references
    # to libpthread also get satisfied by libpthread_private.
    # TODO(dschuff): Also remove the hack in pnacl-ld and use this for pnacl.
    NONIRT_LIBS = (['nacl_sys_private'] +
                   (['c++','pthread_private'] if using_nacl_libcxx else [])),
    PTHREAD_LIBS = ['pthread_private'],
    DYNCODE_LIBS = ['nacl_dyncode_private'],
    EXCEPTION_LIBS = ['nacl_exception_private'],
    LIST_MAPPINGS_LIBS = ['nacl_list_mappings_private'],
    RANDOM_LIBS = ['nacl_random_private'],
    )

def UsesAbiNote(env):
  """Return True if using a new-style GCC with .note.NaCl.ABI.* notes.
This means there will always be an RODATA segment, even if just for the note."""
  return env.Bit('build_arm') and not env.Bit('bitcode')

nacl_env.AddMethod(UsesAbiNote)

def UnderWindowsCoverage(env):
  """Return True if using running on coverage under windows."""
  if 'TRUSTED_ENV' not in env:
    return False
  return env['TRUSTED_ENV'].Bit('coverage_enabled') and env.Bit('host_windows')

nacl_env.AddMethod(UnderWindowsCoverage)

def SetNonStableBitcodeIfAllowed(env, allow_sb_translator=False):
  """ This modifies the environment to allow features that aren't part
      of PNaCl's stable ABI.  If tests using these features should be
      skipped entirely, this returns False.  Otherwise, on success, it
      returns True.
  """
  if env.Bit('bitcode') and env.Bit('skip_nonstable_bitcode'):
    return False
  # The PNaCl sandboxed translator (for the most part) only accepts stable
  # bitcode, so in most cases we skip building non-stable tests.
  # However, there are some limited cases like debug information which
  # we support but do not guarantee stability. Tests targeting such cases
  # can opt-in to testing w/ allow_sb_translator=True.
  if env.Bit('use_sandboxed_translator') and not allow_sb_translator:
    return False
  # Change environment to skip finalization step.
  env.SetBits('nonstable_bitcode')
  return True

nacl_env.AddMethod(SetNonStableBitcodeIfAllowed)


def AllowInlineAssembly(env):
  """ This modifies the environment to allow inline assembly in
      untrusted code.  If the environment cannot be modified to allow
      inline assembly, it returns False.  Otherwise, on success, it
      returns True.
  """
  if env.Bit('bitcode'):
    # For each architecture, we only attempt to make our inline
    # assembly code work with one untrusted-code toolchain.  For x86,
    # we target GCC, but not PNaCl/Clang, because the latter's
    # assembly support has various quirks that we don't want to have
    # to debug.  For ARM, we target PNaCl/Clang, because that is the
    # only current ARM toolchain.  One day, we will have an ARM GCC
    # toolchain, and we will no longer need to use inline assembly
    # with PNaCl/Clang at all.
    #
    # For Non-SFI NaCl we use inline assembly in PNaCl/Clang.
    if not (env.Bit('build_arm') or env.Bit('build_mips32')
            or env.Bit('nonsfi_nacl')):
      return False
    # Inline assembly does not work in pexes.
    if env.Bit('pnacl_generate_pexe'):
      return False
    env.AddBiasForPNaCl()
    env.PNaClForceNative()

    if env.Bit('build_x86_32'):
      env.AppendUnique(CCFLAGS=['--target=i686-unknown-nacl'])
    elif env.Bit('build_x86_64'):
      env.AppendUnique(CCFLAGS=['--target=x86_64-unknown-nacl'])
    elif env.Bit('build_arm'):
      env.AppendUnique(CCFLAGS=['--target=armv7a-unknown-nacl-gnueabihf',
                                '-mfloat-abi=hard'])
    # Enable the use of inline assembly.
    env.Append(CCFLAGS=['-fgnu-inline-asm'])
  return True

nacl_env.AddMethod(AllowInlineAssembly)


# TODO(mseaborn): Enable this unconditionally once the C code on the
# Chromium side compiles successfully with this warning.
if not enable_chrome:
  nacl_env.Append(CFLAGS=['-Wstrict-prototypes'])

# This is the address at which a user executable is expected to place its
# data segment in order to be compatible with the integrated runtime (IRT)
# library.  This address should not be changed lightly.
irt_compatible_rodata_addr = 0x10000000
# This is the address at which the IRT's own code will be located.
# It must be below irt_compatible_rodata and leave enough space for
# the code segment of the IRT.  It should be as close as possible to
# irt_compatible_rodata so as to leave the maximum contiguous area
# available for the dynamic code loading area that falls below it.
# This can be adjusted as necessary for the actual size of the IRT code.
irt_code_addr = irt_compatible_rodata_addr - (6 << 20) # max 6M IRT code
# This is the address at which the IRT's own data will be located.  The
# 32-bit sandboxes limit the address space to 1GB; the initial thread's
# stack sits at the top of the address space and extends down for
# NACL_DEFAULT_STACK_MAX (src/trusted/service_runtime/sel_ldr.h) below.
# So this must be below there, and leave enough space for the IRT's own
# data segment.  It should be as high as possible so as to leave the
# maximum contiguous area available for the user's data and break below.
# This can be adjusted as necessary for the actual size of the IRT data
# (that is RODATA, rounded up to 64k, plus writable data).
# 1G (address space) - 16M (NACL_DEFAULT_STACK_MAX) - 1MB (IRT rodata+data)
irt_data_addr = (1 << 30) - (16 << 20) - (1 << 20)

nacl_env.Replace(
    IRT_DATA_REGION_START = '%#.8x' % irt_compatible_rodata_addr,
    # Load addresses of the IRT's code and data segments.
    IRT_BLOB_CODE_START = '%#.8x' % irt_code_addr,
    IRT_BLOB_DATA_START = '%#.8x' % irt_data_addr,
    )

def TestsUsePublicListMappingsLib(env):
  """Use the public list_mappings library for in-tree tests."""
  env.Replace(LIST_MAPPINGS_LIBS=['nacl_list_mappings'])

def TestsUsePublicLibs(env):
  """Change the environment so it uses public libraries for in-tree tests."""
  env.Replace(NONIRT_LIBS=['pthread'] if env.Bit('bitcode') else [],
              PTHREAD_LIBS=['pthread'],
              DYNCODE_LIBS=['nacl_dyncode', 'nacl'],
              EXCEPTION_LIBS=['nacl_exception', 'nacl'],
              RANDOM_LIBS=['nacl'])

# glibc is incompatible with libpthread_private and libnacl_sys_private.
if nacl_env.Bit('nacl_glibc'):
  nacl_env.Replace(NONIRT_LIBS=[],
                   PTHREAD_LIBS=['pthread'])

# These add on to those set in pre_base_env, above.
nacl_env.Append(
    CPPDEFINES = [
        # This ensures that UINT32_MAX gets defined.
        ['__STDC_LIMIT_MACROS', '1'],
        # This ensures that PRId64 etc. get defined.
        ['__STDC_FORMAT_MACROS', '1'],
        # _GNU_SOURCE ensures that strtof() gets declared.
        ['_GNU_SOURCE', 1],
        ['_POSIX_C_SOURCE', '199506'],
        ['_XOPEN_SOURCE', '600'],

        ['DYNAMIC_ANNOTATIONS_ENABLED', '1' ],
        ['DYNAMIC_ANNOTATIONS_PREFIX', 'NACL_' ],

        ['NACL_BUILD_ARCH', '${BUILD_ARCHITECTURE}'],
        ['NACL_BUILD_SUBARCH', '${BUILD_SUBARCH}'],
        ],
    )

def FixWindowsAssembler(env):
  if env.Bit('host_windows'):
    # ASCOM is the rule used by .s files and ASPPCOM is the rule used by .S
    # files; the latter uses the C preprocessor.  This is needed because Windows
    # builds are case-insensitive, so they all appear as .s files.
    env.Replace(ASCOM='${ASPPCOM}')

FixWindowsAssembler(nacl_env)

# Look in the local include and lib directories before the toolchain's.
nacl_env['INCLUDE_DIR'] = '${TARGET_ROOT}/include'
# Remove the default $LIB_DIR element so that we prepend it without duplication.
# Using PrependUnique alone would let it stay last, where we want it first.
nacl_env.FilterOut(LIBPATH=['${LIB_DIR}'])
nacl_env.PrependUnique(
    CPPPATH = ['${INCLUDE_DIR}'],
    LIBPATH = ['${LIB_DIR}'],
    )

if nacl_env.Bit('bitcode'):
  # passing -O when linking requests LTO, which does additional global
  # optimizations at link time
  nacl_env.Append(LINKFLAGS=['-O3'])
  if not nacl_env.Bit('nacl_glibc'):
    nacl_env.Append(LINKFLAGS=['-static'])

  if nacl_env.Bit('translate_fast'):
    nacl_env.Append(LINKFLAGS=['-Xlinker', '-translate-fast'])
    nacl_env.Append(TRANSLATEFLAGS=['-translate-fast'])
  if nacl_env.Bit('use_sz'):
    nacl_env.Append(TRANSLATEFLAGS=['--use-sz'])

  # With pnacl's clang base/ code uses the "override" keyword.
  nacl_env.Append(CXXFLAGS=['-Wno-c++11-extensions'])
  # Allow extraneous semicolons.  (Until these are removed.)
  # http://code.google.com/p/nativeclient/issues/detail?id=2861
  nacl_env.Append(CCFLAGS=['-Wno-extra-semi'])
  # Allow unused private fields.  (Until these are removed.)
  # http://code.google.com/p/nativeclient/issues/detail?id=2861
  nacl_env.Append(CCFLAGS=['-Wno-unused-private-field'])
  # native_client/src/nonsfi/linux/linux_syscall_structs.h uses designated
  # initializers, which causes a warning when included from c++98 code.
  nacl_env.Append(CXXFLAGS=['-Wno-c99-extensions'])

if nacl_env.Bit('nacl_clang'):
  # third_party/valgrind/nacl_valgrind.h uses asm instead of __asm__
  # https://code.google.com/p/nativeclient/issues/detail?id=3974
  # TODO(dschuff): change it to __asm__ and remove this suppression.
  nacl_env.Append(CCFLAGS=['-Wno-language-extension-token'])

# We use a special environment for building the IRT image because it must
# always use the newlib toolchain, regardless of --nacl_glibc.  We clone
# it from nacl_env here, before too much other cruft has been added.
# We do some more magic below to instantiate it the way we need it.
nacl_irt_env = nacl_env.Clone(
    BUILD_TYPE = 'nacl_irt',
    BUILD_TYPE_DESCRIPTION = 'NaCl IRT build',
    NACL_BUILD_FAMILY = 'UNTRUSTED_IRT',
)

# Provide access to the IRT build environment from the default environment
# which is needed when compiling custom IRT for testing purposes.
nacl_env['NACL_IRT_ENV'] = nacl_irt_env

# Since we don't build src/untrusted/pthread/nacl.scons in
# nacl_irt_env, we must tell the IRT how to find the pthread.h header.
nacl_irt_env.Append(CPPPATH='${MAIN_DIR}/src/untrusted/pthread')

# Map certain flag bits to suffices on the build output.  This needs to
# happen pretty early, because it affects any concretized directory names.
target_variant_map = [
    ('bitcode', 'pnacl'),
    ('translate_fast', 'fast'),
    ('use_sz', 'subzero'),
    ('nacl_pic', 'pic'),
    ('use_sandboxed_translator', 'sbtc'),
    ('nacl_glibc', 'glibc'),
    ('pnacl_generate_pexe', 'pexe'),
    ('nonsfi_nacl', 'nonsfi'),
    ]
for variant_bit, variant_suffix in target_variant_map:
  if nacl_env.Bit(variant_bit):
    nacl_env['TARGET_VARIANT'] += '-' + variant_suffix

if nacl_env.Bit('bitcode'):
  nacl_env['TARGET_VARIANT'] += '-clang'

nacl_env.Replace(TESTRUNNER_LIBS=['testrunner'])

# TODO(mseaborn): Make nacl-glibc-based static linking work with just
# "-static", without specifying a linker script.
# See http://code.google.com/p/nativeclient/issues/detail?id=1298
def GetLinkerScriptBaseName(env):
  if env.Bit('build_x86_64'):
    return 'elf_x86_64_nacl'
  else:
    return 'elf_i386_nacl'

if (nacl_env.Bit('nacl_glibc') and
    nacl_env.Bit('nacl_static_link')):
  nacl_env.Append(LINKFLAGS=['-static'])
  if nacl_env.Bit('build_x86'):
    # The "-lc" is necessary because libgcc_eh depends on libc but for
    # some reason nacl-gcc is not linking with "--start-group/--end-group".
    nacl_env.Append(LINKFLAGS=[
        '-T', 'ldscripts/%s.x.static' % GetLinkerScriptBaseName(nacl_env),
        '-lc'])

if nacl_env.Bit('running_on_valgrind'):
  nacl_env.Append(CCFLAGS = ['-g', '-Wno-overlength-strings',
                             '-fno-optimize-sibling-calls'],
                  CPPDEFINES = [['DYNAMIC_ANNOTATIONS_ENABLED', '1' ],
                                ['DYNAMIC_ANNOTATIONS_PREFIX', 'NACL_' ]])
  # With GLibC, libvalgrind.so is preloaded at runtime.
  # With Newlib, it has to be linked in.
  if not nacl_env.Bit('nacl_glibc'):
    nacl_env.Append(LINKFLAGS = ['-Wl,-u,have_nacl_valgrind_interceptors'],
                    LIBS = ['valgrind'])

environment_list.append(nacl_env)

if not nacl_env.Bit('nacl_glibc'):
  # These are all specific to nacl-newlib so we do not include them
  # when building against nacl-glibc.  The functionality of
  # pthread/startup/stubs/nosys is provided by glibc.  The valgrind
  # code currently assumes nc_threads.
  nacl_env.Append(
      BUILD_SCONSCRIPTS = [
        ####  ALPHABETICALLY SORTED ####
        'src/untrusted/elf_loader/nacl.scons',
        'src/untrusted/pthread/nacl.scons',
        'src/untrusted/stubs/nacl.scons',
        'src/untrusted/nosys/nacl.scons',
        ####  ALPHABETICALLY SORTED ####
      ])
nacl_env.Append(
    BUILD_SCONSCRIPTS = [
    ####  ALPHABETICALLY SORTED ####
    'src/nonsfi/irt/build.scons',
    'src/nonsfi/linux/nacl.scons',
    'src/nonsfi/loader/build.scons',
    'src/shared/gio/nacl.scons',
    'src/shared/imc/nacl.scons',
    'src/shared/platform/nacl.scons',
    'src/trusted/service_runtime/nacl.scons',
    'src/trusted/validator/nacl.scons',
    'src/untrusted/irt/nacl_headers.scons',
    'src/untrusted/minidump_generator/nacl.scons',
    'src/untrusted/nacl/nacl.scons',
    'src/untrusted/pll_loader/nacl.scons',
    'src/untrusted/pnacl_dynloader/nacl.scons',
    'src/untrusted/valgrind/nacl.scons',
    ####  ALPHABETICALLY SORTED ####
])
nacl_env.AddChromeFilesFromGroup('untrusted_scons_files')

# These are tests that are worthwhile to run in IRT variant only.
irt_only_tests = [
    #### ALPHABETICALLY SORTED ####
    'tests/elf_loader/nacl.scons',
    'tests/irt/nacl.scons',
    'tests/irt_compatibility/nacl.scons',
    'tests/irt_entry_alignment/nacl.scons',
    'tests/irt_ext/nacl.scons',
    'tests/irt_stack_alignment/nacl.scons',
    'tests/sbrk/nacl.scons',
    'tests/translator_size_limits/nacl.scons',
    ]

# These are tests that are worthwhile to run in both IRT and non-IRT variants.
# The nacl_irt_test mode runs them in the IRT variants.
irt_variant_tests = [
    #### ALPHABETICALLY SORTED ####
    'tests/app_lib/nacl.scons',
    'tests/benchmark/nacl.scons',
    'tests/bigalloc/nacl.scons',
    'tests/callingconv/nacl.scons',
    'tests/callingconv_ppapi/nacl.scons',
    'tests/callingconv_case_by_case/nacl.scons',
    'tests/clock/nacl.scons',
    'tests/common/nacl.scons',
    'tests/compiler_thread_suspension/nacl.scons',
    'tests/computed_gotos/nacl.scons',
    'tests/data_below_data_start/nacl.scons',
    'tests/data_not_executable/nacl.scons',
    'tests/debug_stub/nacl.scons',
    'tests/dup/nacl.scons',
    'tests/dynamic_code_loading/nacl.scons',
    'tests/dynamic_linking/nacl.scons',
    'tests/egyptian_cotton/nacl.scons',
    'tests/environment_variables/nacl.scons',
    'tests/exception_test/nacl.scons',
    'tests/fdopen_test/nacl.scons',
    'tests/file/nacl.scons',
    'tests/futexes/nacl.scons',
    'tests/gc_instrumentation/nacl.scons',
    'tests/gdb/nacl.scons',
    'tests/glibc_file64_test/nacl.scons',
    'tests/glibc_static_test/nacl.scons',
    'tests/glibc_syscall_wrappers/nacl.scons',
    'tests/glibc_socket_wrappers/nacl.scons',
    'tests/hello_world/nacl.scons',
    'tests/imc_shm_mmap/nacl.scons',
    'tests/includability/nacl.scons',
    'tests/infoleak/nacl.scons',
    'tests/libc/nacl.scons',
    'tests/libc_free_hello_world/nacl.scons',
    'tests/limited_file_access/nacl.scons',
    'tests/list_mappings/nacl.scons',
    'tests/longjmp/nacl.scons',
    'tests/loop/nacl.scons',
    'tests/math/nacl.scons',
    'tests/memcheck_test/nacl.scons',
    'tests/mmap/nacl.scons',
    'tests/mmap_main_nexe/nacl.scons',
    'tests/mmap_prot_exec/nacl.scons',
    'tests/mmap_race_protect/nacl.scons',
    'tests/nacl_log/nacl.scons',
    'tests/nanosleep/nacl.scons',
    'tests/nonsfi/nacl.scons',
    'tests/noop/nacl.scons',
    'tests/nrd_xfer/nacl.scons',
    'tests/nthread_nice/nacl.scons',
    'tests/null/nacl.scons',
    'tests/nullptr/nacl.scons',
    'tests/pagesize/nacl.scons',
    'tests/performance/nacl.scons',
    'tests/pnacl_abi/nacl.scons',
    'tests/pnacl_dynamic_loading/nacl.scons',
    'tests/pnacl_native_objects/nacl.scons',
    'tests/random/nacl.scons',
    'tests/redir/nacl.scons',
    'tests/rodata_not_writable/nacl.scons',
    'tests/run_py/nacl.scons',
    'tests/sel_ldr/nacl.scons',
    'tests/sel_ldr_seccomp/nacl.scons',
    'tests/sel_main_chrome/nacl.scons',
    'tests/signal_handler/nacl.scons',
    'tests/simd/nacl.scons',
    'tests/sleep/nacl.scons',
    'tests/stack_alignment/nacl.scons',
    'tests/stubout_mode/nacl.scons',
    'tests/sysbasic/nacl.scons',
    'tests/syscall_return_regs/nacl.scons',
    'tests/syscall_return_sandboxing/nacl.scons',
    'tests/syscalls/nacl.scons',
    'tests/thread_capture/nacl.scons',
    'tests/threads/nacl.scons',
    'tests/time/nacl.scons',
    'tests/tls/nacl.scons',
    'tests/tls_segment_x86_32/nacl.scons',
    'tests/toolchain/nacl.scons',
    'tests/toolchain/arm/nacl.scons',
    'tests/toolchain/mips/nacl.scons',
    'tests/unittests/shared/platform/nacl.scons',
    'tests/untrusted_check/nacl.scons',
    'tests/unwind_restores_regs/nacl.scons',
    'tests/validator/nacl.scons',
    #### ALPHABETICALLY SORTED ####
    # NOTE: The following tests are really IRT-only tests, but they
    # are in this category so that they can generate libraries (which
    # works in nacl_env but not in nacl_irt_test_env) while also
    # adding tests to nacl_irt_test_env.
    'tests/inbrowser_test_runner/nacl.scons',
    'tests/untrusted_minidump/nacl.scons',
]

# These are tests that are NOT worthwhile to run in an IRT variant.
# In some cases, that's because they are browser tests which always
# use the IRT.  In others, it's because they are special-case tests
# that are incompatible with having an IRT loaded.
nonvariant_tests = [
    #### ALPHABETICALLY SORTED ####
    'tests/barebones/nacl.scons',
    'tests/chrome_extension/nacl.scons',
    'tests/custom_desc/nacl.scons',
    'tests/faulted_thread_queue/nacl.scons',
    'tests/gold_plugin/nacl.scons',
    'tests/imc_sockets/nacl.scons',
    'tests/minnacl/nacl.scons',
    'tests/multiple_sandboxes/nacl.scons',
    # Potential issue with running them:
    # http://code.google.com/p/nativeclient/issues/detail?id=2092
    # See also the comment in "buildbot/buildbot_standard.py"
    'tests/pnacl_shared_lib_test/nacl.scons',
    'tests/pwrite/nacl.scons',
    'tests/signal_handler_single_step/nacl.scons',
    'tests/thread_suspension/nacl.scons',
    'tests/trusted_crash/crash_in_syscall/nacl.scons',
    'tests/trusted_crash/osx_crash_filter/nacl.scons',
    'tests/trusted_crash/osx_crash_forwarding/nacl.scons',
    'tests/unittests/shared/imc/nacl.scons',
    #### ALPHABETICALLY SORTED ####
]

nacl_env.Append(BUILD_SCONSCRIPTS=nonvariant_tests)
nacl_env.AddChromeFilesFromGroup('nonvariant_test_scons_files')
nacl_env.Append(BUILD_SCONSCRIPTS=irt_variant_tests)
nacl_env.AddChromeFilesFromGroup('irt_variant_test_scons_files')

# Defines TESTS_TO_RUN_INBROWSER.
SConscript('tests/inbrowser_test_runner/selection.scons',
           exports=['nacl_env'])

# Possibly install a toolchain by downloading it
# TODO: explore using a less heavy weight mechanism
# NOTE: this uses stuff from: site_scons/site_tools/naclsdk.py
import SCons.Script

SCons.Script.AddOption('--download',
                       dest='download',
                       metavar='DOWNLOAD',
                       default=False,
                       action='store_true',
                       help='deprecated - allow tools to download')

if nacl_env.GetOption('download'):
  print '@@@@ --download is deprecated, use gclient runhooks --force'
  nacl_sync_env = nacl_env.Clone()
  nacl_sync_env['ENV'] = os.environ
  nacl_sync_env.Execute('gclient runhooks --force')


def NaClSharedLibrary(env, lib_name, *args, **kwargs):
  env_shared = env.Clone(COMPONENT_STATIC=False)
  soname = SCons.Util.adjustixes(lib_name, 'lib', '.so')
  env_shared.AppendUnique(SHLINKFLAGS=['-Wl,-soname,%s' % (soname)])
  return env_shared.ComponentLibrary(lib_name, *args, **kwargs)

nacl_env.AddMethod(NaClSharedLibrary)

def NaClSdkLibrary(env, lib_name, *args, **kwargs):
  n = [env.ComponentLibrary(lib_name, *args, **kwargs)]
  if not env.Bit('nacl_disable_shared'):
    n.append(env.NaClSharedLibrary(lib_name, *args, **kwargs))
  return n

nacl_env.AddMethod(NaClSdkLibrary)


# Special environment for untrusted test binaries that use raw syscalls
def RawSyscallObjects(env, sources):
  raw_syscall_env = env.Clone()
  raw_syscall_env.Append(
    CPPDEFINES = [
      ['USE_RAW_SYSCALLS', '1'],
      ],
  )
  objects = []
  for source_file in sources:
    target_name = 'raw_' + os.path.basename(source_file).rstrip('.c')
    object = raw_syscall_env.ComponentObject(target_name,
                                             source_file)
    objects.append(object)
  return objects

nacl_env.AddMethod(RawSyscallObjects)


# The IRT-building environment was cloned from nacl_env, but it should
# ignore the --nacl_glibc, nacl_pic=1 and bitcode=1 switches.
# We have to reinstantiate the naclsdk.py magic after clearing those flags,
# so it regenerates the tool paths right.
# TODO(mcgrathr,bradnelson): could get cleaner if naclsdk.py got folded back in.
nacl_irt_env.ClearBits('nacl_glibc')
nacl_irt_env.ClearBits('nacl_pic')
nacl_irt_env.ClearBits('pnacl_generate_pexe')
nacl_irt_env.ClearBits('use_sandboxed_translator')
nacl_irt_env.ClearBits('bitcode')
# The choice of toolchain used to build the IRT does not depend on the toolchain
# used to build user/test code. nacl-clang is used everywhere for the IRT.
nacl_irt_env.SetBits('nacl_clang')

nacl_irt_env.Tool('naclsdk')
# These are unfortunately clobbered by running Tool, which
# we needed to do to get the destination directory reset.
# We want all the same values from nacl_env.
nacl_irt_env.Replace(EXTRA_CFLAGS=nacl_env['EXTRA_CFLAGS'],
                     EXTRA_CXXFLAGS=nacl_env['EXTRA_CXXFLAGS'],
                     CCFLAGS=nacl_env['CCFLAGS'],
                     CFLAGS=nacl_env['CFLAGS'],
                     CXXFLAGS=nacl_env['CXXFLAGS'])
FixWindowsAssembler(nacl_irt_env)
# Make it find the libraries it builds, rather than the SDK ones.
nacl_irt_env.Replace(LIBPATH='${LIB_DIR}')

# The IRT must be built using LLVM's assembler on x86-64 to preserve sandbox
# base address hiding. It's also used on x86-32 for consistency.
if nacl_irt_env.Bit('build_x86_64') or nacl_irt_env.Bit('build_x86_32'):
  nacl_irt_env.Append(CCFLAGS=['-integrated-as'])
if nacl_irt_env.Bit('build_x86_32'):
  # The x86-32 IRT needs to be callable with an under-aligned stack.
  # See  https://code.google.com/p/nativeclient/issues/detail?id=3935
  nacl_irt_env.Append(CCFLAGS=['-mstackrealign', '-mno-sse'])

# The IRT is C only, don't link with the C++ linker so that it doesn't
# start depending on the C++ standard library and (in the case of
# libc++) pthread.
nacl_irt_env.Replace(LINK=(nacl_irt_env['LINK'].
                           replace('nacl-clang++', 'nacl-clang')))

# TODO(mcgrathr): Clean up uses of these methods.
def AddLibraryDummy(env, nodes):
  return nodes
nacl_irt_env.AddMethod(AddLibraryDummy, 'AddLibraryToSdk')

def AddObjectInternal(env, nodes):
  return env.Replicate('${LIB_DIR}', nodes)
nacl_env.AddMethod(AddObjectInternal, 'AddObjectToSdk')
nacl_irt_env.AddMethod(AddObjectInternal, 'AddObjectToSdk')

def IrtNaClSdkLibrary(env, lib_name, *args, **kwargs):
  env.ComponentLibrary(lib_name, *args, **kwargs)
nacl_irt_env.AddMethod(IrtNaClSdkLibrary, 'NaClSdkLibrary')

nacl_irt_env.AddMethod(SDKInstallBin)

# Populate the internal include directory when AddHeaderToSdk
# is used inside nacl_env.
def AddHeaderInternal(env, nodes, subdir='nacl'):
  dir = '${INCLUDE_DIR}'
  if subdir is not None:
    dir += '/' + subdir
  n = env.Replicate(dir, nodes)
  return n

nacl_irt_env.AddMethod(AddHeaderInternal, 'AddHeaderToSdk')

def PublishHeader(env, nodes, subdir):
  if ('install' in COMMAND_LINE_TARGETS or
      'install_headers' in COMMAND_LINE_TARGETS):
    dir = env.GetAbsDirArg('includedir', 'install_headers')
    if subdir is not None:
      dir += '/' + subdir
    n = env.Install(dir, nodes)
    env.Alias('install', env.Alias('install_headers', n))
    return n

def PublishLibrary(env, nodes):
  env.Alias('build_lib', nodes)

  if ('install' in COMMAND_LINE_TARGETS or
      'install_lib' in COMMAND_LINE_TARGETS):
    dir = env.GetAbsDirArg('libdir', 'install_lib')
    n = env.Install(dir, nodes)
    env.Alias('install', env.Alias('install_lib', n))
    return n

def NaClAddHeader(env, nodes, subdir='nacl'):
  n = AddHeaderInternal(env, nodes, subdir)
  PublishHeader(env, n, subdir)
  return n
nacl_env.AddMethod(NaClAddHeader, 'AddHeaderToSdk')

def NaClAddLibrary(env, nodes):
  nodes = env.Replicate('${LIB_DIR}', nodes)
  PublishLibrary(env, nodes)
  return nodes
nacl_env.AddMethod(NaClAddLibrary, 'AddLibraryToSdk')

def NaClAddObject(env, nodes):
  lib_nodes = env.Replicate('${LIB_DIR}', nodes)
  PublishLibrary(env, lib_nodes)
  return lib_nodes
nacl_env.AddMethod(NaClAddObject, 'AddObjectToSdk')

# We want to do this for nacl_env when not under --nacl_glibc,
# but for nacl_irt_env whether or not under --nacl_glibc, so
# we do it separately for each after making nacl_irt_env and
# clearing its Bit('nacl_glibc').
def AddImplicitLibs(env):
  implicit_libs = []

  # Require the pnacl_irt_shim for pnacl x86-64 and arm.
  # Use -B to have the compiler look for the fresh libpnacl_irt_shim.a.
  if ( env.Bit('bitcode') and
       (env.Bit('build_x86_64') or env.Bit('build_arm'))
       and env['NACL_BUILD_FAMILY'] != 'UNTRUSTED_IRT'):
    # Note: without this hack ibpnacl_irt_shim.a will be deleted
    #       when "built_elsewhere=1"
    #       Since we force the build in a previous step the dependency
    #       is not really needed.
    #       Note: the "precious" mechanism did not work in this case
    if not env.Bit('built_elsewhere'):
      if env.Bit('enable_chrome_side'):
        implicit_libs += ['libpnacl_irt_shim.a']

  if not env.Bit('nacl_glibc'):
    # These are automatically linked in by the compiler, either directly
    # or via the linker script that is -lc.  In the non-glibc build, we
    # are the ones providing these files, so we need dependencies.
    # The ComponentProgram method (site_scons/site_tools/component_builders.py)
    # adds dependencies on env['IMPLICIT_LIBS'] if that's set.
    if env.Bit('bitcode'):
      implicit_libs += ['libnacl.a']
    else:
      implicit_libs += ['crt1.o',
                        'libnacl.a',
                        'crti.o',
                        'crtn.o']
      # TODO(mcgrathr): multilib nonsense defeats -B!  figure out a better way.
      if GetTargetPlatform() == 'x86-32':
        implicit_libs.append(os.path.join('32', 'crt1.o'))
    # libc++ depends on libpthread, and because PPAPI applications always need
    # threads anyway, nacl-clang just includes -lpthread unconditionally.
    if using_nacl_libcxx and env['NACL_BUILD_FAMILY'] != 'UNTRUSTED_IRT':
      implicit_libs += ['libpthread.a']

  if implicit_libs != []:
    env['IMPLICIT_LIBS'] = [env.File(os.path.join('${LIB_DIR}', file))
                            for file in implicit_libs]
    # The -B<dir>/ flag is necessary to tell gcc to look for crt[1in].o there.
    env.Prepend(LINKFLAGS=['-B${LIB_DIR}/'])

AddImplicitLibs(nacl_env)
AddImplicitLibs(nacl_irt_env)

nacl_irt_env.Append(
    BUILD_SCONSCRIPTS = [
        'src/shared/gio/nacl.scons',
        'src/shared/platform/nacl.scons',
        'src/tools/tls_edit/build.scons',
        'src/untrusted/elf_loader/nacl.scons',
        'src/untrusted/irt/nacl.scons',
        'src/untrusted/nacl/nacl.scons',
        'src/untrusted/stubs/nacl.scons',
        'tests/irt_private_pthread/nacl.scons',
    ])
nacl_irt_env.AddChromeFilesFromGroup('untrusted_irt_scons_files')

environment_list.append(nacl_irt_env)

# Since browser_tests already use the IRT normally, those are fully covered
# in nacl_env.  But the non_browser_tests don't use the IRT in nacl_env.
# We want additional variants of those tests with the IRT, so we make
# another environment and repeat them with that adjustment.
nacl_irt_test_env = nacl_env.Clone(
    BUILD_TYPE = 'nacl_irt_test',
    BUILD_TYPE_DESCRIPTION = 'NaCl tests build with IRT',
    NACL_BUILD_FAMILY = 'UNTRUSTED_IRT_TESTS',
    NACL_ENV = nacl_env,

    INCLUDE_DIR = nacl_env.Dir('${INCLUDE_DIR}'),
    LIB_DIR = nacl_env.Dir('${LIB_DIR}'),
    BUILD_SCONSCRIPTS = [],
    )
nacl_irt_test_env.SetBits('tests_use_irt')
if nacl_irt_test_env.Bit('enable_chrome_side'):
  nacl_irt_test_env.Replace(TESTRUNNER_LIBS=['testrunner_browser'])

nacl_irt_test_env.Append(BUILD_SCONSCRIPTS=irt_variant_tests)
nacl_irt_test_env.AddChromeFilesFromGroup('irt_variant_test_scons_files')
nacl_irt_test_env.Append(BUILD_SCONSCRIPTS=irt_only_tests)
TestsUsePublicLibs(nacl_irt_test_env)
TestsUsePublicListMappingsLib(nacl_irt_test_env)

# We add the following settings after creating nacl_irt_test_env because we
# don't want them to be inherited by nacl_irt_test_env.
if nacl_env.Bit('nonsfi_nacl'):
  if nacl_env.Bit('pnacl_generate_pexe'):
    # Not-IRT-using non-SFI code uses Linux syscalls directly.  Since this
    # involves using inline assembly, this requires turning off the PNaCl ABI
    # checker.
    nacl_env.SetBits('nonstable_bitcode')
    nacl_env.Append(LINKFLAGS=['--pnacl-disable-abi-check'])
    # Tell the PNaCl translator to link a Linux executable.
    nacl_env.Append(TRANSLATEFLAGS=['--noirt'])
  else:
    nacl_env.Append(LINKFLAGS=['--pnacl-allow-native', '-Wt,--noirt'])

# If a tests/.../nacl.scons file builds a library, we will just use
# the one already built in nacl_env instead.
def IrtTestDummyLibrary(*args, **kwargs):
  pass
nacl_irt_test_env.AddMethod(IrtTestDummyLibrary, 'ComponentLibrary')

def IrtTestAddNodeToTestSuite(env, node, suite_name, node_name=None,
                              is_broken=False, is_flaky=False,
                              disable_irt_suffix=False):
  # The disable_irt_suffix argument is there for allowing tests
  # defined in nacl_irt_test_env to be part of chrome_browser_tests
  # (rather than part of chrome_browser_tests_irt).
  # TODO(mseaborn): But really, all of chrome_browser_tests should be
  # placed in nacl_irt_test_env rather than in nacl_env.
  suite_name = AddImplicitTestSuites(suite_name, node_name)
  if not disable_irt_suffix:
    if node_name is not None:
      node_name += '_irt'
    suite_name = [name + '_irt' for name in suite_name]
  # NOTE: This needs to be called directly to as we're overriding the
  #       prior version.
  return AddNodeToTestSuite(env, node, suite_name, node_name,
                            is_broken, is_flaky)
nacl_irt_test_env.AddMethod(IrtTestAddNodeToTestSuite, 'AddNodeToTestSuite')

environment_list.append(nacl_irt_test_env)


windows_coverage_env = windows_debug_env.Clone(
    tools = ['code_coverage'],
    BUILD_TYPE = 'coverage-win',
    BUILD_TYPE_DESCRIPTION = 'Windows code coverage build',
    # TODO(bradnelson): switch nacl to common testing process so this won't be
    #    needed.
    MANIFEST_FILE = None,
    COVERAGE_ANALYZER_DIR=r'..\third_party\coverage_analyzer\bin',
    COVERAGE_ANALYZER='$COVERAGE_ANALYZER_DIR\coverage_analyzer.exe',
)
# TODO(bradnelson): Switch nacl to common testing process so this won't be
#                   needed. Ignoring instrumentation failure as that's easier
#                   than trying to gate out the ones with asm we can't handle.
windows_coverage_env['LINKCOM'] = windows_coverage_env.Action([
    windows_coverage_env.get('LINKCOM', []),
    '-$COVERAGE_VSINSTR /COVERAGE ${TARGET}'])
windows_coverage_env.Append(LINKFLAGS = ['/NODEFAULTLIB:msvcrt'])
AddDualLibrary(windows_coverage_env)
environment_list.append(windows_coverage_env)

mac_coverage_env = mac_debug_env.Clone(
    tools = ['code_coverage'],
    BUILD_TYPE = 'coverage-mac',
    BUILD_TYPE_DESCRIPTION = 'MacOS code coverage build',
    # Strict doesnt't currently work for coverage because the path to gcov is
    # magically baked into the compiler.
    LIBS_STRICT = False,
    )
AddDualLibrary(mac_coverage_env)
environment_list.append(mac_coverage_env)

linux_coverage_env = linux_debug_env.Clone(
    tools = ['code_coverage'],
    BUILD_TYPE = 'coverage-linux',
    BUILD_TYPE_DESCRIPTION = 'Linux code coverage build',
    # Strict doesnt't currently work for coverage because the path to gcov is
    # magically baked into the compiler.
    LIBS_STRICT = False,
)

linux_coverage_env.FilterOut(CCFLAGS=['-fPIE'])
linux_coverage_env.Append(CCFLAGS=['-fPIC'])

linux_coverage_env['OPTIONAL_COVERAGE_LIBS'] = '$COVERAGE_LIBS'
AddDualLibrary(linux_coverage_env)
environment_list.append(linux_coverage_env)


# Environment Massaging
RELEVANT_CONFIG = ['NACL_BUILD_FAMILY',
                   'BUILD_TYPE',
                   'TARGET_ROOT',
                   'OBJ_ROOT',
                   'BUILD_TYPE_DESCRIPTION',
                   ]

MAYBE_RELEVANT_CONFIG = ['BUILD_OS',
                         'BUILD_ARCHITECTURE',
                         'BUILD_SUBARCH',
                         'TARGET_OS',
                         'TARGET_ARCHITECTURE',
                         'TARGET_SUBARCH',
                         ]

def DumpCompilerVersion(cc, env):
  if 'gcc' in cc:
    env.Execute(env.Action('set'))
    env.Execute(env.Action('${CC} -v -c'))
    env.Execute(env.Action('${CC} -print-search-dirs'))
    env.Execute(env.Action('${CC} -print-libgcc-file-name'))
  elif cc.startswith('cl'):
    import subprocess
    try:
      p = subprocess.Popen(env.subst('${CC} /V'),
                           bufsize=1000*1000,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE)
      stdout, stderr = p.communicate()
      print stderr[0:stderr.find("\r")]
    except WindowsError:
      # If vcvars was not run before running SCons, we won't be able to find
      # the compiler at this point.  SCons has built in functions for finding
      # the compiler, but they haven't run yet.
      print 'Can not find the compiler, assuming SCons will find it later.'
  else:
    print "UNKNOWN COMPILER"


def SanityCheckEnvironments(all_envs):
  # simple completeness check
  for env in all_envs:
    for tag in RELEVANT_CONFIG:
      assert tag in env, repr(tag)
      assert env[tag], repr(env[tag])


def LinkTrustedEnv(selected_envs):
  # Collect build families and ensure that we have only one env per family.
  family_map = {}
  for env in selected_envs:
    family = env['NACL_BUILD_FAMILY']
    if family not in family_map:
      family_map[family] = env
    else:
      msg = 'You are using incompatible environments simultaneously\n'
      msg += '%s vs %s\n' % (env['BUILD_TYPE'],
                             family_map[family]['BUILD_TYPE'])
      msg += ('Please specfy the exact environments you require, e.g. '
              'MODE=dbg-host,nacl')
      raise Exception(msg)

  # Set TRUSTED_ENV so that tests of untrusted code can locate sel_ldr
  # etc.  We set this on trusted envs too because some tests on
  # trusted envs run sel_ldr (e.g. using checked-in binaries).
  if 'TRUSTED' in family_map:
    for env in selected_envs:
      env['TRUSTED_ENV'] = family_map['TRUSTED']
      # Propagate some environment variables from the trusted environment,
      # in case some (e.g. Mac's DYLD_LIBRARY_PATH) are necessary for
      # running sel_ldr et al in untrusted environments' tests.
      for var in env['TRUSTED_ENV'].get('PROPAGATE_ENV', []):
        env['ENV'][var] = env['TRUSTED_ENV']['ENV'][var]
  if 'TRUSTED' not in family_map or 'UNTRUSTED' not in family_map:
    Banner('Warning: "--mode" did not specify both trusted and untrusted '
           'build environments.  As a result, many tests will not be run.')

def MakeBuildEnv():
  build_platform = GetBuildPlatform()

  # Build Platform Base Function
  platform_func_map = {
      'win32' : MakeWindowsEnv,
      'cygwin': MakeWindowsEnv,
      'linux' : MakeGenericLinuxEnv,
      'linux2': MakeGenericLinuxEnv,
      'darwin': MakeMacEnv,
      }
  if sys.platform not in platform_func_map:
    raise UserError('Unrecognized host platform: %s', sys.platform)
  make_env_func = platform_func_map[sys.platform]

  build_env = make_env_func(build_platform)
  build_env['IS_BUILD_ENV'] = True

  # Building tls_edit depends on gio, platform, and validator_ragel.
  build_env['BUILD_SCONSCRIPTS'] = [
    # KEEP THIS SORTED PLEASE
    'src/shared/gio/build.scons',
    'src/shared/platform/build.scons',
    'src/trusted/validator_ragel/build.scons',
    ]

  # The build environment is only used for intermediate steps and should
  # not be creating any targets. Aliases are used as means to add targets
  # to builds (IE, all_programs, all_libraries...etc.). Since we want to
  # share all of our build scripts but not define any aliases, we should
  # override the alias function and essentially stub it out.
  build_env.Alias = lambda env, target, source=[], actions=None, **kw : []

  return build_env

def LinkBuildEnv(selected_envs):
  build_env_map = {
    'opt': opt_build_env,
    'dbg': dbg_build_env,
    }

  # We need to find the optimization level in order to know which
  # build environment we want to use
  opt_level = None
  for env in selected_envs:
    if env.get('OPTIMIZATION_LEVEL', None):
      opt_level = env['OPTIMIZATION_LEVEL']
      break

  build_env = build_env_map.get(opt_level, opt_build_env)
  for env in selected_envs:
    env['BUILD_ENV'] = build_env

  # If the build environment is different from all the selected environments,
  # we will need to also append it to the selected environments so the targets
  # can be built.
  build_env_root = build_env.subst('${TARGET_ROOT}')
  for env in selected_envs:
    if build_env_root == env.subst('${TARGET_ROOT}'):
      break
  else:
    # Did not find a matching environment, append the build environment now.
    selected_envs.append(build_env)

def DumpEnvironmentInfo(selected_envs):
  if VerboseConfigInfo(pre_base_env):
    Banner("The following environments have been configured")
    for env in selected_envs:
      for tag in RELEVANT_CONFIG:
        assert tag in env, repr(tag)
        print "%s:  %s" % (tag, env.subst(env.get(tag)))
      for tag in MAYBE_RELEVANT_CONFIG:
        print "%s:  %s" % (tag, env.subst(env.get(tag)))
      cc = env.subst('${CC}')
      print 'CC:', cc
      asppcom = env.subst('${ASPPCOM}')
      print 'ASPPCOM:', asppcom
      DumpCompilerVersion(cc, env)
      print
    rev_file = 'toolchain/linux_x86/pnacl_newlib_raw/REV'
    if os.path.exists(rev_file):
      for line in open(rev_file).read().split('\n'):
        if "Revision:" in line:
          print "PNACL : %s" % line

def PnaclSetEmulatorForSandboxedTranslator(selected_envs):
  # Slip in emulator flags if necessary, for the sandboxed pnacl translator
  # on ARM, once emulator is actually known (vs in naclsdk.py, where it
  # is not yet known).
  for env in selected_envs:
    if (env['NACL_BUILD_FAMILY'] != 'TRUSTED'
        and env.Bit('bitcode')
        and env.Bit('use_sandboxed_translator')
        and env.UsingEmulator()):
      # This must modify the LINK command itself, since LINKFLAGS may
      # be filtered (e.g., in barebones tests).
      env.Append(LINK=' --pnacl-use-emulator')
      env.Append(TRANSLATE=' --pnacl-use-emulator')


# Blank out defaults.
Default(None)

# Apply optional supplement if present in the directory tree.
if os.path.exists(pre_base_env.subst('$MAIN_DIR/supplement/supplement.scons')):
  SConscript('supplement/supplement.scons', exports=['environment_list'])

# print sytem info (optionally)
if VerboseConfigInfo(pre_base_env):
  Banner('SCONS ARGS:' + str(sys.argv))
  os.system(pre_base_env.subst('${PYTHON} tools/sysinfo.py'))

CheckArguments()

SanityCheckEnvironments(environment_list)
selected_envs = FilterEnvironments(environment_list)

# If we are building NaCl, build nacl_irt too.  This works around it being
# a separate mode due to the vagaries of scons when we'd really rather it
# not be, while not requiring that every bot command line using --mode be
# changed to list '...,nacl,nacl_irt' explicitly.
if nacl_env in selected_envs:
  selected_envs.append(nacl_irt_env)

# The nacl_irt_test_env requires nacl_env to build things correctly.
if nacl_irt_test_env in selected_envs and nacl_env not in selected_envs:
  selected_envs.append(nacl_env)

DumpEnvironmentInfo(selected_envs)
LinkTrustedEnv(selected_envs)

# When building NaCl, any intermediate build tool that is used during the
# build process must be built using the current build environment, not the
# target. Create a build environment for this purpose and link it into
# the selected environments
dbg_build_env, opt_build_env = GenerateOptimizationLevels(MakeBuildEnv())
LinkBuildEnv(selected_envs)

# This must happen after LinkTrustedEnv, since that is where TRUSTED_ENV
# is finally set, and env.UsingEmulator() checks TRUSTED_ENV for the emulator.
# This must also happen before BuildEnvironments.
PnaclSetEmulatorForSandboxedTranslator(selected_envs)

BuildEnvironments(selected_envs)

# Change default to build everything, but not run tests.
Default(['all_programs', 'all_bundles', 'all_test_programs', 'all_libraries'])


# Sanity check whether we are ready to build nacl modules
# NOTE: this uses stuff from: site_scons/site_tools/naclsdk.py
if nacl_env.Bit('naclsdk_validate') and (nacl_env in selected_envs or
                                         nacl_irt_env in selected_envs):
  nacl_env.ValidateSdk()

if BROKEN_TEST_COUNT > 0:
  msg = "There are %d broken tests." % BROKEN_TEST_COUNT
  if GetOption('brief_comstr'):
    msg += " Add --verbose to the command line for more information."
  print msg

# separate warnings from actual build output
Banner('B U I L D - O U T P U T:')
