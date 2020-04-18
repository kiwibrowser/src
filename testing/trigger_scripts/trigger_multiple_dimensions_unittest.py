#!/usr/bin/python
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for trigger_multiple_dimensions.py."""

import unittest

import trigger_multiple_dimensions

class Args(object):
  def __init__(self):
    self.shards = 1
    self.dump_json = ''
    self.multiple_trigger_configs = []
    self.multiple_dimension_script_verbose = False


class FakeTriggerer(trigger_multiple_dimensions.MultiDimensionTestTriggerer):
  def __init__(self, bot_configs, bot_statuses,
               use_superclass_random_number_generator, first_random_number):
    super(FakeTriggerer, self).__init__()
    self._bot_configs = bot_configs
    self._bot_statuses = bot_statuses
    self._swarming_runs = []
    self._files = {}
    self._temp_file_id = 0
    self._use_superclass_rng = use_superclass_random_number_generator
    self._last_random_number = first_random_number

  def set_files(self, files):
    self._files = files

  def choose_random_int(self, max_num):
    if self._use_superclass_rng:
      return super(FakeTriggerer, self).choose_random_int(max_num)
    if self._last_random_number > max_num:
      self._last_random_number = 1
    result = self._last_random_number
    self._last_random_number += 1
    return result

  def make_temp_file(self, prefix=None, suffix=None):
    result = prefix + str(self._temp_file_id) + suffix
    self._temp_file_id += 1
    return result

  def delete_temp_file(self, temp_file):
    pass

  def read_json_from_temp_file(self, temp_file):
    return self._files[temp_file]

  def write_json_to_file(self, merged_json, output_file):
    self._files[output_file] = merged_json

  def parse_bot_configs(self, args):
    pass

  def query_swarming_for_bot_configs(self, verbose):
    # Sum up the total count of all bots.
    self._total_bots = sum(x['total'] for x in self._bot_statuses)

  def run_swarming(self, args, verbose):
    self._swarming_runs.append(args)


WIN_NVIDIA_QUADRO_P400_STABLE_DRIVER = '10de:1cb3-23.21.13.8792'
WIN7 = 'Windows-2008ServerR2-SP1'
WIN10 = 'Windows-10'

WIN7_NVIDIA = {
  'gpu': WIN_NVIDIA_QUADRO_P400_STABLE_DRIVER,
  'os': WIN7,
  'pool': 'Chrome-GPU',
}

WIN10_NVIDIA = {
  'gpu': WIN_NVIDIA_QUADRO_P400_STABLE_DRIVER,
  'os': WIN10,
  'pool': 'Chrome-GPU',
}

class UnitTest(unittest.TestCase):
  def basic_win7_win10_setup(self, bot_statuses,
                             use_superclass_random_number_generator=False,
                             first_random_number=1):
    triggerer = FakeTriggerer(
      [
        WIN7_NVIDIA,
        WIN10_NVIDIA
      ],
      bot_statuses,
      use_superclass_random_number_generator,
      first_random_number
    )
    # Note: the contents of these JSON files don't accurately reflect
    # that produced by "swarming.py trigger". The unit tests only
    # verify that shard 0's JSON is preserved.
    triggerer.set_files({
      'base_trigger_dimensions0.json': {
        'base_task_name': 'webgl_conformance_tests',
        'request': {
          'expiration_secs': 3600,
          'properties': {
            'execution_timeout_secs': 3600,
          },
        },
        'tasks': {
          'webgl_conformance_tests on NVIDIA GPU on Windows': {
            'task_id': 'f001',
          },
        },
      },
      'base_trigger_dimensions1.json': {
        'tasks': {
          'webgl_conformance_tests on NVIDIA GPU on Windows': {
            'task_id': 'f002',
          },
        },
      },
    })
    args = Args()
    args.shards = 2
    args.dump_json = 'output.json'
    args.multiple_dimension_script_verbose = False
    triggerer.trigger_tasks(
      args,
      [
        'trigger',
        '--dimension',
        'gpu',
        WIN_NVIDIA_QUADRO_P400_STABLE_DRIVER,
        '--dimension',
        'os',
        WIN7,
        '--dimension',
        'pool',
        'Chrome-GPU',
        '--',
        'webgl_conformance',
      ])
    return triggerer

  def list_contains_sublist(self, main_list, sub_list):
    return any(sub_list == main_list[offset:offset + len(sub_list)]
               for offset in xrange(len(main_list) - (len(sub_list) - 1)))

  def shard_runs_on_os(self, triggerer, shard_index, os):
    return self.list_contains_sublist(triggerer._swarming_runs[shard_index],
                                      ['--dimension', 'os', os])

  def test_parse_bot_configs(self):
    triggerer = trigger_multiple_dimensions.MultiDimensionTestTriggerer()
    args = Args()
    args.multiple_trigger_configs = "{ foo }"
    self.assertRaisesRegexp(ValueError, "Error while parsing JSON.*",
                            triggerer.parse_bot_configs, args)
    args.multiple_trigger_configs = "{ \"foo\": \"bar\" }"
    self.assertRaisesRegexp(ValueError, "Bot configurations must be a list.*",
                            triggerer.parse_bot_configs, args)
    args.multiple_trigger_configs = "[]"
    self.assertRaisesRegexp(ValueError,
                            "Bot configuration list must have at least.*",
                            triggerer.parse_bot_configs, args)
    args.multiple_trigger_configs = "[{}, \"\"]"
    self.assertRaisesRegexp(ValueError,
                            "Bot configurations must all be.*",
                            triggerer.parse_bot_configs, args)
    args.multiple_trigger_configs = "[{}]"
    triggerer.parse_bot_configs(args)
    self.assertEqual(triggerer._bot_configs, [{}])

  def test_split_with_available_machines(self):
    triggerer = self.basic_win7_win10_setup(
      [
        {
          'total': 1,
          'available': 1,
        },
        {
          'total': 1,
          'available': 1,
        },
      ],
    )
    # First shard should run on Win7.
    self.assertTrue(self.shard_runs_on_os(triggerer, 0, WIN7))
    # Second shard should run on Win10.
    self.assertTrue(self.shard_runs_on_os(triggerer, 1, WIN10))
    # And not vice versa.
    self.assertFalse(self.shard_runs_on_os(triggerer, 0, WIN10))
    self.assertFalse(self.shard_runs_on_os(triggerer, 1, WIN7))

  def test_shard_env_vars(self):
    triggerer = self.basic_win7_win10_setup(
      [
        {
          'total': 2,
          'available': 2,
        },
        {
          'total': 2,
          'available': 0,
        },
      ],
    )
    self.assertTrue(self.list_contains_sublist(
      triggerer._swarming_runs[0], ['--env', 'GTEST_SHARD_INDEX', '0']))
    self.assertTrue(self.list_contains_sublist(
      triggerer._swarming_runs[1], ['--env', 'GTEST_SHARD_INDEX', '1']))
    self.assertTrue(self.list_contains_sublist(
      triggerer._swarming_runs[0], ['--env', 'GTEST_TOTAL_SHARDS', '2']))
    self.assertTrue(self.list_contains_sublist(
      triggerer._swarming_runs[1], ['--env', 'GTEST_TOTAL_SHARDS', '2']))

  def test_json_merging(self):
    triggerer = self.basic_win7_win10_setup(
      [
        {
          'total': 1,
          'available': 1,
        },
        {
          'total': 1,
          'available': 1,
        },
      ],
    )
    self.assertTrue('output.json' in triggerer._files)
    output_json = triggerer._files['output.json']
    self.assertTrue('base_task_name' in output_json)
    self.assertTrue('request' in output_json)
    self.assertEqual(output_json['request']['expiration_secs'], 3600)
    self.assertEqual(
      output_json['request']['properties']['execution_timeout_secs'], 3600)

  def test_split_with_only_one_config_available(self):
    triggerer = self.basic_win7_win10_setup(
      [
        {
          'total': 2,
          'available': 2,
        },
        {
          'total': 2,
          'available': 0,
        },
      ],
    )
    # Both shards should run on Win7.
    self.assertTrue(self.shard_runs_on_os(triggerer, 0, WIN7))
    self.assertTrue(self.shard_runs_on_os(triggerer, 1, WIN7))
    # Redo with only Win10 bots available.
    triggerer = self.basic_win7_win10_setup(
      [
        {
          'total': 2,
          'available': 0,
        },
        {
          'total': 2,
          'available': 2,
        },
      ],
    )
    self.assertTrue(self.shard_runs_on_os(triggerer, 0, WIN10))
    self.assertTrue(self.shard_runs_on_os(triggerer, 1, WIN10))

  def test_split_with_no_bots_available(self):
    triggerer = self.basic_win7_win10_setup(
      [
        {
          'total': 1,
          'available': 0,
        },
        {
          'total': 1,
          'available': 0,
        },
      ],
    )
    # Given the fake random number generator above, first shard should
    # run on Win7.
    self.assertTrue(self.shard_runs_on_os(triggerer, 0, WIN7))
    # Second shard should run on Win10.
    self.assertTrue(self.shard_runs_on_os(triggerer, 1, WIN10))
    # Try again with different bot distribution and random numbers.
    triggerer = self.basic_win7_win10_setup(
      [
        {
          'total': 2,
          'available': 0,
        },
        {
          'total': 2,
          'available': 0,
        },
      ],
      first_random_number=3,
    )
    self.assertTrue(self.shard_runs_on_os(triggerer, 0, WIN10))
    self.assertTrue(self.shard_runs_on_os(triggerer, 1, WIN10))

  def test_superclass_random_number_generator_works(self):
    # Probe randomly a certain number of times.
    num_runs = 0
    for _ in xrange(100):
      triggerer = self.basic_win7_win10_setup(
        [
          {
            'total': 2,
            'available': 0,
          },
          {
            'total': 2,
            'available': 0,
          },
        ],
        use_superclass_random_number_generator=True
      )
      for _ in xrange(2):
        self.assertTrue(self.shard_runs_on_os(triggerer, 0, WIN7) or
                        self.shard_runs_on_os(triggerer, 0, WIN10))
        num_runs += 1
    self.assertEqual(num_runs, 200)

if __name__ == '__main__':
  unittest.main()
