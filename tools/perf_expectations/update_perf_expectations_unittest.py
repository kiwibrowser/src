#!/usr/bin/env python
# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Unit tests for update_perf_expectations."""
import copy
from StringIO import StringIO
import unittest
import make_expectations as perf_ex_lib
import update_perf_expectations as upe_mod


# A separate .json file contains the list of test cases we'll use.
# The tests used to be defined inline here, but are >80 characters in length.
# Now they are expected to be defined in file ./sample_test_cases.json.
# Create a dictionary of tests using .json file.
all_tests = perf_ex_lib.ConvertJsonIntoDict(
    perf_ex_lib.ReadFile('sample_test_cases.json'))
# Get all keys.
all_tests_keys = all_tests.keys()


def VerifyPreparedTests(self, tests_to_update, reva, revb):
  # Work with a copy of the set of tests.
  all_tests_copy = copy.deepcopy(all_tests)
  upe_mod.PrepareTestsForUpdate(tests_to_update, all_tests_copy, reva, revb)
  # Make sure reva < revb
  if reva > revb:
    temp = reva
    reva = revb
    revb = temp
  # Run through all tests and make sure only those that were
  # specified to be modified had their 'sha1' value removed.
  for test_key in all_tests_keys:
    new_test_value = all_tests_copy[test_key]
    original_test_value = all_tests[test_key]
    if test_key in tests_to_update:
      # Make sure there is no "sha1".
      self.assertFalse('sha1' in new_test_value)
      # Make sure reva and revb values are correctly set.
      self.assertEqual(reva, new_test_value['reva'])
      self.assertEqual(revb, new_test_value['revb'])
    else:
      # Make sure there is an "sha1" value
      self.assertTrue('sha1' in new_test_value)
      # Make sure the sha1, reva and revb values have not changed.
      self.assertEqual(original_test_value['sha1'], new_test_value['sha1'])
      self.assertEqual(original_test_value['reva'], new_test_value['reva'])
      self.assertEqual(original_test_value['revb'], new_test_value['revb'])


class UpdatePerfExpectationsTest(unittest.TestCase):
  def testFilterMatch(self):
    """Verifies different regular expressions test filter."""
    self.maxDiff = None
    # Tests to update specified by a single literal string.
    tests_to_update = 'win-release/media_tests_av_perf/fps/tulip2.webm'
    expected_tests_list = ['win-release/media_tests_av_perf/fps/tulip2.webm']
    self.assertEqual(expected_tests_list,
                     upe_mod.GetMatchingTests(tests_to_update,
                                              all_tests_keys))

    # Tests to update specified by a single reg-ex
    tests_to_update = 'win-release/media_tests_av_perf/fps.*'
    expected_tests_list = ['win-release/media_tests_av_perf/fps/crowd1080.webm',
                           'win-release/media_tests_av_perf/fps/crowd2160.webm',
                           'win-release/media_tests_av_perf/fps/crowd360.webm',
                           'win-release/media_tests_av_perf/fps/crowd480.webm',
                           'win-release/media_tests_av_perf/fps/crowd720.webm',
                           'win-release/media_tests_av_perf/fps/tulip2.m4a',
                           'win-release/media_tests_av_perf/fps/tulip2.mp3',
                           'win-release/media_tests_av_perf/fps/tulip2.mp4',
                           'win-release/media_tests_av_perf/fps/tulip2.ogg',
                           'win-release/media_tests_av_perf/fps/tulip2.ogv',
                           'win-release/media_tests_av_perf/fps/tulip2.wav',
                           'win-release/media_tests_av_perf/fps/tulip2.webm']
    actual_list = upe_mod.GetMatchingTests(tests_to_update,
                                           all_tests_keys)
    actual_list.sort()
    self.assertEqual(expected_tests_list, actual_list)

    # Tests to update are specified by a single reg-ex, spanning multiple OSes.
    tests_to_update = '.*-release/media_tests_av_perf/fps.*'
    expected_tests_list = ['linux-release/media_tests_av_perf/fps/tulip2.m4a',
                           'linux-release/media_tests_av_perf/fps/tulip2.mp3',
                           'linux-release/media_tests_av_perf/fps/tulip2.mp4',
                           'linux-release/media_tests_av_perf/fps/tulip2.ogg',
                           'linux-release/media_tests_av_perf/fps/tulip2.ogv',
                           'linux-release/media_tests_av_perf/fps/tulip2.wav',
                           'win-release/media_tests_av_perf/fps/crowd1080.webm',
                           'win-release/media_tests_av_perf/fps/crowd2160.webm',
                           'win-release/media_tests_av_perf/fps/crowd360.webm',
                           'win-release/media_tests_av_perf/fps/crowd480.webm',
                           'win-release/media_tests_av_perf/fps/crowd720.webm',
                           'win-release/media_tests_av_perf/fps/tulip2.m4a',
                           'win-release/media_tests_av_perf/fps/tulip2.mp3',
                           'win-release/media_tests_av_perf/fps/tulip2.mp4',
                           'win-release/media_tests_av_perf/fps/tulip2.ogg',
                           'win-release/media_tests_av_perf/fps/tulip2.ogv',
                           'win-release/media_tests_av_perf/fps/tulip2.wav',
                           'win-release/media_tests_av_perf/fps/tulip2.webm']
    actual_list = upe_mod.GetMatchingTests(tests_to_update,
                                           all_tests_keys)
    actual_list.sort()
    self.assertEqual(expected_tests_list, actual_list)

  def testLinesFromInputFile(self):
    """Verifies different string formats specified in input file."""

    # Tests to update have been specified by a single literal string in
    # an input file.
    # Use the StringIO class to mock a file object.
    lines_from_file = StringIO(
        'win-release/media_tests_av_perf/fps/tulip2.webm')
    contents = lines_from_file.read()
    expected_tests_list = ['win-release/media_tests_av_perf/fps/tulip2.webm']
    actual_list = upe_mod.GetTestsToUpdate(contents, all_tests_keys)
    actual_list.sort()
    self.assertEqual(expected_tests_list, actual_list)
    lines_from_file.close()

    # Tests to update specified by a single reg-ex in an input file.
    lines_from_file = StringIO('win-release/media_tests_av_perf/fps/tulip2.*\n')
    contents = lines_from_file.read()
    expected_tests_list = ['win-release/media_tests_av_perf/fps/tulip2.m4a',
                           'win-release/media_tests_av_perf/fps/tulip2.mp3',
                           'win-release/media_tests_av_perf/fps/tulip2.mp4',
                           'win-release/media_tests_av_perf/fps/tulip2.ogg',
                           'win-release/media_tests_av_perf/fps/tulip2.ogv',
                           'win-release/media_tests_av_perf/fps/tulip2.wav',
                           'win-release/media_tests_av_perf/fps/tulip2.webm']
    actual_list = upe_mod.GetTestsToUpdate(contents, all_tests_keys)
    actual_list.sort()
    self.assertEqual(expected_tests_list, actual_list)
    lines_from_file.close()

    # Tests to update specified by multiple lines in an input file.
    lines_from_file = StringIO(
        '.*-release/media_tests_av_perf/fps/tulip2.*\n'
        'win-release/media_tests_av_perf/dropped_fps/tulip2.*\n'
        'linux-release/media_tests_av_perf/audio_latency/latency')
    contents = lines_from_file.read()
    expected_tests_list = [
        'linux-release/media_tests_av_perf/audio_latency/latency',
        'linux-release/media_tests_av_perf/fps/tulip2.m4a',
        'linux-release/media_tests_av_perf/fps/tulip2.mp3',
        'linux-release/media_tests_av_perf/fps/tulip2.mp4',
        'linux-release/media_tests_av_perf/fps/tulip2.ogg',
        'linux-release/media_tests_av_perf/fps/tulip2.ogv',
        'linux-release/media_tests_av_perf/fps/tulip2.wav',
        'win-release/media_tests_av_perf/dropped_fps/tulip2.wav',
        'win-release/media_tests_av_perf/dropped_fps/tulip2.webm',
        'win-release/media_tests_av_perf/fps/tulip2.m4a',
        'win-release/media_tests_av_perf/fps/tulip2.mp3',
        'win-release/media_tests_av_perf/fps/tulip2.mp4',
        'win-release/media_tests_av_perf/fps/tulip2.ogg',
        'win-release/media_tests_av_perf/fps/tulip2.ogv',
        'win-release/media_tests_av_perf/fps/tulip2.wav',
        'win-release/media_tests_av_perf/fps/tulip2.webm']
    actual_list = upe_mod.GetTestsToUpdate(contents, all_tests_keys)
    actual_list.sort()
    self.assertEqual(expected_tests_list, actual_list)
    lines_from_file.close()

  def testPreparingForUpdate(self):
    """Verifies that tests to be modified are changed as expected."""
    tests_to_update = [
        'linux-release/media_tests_av_perf/audio_latency/latency',
        'linux-release/media_tests_av_perf/fps/tulip2.m4a',
        'linux-release/media_tests_av_perf/fps/tulip2.mp3',
        'linux-release/media_tests_av_perf/fps/tulip2.mp4',
        'linux-release/media_tests_av_perf/fps/tulip2.ogg',
        'linux-release/media_tests_av_perf/fps/tulip2.ogv',
        'linux-release/media_tests_av_perf/fps/tulip2.wav',
        'win-release/media_tests_av_perf/dropped_fps/tulip2.wav',
        'win-release/media_tests_av_perf/dropped_fps/tulip2.webm',
        'win-release/media_tests_av_perf/fps/tulip2.mp3',
        'win-release/media_tests_av_perf/fps/tulip2.mp4',
        'win-release/media_tests_av_perf/fps/tulip2.ogg',
        'win-release/media_tests_av_perf/fps/tulip2.ogv',
        'win-release/media_tests_av_perf/fps/tulip2.wav',
        'win-release/media_tests_av_perf/fps/tulip2.webm']
    # Test regular positive integers.
    reva = 12345
    revb = 54321
    VerifyPreparedTests(self, tests_to_update, reva, revb)
    # Test negative values.
    reva = -54321
    revb = 12345
    with self.assertRaises(ValueError):
      upe_mod.PrepareTestsForUpdate(tests_to_update, all_tests, reva, revb)
    # Test reva greater than revb.
    reva = 54321
    revb = 12345
    upe_mod.PrepareTestsForUpdate(tests_to_update, all_tests, reva, revb)
    # Test non-integer values
    reva = 'sds'
    revb = 12345
    with self.assertRaises(ValueError):
      upe_mod.PrepareTestsForUpdate(tests_to_update, all_tests, reva, revb)


if __name__ == '__main__':
  unittest.main()
