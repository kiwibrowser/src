#!/usr/bin/env python
# Copyright 2017 The PDFium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generates a coverage report for given binaries using llvm-gcov & lcov.

Requires llvm-cov 3.5 or later.
Requires lcov 1.11 or later.
Requires that 'use_coverage = true' is set in args.gn.
"""

import argparse
from collections import namedtuple
import os
import pprint
import re
import subprocess
import sys


# Add src dir to path to avoid having to set PYTHONPATH.
sys.path.append(
    os.path.abspath(
       os.path.join(
          os.path.dirname(__file__),
          os.path.pardir,
          os.path.pardir,
          os.path.pardir)))

from testing.tools.common import GetBooleanGnArg


# 'binary' is the file that is to be run for the test.
# 'use_test_runner' indicates if 'binary' depends on test_runner.py and thus
# requires special handling.
TestSpec = namedtuple('TestSpec', 'binary, use_test_runner')

# All of the coverage tests that the script knows how to run.
COVERAGE_TESTS = {
    'pdfium_unittests': TestSpec('pdfium_unittests', False),
    'pdfium_embeddertests': TestSpec('pdfium_embeddertests', False),
    'corpus_tests': TestSpec('run_corpus_tests.py', True),
    'javascript_tests': TestSpec('run_javascript_tests.py', True),
    'pixel_tests': TestSpec('run_pixel_tests.py', True),
}

# Coverage tests that are known to take a long time to run, so are not in the
# default set. The user must either explicitly invoke these tests or pass in
# --slow.
SLOW_TESTS = ['corpus_tests', 'javascript_tests', 'pixel_tests']

class CoverageExecutor(object):

  def __init__(self, parser, args):
    """Initialize executor based on the current script environment

    Args:
        parser: argparse.ArgumentParser for handling improper inputs.
        args: Dictionary of arguments passed into the calling script.
    """
    self.dry_run = args['dry_run']
    self.verbose = args['verbose']

    llvm_cov = self.determine_proper_llvm_cov()
    if not llvm_cov:
      print 'Unable to find appropriate llvm-cov to use'
      sys.exit(1)
    self.lcov_env = os.environ
    self.lcov_env['LLVM_COV_BIN'] = llvm_cov

    self.lcov = self.determine_proper_lcov()
    if not self.lcov:
      print 'Unable to find appropriate lcov to use'
      sys.exit(1)

    self.coverage_files = set()
    self.source_directory = args['source_directory']
    if not os.path.isdir(self.source_directory):
      parser.error("'%s' needs to be a directory" % self.source_directory)

    self.build_directory = args['build_directory']
    if not os.path.isdir(self.build_directory):
      parser.error("'%s' needs to be a directory" % self.build_directory)

    self.coverage_tests = self.calculate_coverage_tests(args)
    if not self.coverage_tests:
      parser.error(
          'No valid tests in set to be run. This is likely due to bad command '
          'line arguments')

    if not GetBooleanGnArg('use_coverage', self.build_directory, self.verbose):
      parser.error(
          'use_coverage does not appear to be set to true for build, but is '
          'needed')

    self.use_goma = GetBooleanGnArg('use_goma', self.build_directory,
                                    self.verbose)

    self.output_directory = args['output_directory']
    if not os.path.exists(self.output_directory):
      if not self.dry_run:
        os.makedirs(self.output_directory)
    elif not os.path.isdir(self.output_directory):
      parser.error('%s exists, but is not a directory' % self.output_directory)
    self.coverage_totals_path = os.path.join(self.output_directory,
                                             'pdfium_totals.info')

  def check_output(self, args, dry_run=False, env=None):
    """Dry run aware wrapper of subprocess.check_output()"""
    if dry_run:
      print "Would have run '%s'" % ' '.join(args)
      return ''

    output = subprocess.check_output(args, env=env)

    if self.verbose:
      print "check_output(%s) returned '%s'" % (args, output)
    return output

  def call(self, args, dry_run=False, env=None):
    """Dry run aware wrapper of subprocess.call()"""
    if dry_run:
      print "Would have run '%s'" % ' '.join(args)
      return 0

    output = subprocess.call(args, env=env)

    if self.verbose:
      print 'call(%s) returned %s' % (args, output)
    return output

  def call_lcov(self, args, dry_run=False, needs_directory=True):
    """Wrapper to call lcov that adds appropriate arguments as needed."""
    lcov_args = [
        self.lcov, '--config-file',
        os.path.join(self.source_directory, 'testing', 'tools', 'coverage',
                     'lcovrc'),
        '--gcov-tool',
        os.path.join(self.source_directory, 'testing', 'tools', 'coverage',
                     'llvm-gcov')
    ]
    if needs_directory:
      lcov_args.extend(['--directory', self.source_directory])
    if not self.verbose:
      lcov_args.append('--quiet')
    lcov_args.extend(args)
    return self.call(lcov_args, dry_run=dry_run, env=self.lcov_env)

  def calculate_coverage_tests(self, args):
    """Determine which tests should be run."""
    testing_tools_directory = os.path.join(self.source_directory, 'testing',
                                           'tools')
    coverage_tests = {}
    for name in COVERAGE_TESTS.keys():
      test_spec = COVERAGE_TESTS[name]
      if test_spec.use_test_runner:
        binary_path = os.path.join(testing_tools_directory, test_spec.binary)
      else:
        binary_path = os.path.join(self.build_directory, test_spec.binary)
      coverage_tests[name] = TestSpec(binary_path, test_spec.use_test_runner)

    if args['tests']:
      return {name: spec
        for name, spec in coverage_tests.iteritems() if name in args['tests']}
    elif not args['slow']:
      return {name: spec
        for name, spec in coverage_tests.iteritems() if name not in SLOW_TESTS}
    else:
      return coverage_tests

  def find_acceptable_binary(self, binary_name, version_regex,
                             min_major_version, min_minor_version):
    """Find the newest version of binary that meets the min version."""
    min_version = (min_major_version, min_minor_version)
    parsed_versions = {}
    # When calling Bash builtins like this the command and arguments must be
    # passed in as a single string instead of as separate list members.
    potential_binaries = self.check_output(
        ['bash', '-c', 'compgen -abck %s' % binary_name]).splitlines()
    for binary in potential_binaries:
      if self.verbose:
        print 'Testing llvm-cov binary, %s' % binary
      # Assuming that scripts that don't respond to --version correctly are not
      # valid binaries and just happened to get globbed in. This is true for
      # lcov and llvm-cov
      try:
        version_output = self.check_output([binary, '--version']).splitlines()
      except subprocess.CalledProcessError:
        if self.verbose:
          print '--version returned failure status 1, so ignoring'
        continue

      for line in version_output:
        matcher = re.match(version_regex, line)
        if matcher:
          parsed_version = (int(matcher.group(1)), int(matcher.group(2)))
          if parsed_version >= min_version:
            parsed_versions[parsed_version] = binary
          break

    if not parsed_versions:
      return None
    return parsed_versions[max(parsed_versions)]

  def determine_proper_llvm_cov(self):
    """Find a version of llvm_cov that will work with the script."""
    version_regex = re.compile('.*LLVM version ([\d]+)\.([\d]+).*')
    return self.find_acceptable_binary('llvm-cov', version_regex, 3, 5)

  def determine_proper_lcov(self):
    """Find a version of lcov that will work with the script."""
    version_regex = re.compile('.*LCOV version ([\d]+)\.([\d]+).*')
    return self.find_acceptable_binary('lcov', version_regex, 1, 11)

  def build_binaries(self):
    """Build all the binaries that are going to be needed for coverage
    generation."""
    call_args = ['ninja']
    if self.use_goma:
      call_args.extend(['-j', '250'])
    call_args.extend(['-C', self.build_directory])
    return self.call(call_args, dry_run=self.dry_run) == 0

  def generate_coverage(self, name, spec):
    """Generate the coverage data for a test

    Args:
        name: Name associated with the test to be run. This is used as a label
              in the coverage data, so should be unique across all of the tests
              being run.
        spec: Tuple containing the path to the binary to run, and if this test
              uses test_runner.py.
    """
    if self.verbose:
      print "Generating coverage for test '%s', using data '%s'" % (name, spec)
    if not os.path.exists(spec.binary):
      print('Unable to generate coverage for %s, since it appears to not exist'
            ' @ %s') % (name, spec.binary)
      return False

    if self.call_lcov(['--zerocounters'], dry_run=self.dry_run):
      print 'Unable to clear counters for %s' % name
      return False

    binary_args = [spec.binary]
    if spec.use_test_runner:
      # Test runner performs multi-threading in the wrapper script, not the test
      # binary, so need -j 1, otherwise multiple processes will be writing to
      # the code coverage files, invalidating results.
      # TODO(pdfium:811): Rewrite how test runner tests work, so that they can
      # be run in multi-threaded mode.
      binary_args.extend(['-j', '1', '--build-dir', self.build_directory])
    if self.call(binary_args, dry_run=self.dry_run) and self.verbose:
      print('Running %s appears to have failed, which might affect '
            'results') % spec.binary

    output_raw_path = os.path.join(self.output_directory, '%s_raw.info' % name)
    if self.call_lcov(
        ['--capture', '--test-name', name, '--output-file', output_raw_path],
        dry_run=self.dry_run):
      print 'Unable to capture coverage data for %s' % name
      return False

    output_filtered_path = os.path.join(self.output_directory,
                                        '%s_filtered.info' % name)
    output_filters = [
        '/usr/include/*', '*third_party*', '*testing*', '*_unittest.cpp',
        '*_embeddertest.cpp'
    ]
    if self.call_lcov(
        ['--remove', output_raw_path] + output_filters +
        ['--output-file', output_filtered_path],
        dry_run=self.dry_run,
        needs_directory=False):
      print 'Unable to filter coverage data for %s' % name
      return False

    self.coverage_files.add(output_filtered_path)
    return True

  def merge_coverage(self):
    """Merge all of the coverage data sets into one for report generation."""
    merge_args = []
    for coverage_file in self.coverage_files:
      merge_args.extend(['--add-tracefile', coverage_file])

    merge_args.extend(['--output-file', self.coverage_totals_path])
    return self.call_lcov(
        merge_args, dry_run=self.dry_run, needs_directory=False) == 0

  def generate_report(self):
    """Produce HTML coverage report based on combined coverage data set."""
    config_file = os.path.join(
        self.source_directory, 'testing', 'tools', 'coverage', 'lcovrc')

    lcov_args = ['genhtml',
      '--config-file', config_file,
      '--legend',
      '--demangle-cpp',
      '--show-details',
      '--prefix', self.source_directory,
      '--ignore-errors',
      'source', self.coverage_totals_path,
      '--output-directory', self.output_directory]
    return self.call(lcov_args, dry_run=self.dry_run) == 0

  def run(self):
    """Setup environment, execute the tests and generate coverage report"""
    if not self.build_binaries():
      print 'Failed to successfully build binaries'
      return False

    for name in self.coverage_tests.keys():
      if not self.generate_coverage(name, self.coverage_tests[name]):
        print 'Failed to successfully generate coverage data'
        return False

    if not self.merge_coverage():
      print 'Failed to successfully merge generated coverage data'
      return False

    if not self.generate_report():
      print 'Failed to successfully generated coverage report'
      return False

    return True


def main():
  parser = argparse.ArgumentParser()
  parser.formatter_class = argparse.RawDescriptionHelpFormatter
  parser.description = ('Generates a coverage report for given binaries using '
                        'llvm-cov & lcov.\n\n'
                        'Requires llvm-cov 3.5 or later.\n'
                        'Requires lcov 1.11 or later.\n\n'
                        'By default runs pdfium_unittests and '
                        'pdfium_embeddertests. If --slow is passed in then all '
                        'tests will be run. If any of the tests are specified '
                        'on the command line, then only those will be run.')
  parser.add_argument(
      '-s',
      '--source_directory',
      help='Location of PDFium source directory, defaults to CWD',
      default=os.getcwd())
  build_default = os.path.join('out', 'Coverage')
  parser.add_argument(
      '-b',
      '--build_directory',
      help=
      'Location of PDFium build directory with coverage enabled, defaults to '
      '%s under CWD' % build_default,
      default=os.path.join(os.getcwd(), build_default))
  output_default = 'coverage_report'
  parser.add_argument(
      '-o',
      '--output_directory',
      help='Location to write out coverage report to, defaults to %s under CWD '
      % output_default,
      default=os.path.join(os.getcwd(), output_default))
  parser.add_argument(
      '-n',
      '--dry-run',
      help='Output commands instead of executing them',
      action='store_true')
  parser.add_argument(
      '-v',
      '--verbose',
      help='Output additional diagnostic information',
      action='store_true')
  parser.add_argument(
      '--slow',
      help='Run all tests, even those known to take a long time. Ignored if '
      'specific tests are passed in.',
      action='store_true')
  parser.add_argument(
      'tests',
      help='Tests to be run, defaults to all. Valid entries are %s' %
      COVERAGE_TESTS.keys(),
      nargs='*')

  args = vars(parser.parse_args())
  if args['verbose']:
    pprint.pprint(args)

  executor = CoverageExecutor(parser, args)
  if executor.run():
    return 0
  return 1


if __name__ == '__main__':
  sys.exit(main())
