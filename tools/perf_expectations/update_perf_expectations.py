#!/usr/bin/env python
# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Prepare tests that require re-baselining for input to make_expectations.py.

The regularly running perf-AV tests require re-baselineing of expectations
about once a week. The steps involved in rebaselining are:

1.) Identify the tests to update, based off reported e-mail results.
2.) Figure out reva and revb values, which is the starting and ending revision
 numbers for the range that we should use to obtain new thresholds.
3.) Modify lines in perf_expectations.json referring to the tests to be updated,
 so that they may be used as input to make_expectations.py.

This script automates the last step above.

Here's a sample line from perf_expectations.json:

"win-release/media_tests_av_perf/fps/tulip2.m4a": {"reva": 163299, \
"revb": 164141, "type": "absolute", "better": "higher", "improve": 0, \
"regress": 0, "sha1": "54d94538"},

To get the above test ready for input to make_expectations.py, it should become:

"win-release/media_tests_av_perf/fps/tulip2.m4a": {"reva": <new reva>, \
"revb": <new revb>, "type": "absolute", "better": "higher", "improve": 0, \
"regress": 0},

Examples:

1.) To update the test specified above and get baseline
values using the revision range 12345 and 23456, run this script with a command
line like this:
  python update_perf_expectations.py -f \
  win-release/media_tests_av_perf/fps/tulip2.m4a --reva 12345 --revb 23456
Or, using an input file,
where the input file contains a single line with text
  win-release/media_tests_av_perf/fps/tulip2.m4a
run with this command line:
  python update_perf_expectations.py -i input.txt --reva 12345 --revb 23456

2.) Let's say you want to update all seek tests on windows, and get baseline
values using the revision range 12345 and 23456.
Run this script with this command line:
  python update_perf_expectations.py -f win-release/media_tests_av_perf/seek/ \
   --reva 12345 --revb 23456
Or:
  python update_perf_expectations.py -f win-release/.*/seek/ --reva 12345 \
  --revb 23456

Or, using an input file,
where the input file contains a single line with text win-release/.*/seek/:
  python update_perf_expectations.py -i input.txt --reva 12345 --revb 23456

3.) Similarly, if you want to update seek tests on all platforms
  python update_perf_expectations.py -f .*-release/.*/seek/ --reva 12345 \
  --revb 23456

"""

import logging
from optparse import OptionParser
import os
import re

import make_expectations as perf_ex_lib

# Default logging is INFO. Use --verbose to enable DEBUG logging.
_DEFAULT_LOG_LEVEL = logging.INFO


def GetTestsToUpdate(contents, all_test_keys):
  """Parses input contents and obtains tests to be re-baselined.

  Args:
    contents: string containing contents of input file.
    all_test_keys: list of keys of test dictionary.
  Returns:
    A list of keys for tests that should be updated.
  """
  # Each line of the input file specifies a test case to update.
  tests_list = []
  for test_case_filter in contents.splitlines():
    # Skip any empty lines.
    if test_case_filter:
      # Sample expected line:
      # win-release/media_tests_av_perf/seek/\
      # CACHED_BUFFERED_SEEK_NoConstraints_crowd1080.ogv
      # Or, if reg-ex, then sample line:
      # win-release/media-tests_av_perf/seek*
      # Skip any leading spaces if they exist in the input file.
      logging.debug('Trying to match %s', test_case_filter)
      tests_list.extend(GetMatchingTests(test_case_filter.strip(),
                                         all_test_keys))
  return tests_list


def GetMatchingTests(tests_to_update, all_test_keys):
  """Parses input reg-ex filter and obtains tests to be re-baselined.

  Args:
    tests_to_update: reg-ex string specifying tests to be updated.
    all_test_keys: list of keys of tests dictionary.
  Returns:
    A list of keys for tests that should be updated.
  """
  tests_list = []
  search_string = re.compile(tests_to_update)
  # Get matching tests from the dictionary of tests
  for test_key in all_test_keys:
    if search_string.match(test_key):
      tests_list.append(test_key)
      logging.debug('%s will be updated', test_key)
  logging.info('%s tests found matching reg-ex: %s', len(tests_list),
               tests_to_update)
  return tests_list


def PrepareTestsForUpdate(tests_to_update, all_tests, reva, revb):
  """Modifies value of tests that are to re-baselined:
     Set reva and revb values to specified new values. Remove sha1.

  Args:
    tests_to_update: list of tests to be updated.
    all_tests: dictionary of all tests.
    reva: oldest revision in range to use for new values.
    revb: newest revision in range to use for new values.
  Raises:
    ValueError: If reva or revb are not valid ints, or if either
    of them are negative.
  """
  reva = int(reva)
  revb = int(revb)

  if reva < 0 or revb < 0:
    raise ValueError('Revision values should be positive.')
  # Ensure reva is less than revb.
  # (this is similar to the check done in make_expectations.py)
  if revb < reva:
    temp = revb
    revb = reva
    reva = temp
  for test_key in tests_to_update:
    # Get original test from the dictionary of tests
    test_value = all_tests[test_key]
    if test_value:
      # Sample line in perf_expectations.json:
      #  "linux-release/media_tests _av_perf/dropped_frames/crowd360.webm":\
      # {"reva": 155180, "revb": 155280, "type": "absolute", \
      # "better": "lower", "improve": 0, "regress": 3, "sha1": "276ba29c"},
      # Set new revision range
      test_value['reva'] = reva
      test_value['revb'] = revb
      # Remove sha1 to indicate this test requires an update
      # Check first to make sure it exist.
      if 'sha1' in test_value:
        del test_value['sha1']
    else:
      logging.warning('%s does not exist.', test_key)
  logging.info('Done preparing tests for update.')


def GetCommandLineOptions():
  """Parse command line arguments.

  Returns:
    An options object containing command line arguments and their values.
  """
  parser = OptionParser()

  parser.add_option('--reva', dest='reva', type='int',
                    help='Starting revision of new range.',
                    metavar='START_REVISION')
  parser.add_option('--revb', dest='revb', type='int',
                    help='Ending revision of new range.',
                    metavar='END_REVISION')
  parser.add_option('-f', dest='tests_filter',
                    help='Regex to use for filtering tests to be updated. '
                    'At least one of -filter or -input_file must be provided. '
                    'If both are provided, then input-file is used.',
                    metavar='FILTER', default='')
  parser.add_option('-i', dest='input_file',
                    help='Optional path to file with reg-exes for tests to'
                    ' update. If provided, it overrides the filter argument.',
                    metavar='INPUT_FILE', default='')
  parser.add_option('--config', dest='config_file',
                    default=perf_ex_lib.DEFAULT_CONFIG_FILE,
                    help='Set the config file to FILE.', metavar='FILE')
  parser.add_option('-v', dest='verbose', action='store_true', default=False,
                    help='Enable verbose output.')
  options = parser.parse_args()[0]
  return options


def Main():
  """Main driver function."""
  options = GetCommandLineOptions()

  _SetLogger(options.verbose)
  # Do some command-line validation
  if not options.input_file and not options.tests_filter:
    logging.error('At least one of input-file or test-filter must be provided.')
    exit(1)
  if options.input_file and options.tests_filter:
    logging.error('Specify only one of input file or test-filter.')
    exit(1)
  if not options.reva or not options.revb:
    logging.error('Start and end revision of range must be specified.')
    exit(1)

  # Load config.
  config = perf_ex_lib.ConvertJsonIntoDict(
      perf_ex_lib.ReadFile(options.config_file))

  # Obtain the perf expectations file from the config file.
  perf_file = os.path.join(
      os.path.dirname(options.config_file), config['perf_file'])

  # We should have all the information we require now.
  # On to the real thang.
  # First, get all the existing tests from the original perf_expectations file.
  all_tests = perf_ex_lib.ConvertJsonIntoDict(
      perf_ex_lib.ReadFile(perf_file))
  all_test_keys = all_tests.keys()
  # Remove the load key, because we don't want to modify it.
  all_test_keys.remove('load')
  # Keep tests sorted, like in the original file.
  all_test_keys.sort()

  # Next, get all tests that have been identified for an update.
  tests_to_update = []
  if options.input_file:
    # Tests to update have been specified in an input_file.
    # Get contents of file.
    tests_filter = perf_ex_lib.ReadFile(options.input_file)
  elif options.tests_filter:
    # Tests to update have been specified as a reg-ex filter.
    tests_filter = options.tests_filter

  # Get tests to update based on filter specified.
  tests_to_update = GetTestsToUpdate(tests_filter, all_test_keys)
  logging.info('Done obtaining matching tests.')

  # Now, prepare tests for update.
  PrepareTestsForUpdate(tests_to_update, all_tests, options.reva, options.revb)

  # Finally, write modified tests back to perf_expectations file.
  perf_ex_lib.WriteJson(perf_file, all_tests, all_test_keys,
                        calculate_sha1=False)
  logging.info('Done writing tests for update to %s.', perf_file)


def _SetLogger(verbose):
  log_level = _DEFAULT_LOG_LEVEL
  if verbose:
    log_level = logging.DEBUG
  logging.basicConfig(level=log_level, format='%(message)s')


if __name__ == '__main__':
  Main()
