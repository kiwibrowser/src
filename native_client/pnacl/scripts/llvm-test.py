#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This script runs the LLVM regression tests and the LLVM testsuite.
   These tests are tightly coupled to the LLVM build, and require that
   LLVM has been built on this host by build.sh.  It also assumes that
   the test suite source has been checked out using gclient (build.sh
   git-sync).

   The testsuite must be configured, then run, then reported.
   Currently it requires clean in between runs of different arches.

   The regression tests require nothing more than running 'make check'
   in the build directory, but currently not all of the upstream tests
   pass in our source tree, so we currently use the same
   known-failures mechanism that the testsuite uses. Once we eliminate
   the locally-caused failures, we should expect 'make check' to
   always pass and can get rid of the regression known failures.
"""

import contextlib
import datetime
import os
import optparse
import shutil
import subprocess
import sys
import parse_llvm_test_report

sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))
import pynacl.platform

@contextlib.contextmanager
def remember_cwd():
  """Provides a shell 'pushd'/'popd' pattern.

  Use as:
      with remember_cwd():
        os.chdir(...)
        ...
      # Original cwd restored here
  """
  curdir = os.getcwd()
  try:
    yield
  finally:
    os.chdir(curdir)


def ParseCommandLine(argv):
  usage = """%prog [options]

Specify the tests or test subsets in the options; common tests are
--llvm-regression and --testsuite-all.

The --opt arguments control the frontend/backend optimization flags.
The default set is {O3f,O2b}, other options are {O0f,O0b,O2b_sz,O0b_sz}.
"""
  parser = optparse.OptionParser(usage=usage)
  parser.add_option('--arch', dest='arch',
                    help=('Architecture to test, e.g. x86-32, x86-64, arm; ' +
                          'required for most tests'))
  parser.add_option('--opt', dest='opt_attributes', action='append',
                    default=[],
                    help=('Add optimization level attribute of ' +
                          'test configuration'))
  parser.add_option('--llvm-regression', dest='run_llvm_regression',
                    action='store_true', default=False,
                    help='Run the LLVM regression tests')
  parser.add_option('--libcxx-tests', dest='run_libcxx_tests',
                    action='store_true', default=False,
                    help='Run the libc++ tests')
  parser.add_option('--testsuite-clean', dest='testsuite_clean',
                    action='store_true', default=False,
                    help='Clean the testsuite build directory')
  parser.add_option('--testsuite-prereq', dest='testsuite_prereq',
                    action='store_true', default=False,
                    help='Build the testsuite prerequisites')
  parser.add_option('--testsuite-configure', dest='testsuite_configure',
                    action='store_true', default=False,
                    help='Configure the testsuite build directory')
  parser.add_option('--testsuite-run', dest='testsuite_run',
                    action='store_true', default=False,
                    help='Run the testsuite (requires <arch> argument)')
  parser.add_option('--testsuite-report', dest='testsuite_report',
                    action='store_true', default=False,
                    help=('Generate the testsuite report ' +
                          '(requires <arch> argument)'))
  parser.add_option('--testsuite-all', dest='testsuite_all',
                    action='store_true', default=False,
                    help='Run all testsuite steps (requires <arch> argument)')
  parser.add_option('--llvm-buildpath', dest='llvm_buildpath',
                    help='Path to the LLVM build directory')
  parser.add_option('-v', '--verbose', action='store_true',
                    default=False, dest='verbose',
                    help=('[--testsuite-report/regression option] ' +
                          'Print compilation/run logs of failing tests in'
                          'testsuite report and print all regression output'))
  # The following options are specific to parse_llvm_test_report.
  parser.add_option('-x', '--exclude', action='append', dest='excludes',
                    default=[],
                    help=('[--testsuite-report option] ' +
                          'Add list of excluded tests (expected fails)'))
  parser.add_option('-c', '--check-excludes', action='store_true',
                    default=False, dest='check_excludes',
                    help=('[--testsuite-report option] ' +
                          'Report tests which unexpectedly pass'))
  parser.add_option('-p', '--build-path', dest='buildpath',
                    help=('[--testsuite-report option] ' +
                          'Path to test-suite build directory'))
  parser.add_option('-a', '--attribute', dest='attributes', action='append',
                    default=[],
                    help=('[--testsuite-report option] ' +
                          'Add attribute of test configuration (e.g. arch)'))
  parser.add_option('-t', '--testsuite', action='store_true', dest='testsuite',
                    default=False,
                    help=('[--testsuite-report option] ' +
                          'Signify LLVM testsuite tests'))
  parser.add_option('-l', '--lit', action='store_true', dest='lit',
                    default=False,
                    help=('[--testsuite-report option] ' +
                          'Signify LLVM LIT regression tests'))

  options, args = parser.parse_args(argv)
  return options, args


def Fatal(text):
  """Prints an error message and exits."""
  print >> sys.stderr, text
  sys.exit(1)


def ParseConfig(options):
  """Constructs a frontend/backend dict based on --opt arguments.

  Args:
    options: The result of OptionParser().parse_args().

  Returns:
    A simple dict containing keys 'frontend_opt', 'frontend_attr',
    'backend_opt', and 'backend_attr', each mapped to a valid string
    value.  The result is a function of the --opt command-line
    arguments, with defaults in place when there are too few --opt
    arguments.
  """
  configs = dict(O0f={'frontend_opt': '-O0', 'frontend_attr': 'O0f'},
                 O3f={'frontend_opt': '-O3', 'frontend_attr': 'O3f'},
                 O0b={'backend_opt': '-translate-fast',
                      'backend_attr': 'O0b'},
                 O2b={'backend_opt': '-O2', 'backend_attr': 'O2b'},
                 O0b_sz={'backend_opt': '-translate-fast --use-sz',
                         'backend_attr': 'O0b_sz'},
                 O2b_sz={'backend_opt': '-O2 --use-sz',
                         'backend_attr': 'O2b_sz'},
                 )
  result = {}
  # Default is pnacl-clang -O3, pnacl-translate -O2
  for attr in ['O3f', 'O2b'] + options.opt_attributes:
    if attr in configs:
      result.update(configs[attr])
  return result


def GetConfigSuffix(config):
  """Create a string to be used as a file suffix.

  Args:
    config: A dict that was the result of ParseConfig().

  Returns:
    A string that concatenates the frontend and backend attributes.
  """
  return config['frontend_attr'] + '_' + config['backend_attr']

def SetupEnvironment(options):
  """Create an environment.

  This is based on the current system, various defaults, and various
  environment variables.

  Args:
    options: The result of OptionParser.parse_args()
  Returns:
    A dict with various string->string mappings.
  """
  env = {}
  pwd = os.getcwd()
  if not pwd.endswith(os.sep + 'native_client'):
    Fatal("ERROR: must be run in native_client/ directory!\n" +
          "       (Current directory is " + pwd + ")")
  # Simulate what's needed from common-tools.sh.
  # We need PNACL_BUILDBOT, BUILD_PLATFORM, and HOST_ARCH.
  # TODO(dschuff): This should come from toolchain_build or the upcoming common
  # python infrastructure.
  env['PNACL_BUILDBOT'] = os.environ.get('PNACL_BUILDBOT', 'false')
  if sys.platform == 'linux2':
    env['BUILD_PLATFORM'] = 'linux'
    env['BUILD_ARCH'] = os.environ.get(
        'BUILD_ARCH',
        'x86_64' if pynacl.platform.IsArch64Bit() else 'i686')
    env['HOST_ARCH'] = os.environ.get('HOST_ARCH', env['BUILD_ARCH'])
    env['HOST_TRIPLE'] = env['HOST_ARCH'] + '_linux'
  elif sys.platform == 'cygwin':
    env['BUILD_PLATFORM'] = 'win'
    env['HOST_ARCH'] = os.environ.get('HOST_ARCH', 'x86_32')
    env['HOST_TRIPLE'] = 'i686_pc_cygwin'
  elif sys.platform == 'darwin':
    env['BUILD_PLATFORM'] = 'mac'
    env['HOST_ARCH'] = os.environ.get('HOST_ARCH', 'x86_64')
    env['HOST_TRIPLE'] = 'x86_64_apple_darwin'
  elif sys.platform == 'win32':
    env['BUILD_PLATFORM'] = 'win'
    env['HOST_ARCH'] = os.environ.get('HOST_ARCH', 'x86_64')
    env['HOST_TRIPLE'] = 'i686_w64_mingw32'
    # TODO(dschuff) unify this with toolchain_build_pnacl
    msys_path = os.environ.get(
        'MSYS',
        os.path.join(os.getcwd(), 'mingw32', 'msys', 'bin'))
    os.environ['PATH'] = os.pathsep.join([os.environ['PATH'], msys_path])
  else:
    Fatal("Unknown system " + sys.platform)
  if env['HOST_ARCH'] in ['i386', 'i686']:
    env['HOST_ARCH'] = 'x86_32'


  # Set up the rest of the environment.
  env['NACL_ROOT'] = pwd
  env['LLVM_TESTSUITE_SRC'] = (
    '{NACL_ROOT}/toolchain_build/src/llvm-test-suite'.format(**env))
  env['LLVM_TESTSUITE_BUILD'] = (
    '{NACL_ROOT}/pnacl/build/llvm-test-suite'.format(**env))
  env['TC_SRC_LLVM'] = (
    '{NACL_ROOT}/toolchain_build/src/llvm'.format(**env))
  env['TC_BUILD_LLVM'] = options.llvm_buildpath or (
    '{NACL_ROOT}/toolchain_build/out/llvm_{HOST_TRIPLE}_work'.format(**env))
  env['TC_BUILD_LIBCXX'] = (
    ('{NACL_ROOT}/toolchain_build/out/' +
     'libcxx_le32_work/').format(**env))
  env['PNACL_CONCURRENCY'] = os.environ.get('PNACL_CONCURRENCY', '8')

  # The toolchain used may not be the one downloaded, but one that is freshly
  # built into a different directory,
  # Overriding the default here will not affect the sel_ldr
  # and IRT used to run the tests (they are controlled by run.py)
  env['PNACL_TOOLCHAIN_DIR'] = (
    os.environ.get('PNACL_TOOLCHAIN_DIR',
                   '{BUILD_PLATFORM}_x86/pnacl_newlib'.format(**env)))
  env['PNACL_BIN'] = (
    '{NACL_ROOT}/toolchain/{PNACL_TOOLCHAIN_DIR}/bin'.format(**env))
  env['PNACL_SDK_DIR'] = (
    '{NACL_ROOT}/toolchain/{PNACL_TOOLCHAIN_DIR}/le32-nacl/lib'
    .format(**env))
  env['PNACL_SCRIPTS'] = '{NACL_ROOT}/pnacl/scripts'.format(**env)
  env['LLVM_REGRESSION_KNOWN_FAILURES'] = (
      '{pwd}/pnacl/scripts/llvm_regression_known_failures.txt'.format(pwd=pwd))
  env['LIBCXX_KNOWN_FAILURES'] = (
      '{pwd}/pnacl/scripts/libcxx_known_failures.txt'.format(pwd=pwd))
  return env


def ToolchainWorkDirExists(work_dir):
  # TODO(dschuff): Because this script is run directly from the buildbot
  # script and not as part of a toolchain_build rule, we do not know
  # whether the llvm target was actually built (in which case the working
  # directory is still there) or whether it was just retrieved from cache
  # (in which case it was clobbered, since the bots run with --clobber).
  # Check if ninja or make rule exists.
  return (os.path.isfile(os.path.join(work_dir, 'build.ninja')) or
          os.path.isfile(os.path.join(work_dir, 'Makefile')))


def RunLitTest(testdir, testarg, lit_failures, env, options):
  """Run LLVM lit tests, and check failures against known failures.

  Args:
    testdir: Directory with the make/ninja file to test.
    testarg: argument to pass to make/ninja.
    env: The result of SetupEnvironment().
    options: The result of OptionParser().parse_args().

  Returns:
    0 always
  """
  with remember_cwd():
    if not ToolchainWorkDirExists(testdir):
      print 'Working directory %s is empty. Not running tests' % testdir
      if env['PNACL_BUILDBOT'] != 'false' or options.verbose:
        print '@@@STEP_TEXT (skipped)@@@'
      return 0
    os.chdir(testdir)

    sub_env = os.environ.copy()
    # Tell run.py to use the architecture specified by --arch, or the
    # current host architecture if none was provided.
    sub_env['PNACL_RUN_ARCH'] = options.arch or env['HOST_ARCH']

    maker = 'ninja' if os.path.isfile('./build.ninja') else 'make'
    cmd = [maker, testarg, '-v' if maker == 'ninja' else 'VERBOSE=1']
    print 'Running lit test:', ' '.join(cmd)
    make_pipe = subprocess.Popen(cmd, env=sub_env, stdout=subprocess.PIPE)

    lines = []
    # When run by a buildbot, we need to incrementally tee the 'make'
    # stdout to our stdout, rather than collect its entire stdout and
    # print it at the end.  Otherwise the watchdog may try to kill the
    # process after too long without any output.
    #
    # Note: We use readline instead of 'for line in make_pipe.stdout'
    # because otherwise the process on the Cygwin bot seems to hang
    # when the 'make' process completes (with slightly truncated
    # output).  The readline avoids buffering when reading from a
    # pipe in Python 2, which may be complicit in the problem.
    for line in iter(make_pipe.stdout.readline, ''):
      if env['PNACL_BUILDBOT'] != 'false' or options.verbose:
        # The buildbots need to be fully verbose and print all output.
        print str(datetime.datetime.now()) + ' ' + line,
      lines.append(line)
    print (str(datetime.datetime.now()) + ' ' +
           "Waiting for '%s' to complete." % cmd)
    make_pipe.wait()
    make_stdout = ''.join(lines)

    parse_options = vars(options)
    parse_options['lit'] = True
    parse_options['excludes'].append(env[lit_failures])
    parse_options['attributes'].append(env['BUILD_PLATFORM'])
    parse_options['attributes'].append(env['HOST_ARCH'])
    print (str(datetime.datetime.now()) + ' ' +
           'Parsing LIT test report output.')
    ret = parse_llvm_test_report.Report(parse_options, filecontents=make_stdout)
  return ret


def EnsureSdkExists(env):
  """Ensure that a build of the SDK exists.  Exits if not.

  Args:
    env: The result of SetupEnvironment().
  """
  libnacl_path = os.path.join(env['PNACL_SDK_DIR'], 'libnacl.a')
  if not os.path.isfile(libnacl_path):
    Fatal("""
ERROR: libnacl does not seem to exist in %s
ERROR: have you run 'pnacl/build.sh sdk' ?
    """ % libnacl_path)


def TestsuitePrereq(env, options):
  """Run the LLVM test suite prerequisites.

  Args:
    env: The result of SetupEnvironment().
    options: The result of OptionParser().parse_args().

  Returns:
    0 for success, non-zero integer on failure.
  """
  arch = options.arch or Fatal("Error: missing --arch argument")
  return subprocess.call(['./scons',
                          'platform=' + arch,
                          'irt_core',
                          'sel_ldr',
                          'elf_loader',
                          '-j{PNACL_CONCURRENCY}'.format(**env)])


def TestsuiteRun(env, config, options):
  """Run the LLVM test suite.

  Args:
    env: The result of SetupEnvironment().
    config: A dict that was the result of ParseConfig().  This
        determines the specific optimization levels.
    options: The result of OptionParser().parse_args().

  Returns:
    0 for success, non-zero integer on failure.
  """
  arch = options.arch or Fatal("Error: missing --arch argument")
  EnsureSdkExists(env)
  suffix = GetConfigSuffix(config)
  opt_clang = config['frontend_opt']
  opt_trans = config['backend_opt']
  build_path = env['LLVM_TESTSUITE_BUILD']
  if not os.path.isdir(build_path):
    os.makedirs(build_path)
  with remember_cwd():
    os.chdir(build_path)
    if not os.path.exists('Makefile'):
      result = TestsuiteConfigure(env)
      if result:
        return result
    result = subprocess.call(['make',
                              '-j{PNACL_CONCURRENCY}'.format(**env),
                              'OPTFLAGS=' + opt_clang,
                              'PNACL_TRANSLATE_FLAGS=' + opt_trans,
                              'PNACL_BIN={PNACL_BIN}'.format(**env),
                              'PNACL_RUN={NACL_ROOT}/run.py'.format(**env),
                              'COLLATE=true',
                              'PNACL_ARCH=' + arch,
                              'ENABLE_PARALLEL_REPORT=true',
                              'DISABLE_CBE=true',
                              'DISABLE_JIT=true',
                              'RUNTIMELIMIT=850',
                              'TEST=pnacl',
                              'report.csv'])
    if result:
      return result
    os.rename('report.pnacl.csv', 'report.pnacl.{arch}.{suffix}.csv'
              .format(arch=arch, suffix=suffix))
    os.rename('report.pnacl.raw.out',
              ('report.pnacl.{arch}.{suffix}.raw.out'
               .format(arch=arch, suffix=suffix)))
  return 0


def TestsuiteConfigure(env):
  """Run the LLVM test suite configure script.

  Args:
    env: The result of SetupEnvironment().

  Returns:
    0 for success, non-zero integer on failure.
  """
  build_path = env['LLVM_TESTSUITE_BUILD']
  if not os.path.isdir(build_path):
    os.makedirs(build_path)
  with remember_cwd():
    os.chdir(build_path)
    args = ['{LLVM_TESTSUITE_SRC}/configure'.format(**env),
            '--with-llvmcc=clang',
            '--with-clang={PNACL_BIN}/pnacl-clang'.format(**env),
            '--with-llvmsrc={TC_SRC_LLVM}'.format(**env),
            '--with-llvmobj={TC_BUILD_LLVM}'.format(**env)]
    result = subprocess.call(args)
  return result


def TestsuiteClean(env):
  """Clean the LLVM test suite build directory.

  Args:
    env: The result of SetupEnvironment().

  Returns:
    0 always

  Raises:
    OSError: The LLVM_TESTSUITE_BUILD directory couldn't be removed
    for some reason.
  """
  if os.path.isdir(env['LLVM_TESTSUITE_BUILD']):
    shutil.rmtree(env['LLVM_TESTSUITE_BUILD'])
  elif os.path.isfile(env['LLVM_TESTSUITE_BUILD']):
    os.remove(env['LLVM_TESTSUITE_BUILD'])
  return 0


def TestsuiteReport(env, config, options):
  """Generate a report from the prior LLVM test suite run.

  Args:
    env: The result of SetupEnvironment().
    config: A dict that was the result of ParseConfig().  This
        determines the specific optimization levels.
    options: The result of OptionParser().parse_args().

  Returns:
    0 for success, non-zero integer on failure.
  """
  arch = options.arch or Fatal("Error: missing --arch argument")
  suffix = GetConfigSuffix(config)
  report_file = ('{LLVM_TESTSUITE_BUILD}/report.pnacl.{arch}.{suffix}.csv'
                 .format(arch=arch, suffix=suffix, **env))
  failures1 = '{PNACL_SCRIPTS}/testsuite_known_failures_base.txt'.format(**env)
  failures2 = '{PNACL_SCRIPTS}/testsuite_known_failures_pnacl.txt'.format(**env)
  parse_options = vars(options)
  parse_options['excludes'].extend([failures1, failures2])
  parse_options['buildpath'] = env['LLVM_TESTSUITE_BUILD']
  parse_options['attributes'].extend([arch,
                                      config['frontend_attr'],
                                      config['backend_attr']])
  parse_options['testsuite'] = True
  return parse_llvm_test_report.Report(parse_options, filename=report_file)


def RunTestsuiteSteps(env, config, options):
  result = 0
  if not ToolchainWorkDirExists(env['TC_BUILD_LLVM']):
    print ('LLVM build directory %s is empty. Skipping testsuite' %
           env['TC_BUILD_LLVM'])
    if env['PNACL_BUILDBOT'] != 'false' or options.verbose:
      print '@@@STEP_TEXT (skipped)@@@'
    return result
  if options.testsuite_all or options.testsuite_prereq:
    result = result or TestsuitePrereq(env, options)
  if options.testsuite_all or options.testsuite_clean:
    result = result or TestsuiteClean(env)
  if options.testsuite_all or options.testsuite_configure:
    result = result or TestsuiteConfigure(env)
  if options.testsuite_all or options.testsuite_run:
    result = result or TestsuiteRun(env, config, options)
  if options.testsuite_all or options.testsuite_report:
    result = result or TestsuiteReport(env, config, options)
  return result


def main(argv):
  options, args = ParseCommandLine(argv[1:])
  if len(args):
    Fatal("Unknown arguments: " + ', '.join(args))
  config = ParseConfig(options)
  env = SetupEnvironment(options)
  result = 0
  # Run each specified test in sequence, and return on the first failure.
  if options.run_llvm_regression:
    result = result or RunLitTest(env['TC_BUILD_LLVM'], 'check-all',
                                  'LLVM_REGRESSION_KNOWN_FAILURES',
                                  env, options)
  if options.run_libcxx_tests:
    EnsureSdkExists(env)
    result = result or RunLitTest(env['TC_BUILD_LIBCXX'], 'check-libcxx',
                                  'LIBCXX_KNOWN_FAILURES',
                                  env, options)

  result = result or RunTestsuiteSteps(env, config, options)
  return result

if __name__ == '__main__':
  sys.exit(main(sys.argv))
