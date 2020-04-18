#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs isolate bundled Telemetry unittests.

This script attempts to emulate the contract of gtest-style tests
invoked via recipes. The main contract is that the caller passes the
argument:

  --isolated-script-test-output=[FILENAME]

json is written to that file in the format produced by
common.parse_common_test_results.

Optional argument:

  --isolated-script-test-filter=[TEST_NAMES]

is a double-colon-separated ("::") list of test names, to run just that subset
of tests. This list is parsed by this harness and remapped to multiple arguments
passed to the target script.

This script is intended to be the base command invoked by the isolate,
followed by a subsequent Python script. It could be generalized to
invoke an arbitrary executable.

"""

import argparse
import json
import os
import sys

import common

# Add src/testing/ into sys.path for importing xvfb.
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import xvfb


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--isolated-script-test-output', type=str,
      required=True)
  parser.add_argument(
      '--isolated-script-test-filter', type=str,
      required=False)
  parser.add_argument('--xvfb', help='Start xvfb.', action='store_true')
  args, rest_args = parser.parse_known_args()
  # Remove the chartjson extra arg until this script cares about chartjson
  # results from telemetry
  index = 0
  for arg in rest_args:
    if ('--isolated-script-test-chartjson-output' in arg or
        '--isolated-script-test-perf-output' in arg):
      rest_args.pop(index)
      break
    index += 1
  if args.isolated_script_test_filter:
    # This test harness doesn't yet support reading the test list from
    # a file.
    filter_list = common.extract_filter_list(args.isolated_script_test_filter)
    # This harness takes the test names to run as the first arguments.
    # The first argument of rest_args is the script to run, so insert
    # the test names after that.
    rest_args = [rest_args[0]] + filter_list + rest_args[1:]

  # Compatibility with gtest-based sharding.
  total_shards = None
  shard_index = None
  env = os.environ.copy()
  env['CHROME_HEADLESS'] = '1'

  if 'GTEST_TOTAL_SHARDS' in env:
    total_shards = int(env['GTEST_TOTAL_SHARDS'])
    del env['GTEST_TOTAL_SHARDS']
  if 'GTEST_SHARD_INDEX' in env:
    shard_index = int(env['GTEST_SHARD_INDEX'])
    del env['GTEST_SHARD_INDEX']
  sharding_args = []
  if total_shards is not None and shard_index is not None:
    sharding_args = [
      '--total-shards=%d' % total_shards,
      '--shard-index=%d' % shard_index
    ]
  cmd = [sys.executable] + rest_args + sharding_args + [
      '--write-full-results-to', args.isolated_script_test_output]
  if args.xvfb:
    return xvfb.run_executable(cmd, env)
  else:
    return common.run_command(cmd, env=env)


# This is not really a "script test" so does not need to manually add
# any additional compile targets.
def main_compile_targets(args):
  json.dump([], args.output)


if __name__ == '__main__':
  # Conform minimally to the protocol defined by ScriptTest.
  if 'compile_targets' in sys.argv:
    funcs = {
      'run': None,
      'compile_targets': main_compile_targets,
    }
    sys.exit(common.run_script(sys.argv[1:], funcs))
  sys.exit(main())
