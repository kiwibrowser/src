#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs an isolate bundled Telemetry GPU integration test.

This script attempts to emulate the contract of gtest-style tests
invoked via recipes. The main contract is that the caller passes the
argument:

  --isolated-script-test-output=[FILENAME]

json is written to that file in the format produced by
common.parse_common_test_results.

Optional argument:

  --isolated-script-test-filter=[TEST_NAMES]

is a double-colon-separated ("::") list of test names, to run just that subset
of tests. This list is parsed by this harness and sent down via the
--test-filter argument.

This script is intended to be the base command invoked by the isolate,
followed by a subsequent Python script. It could be generalized to
invoke an arbitrary executable.
"""

import argparse
import json
import os
import shutil
import sys
import tempfile
import traceback

import common

# Add src/testing/ into sys.path for importing xvfb.
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import xvfb

# Unfortunately we need to copy these variables from ../test_env.py.
# Importing it and using its get_sandbox_env breaks test runs on Linux
# (it seems to unset DISPLAY).
CHROME_SANDBOX_ENV = 'CHROME_DEVEL_SANDBOX'
CHROME_SANDBOX_PATH = '/opt/chromium/chrome_sandbox'

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
    filter_list = common.extract_filter_list(args.isolated_script_test_filter)
    # Need to convert this to a valid regex.
    filter_regex = '(' + '|'.join(filter_list) + ')'
    rest_args.append('--test-filter=' + filter_regex)

  xvfb_proc = None
  openbox_proc = None
  xcompmgr_proc = None
  env = os.environ.copy()
  # Assume we want to set up the sandbox environment variables all the
  # time; doing so is harmless on non-Linux platforms and is needed
  # all the time on Linux.
  env[CHROME_SANDBOX_ENV] = CHROME_SANDBOX_PATH
  if args.xvfb and xvfb.should_start_xvfb(env):
    xvfb_proc, openbox_proc, xcompmgr_proc = xvfb.start_xvfb(env=env,
                                                             build_dir='.')
    assert xvfb_proc and openbox_proc and xcompmgr_proc, 'Failed to start xvfb'
  # Compatibility with gtest-based sharding.
  total_shards = None
  shard_index = None
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
  try:
    valid = True
    rc = 0
    try:
      env['CHROME_HEADLESS'] = '1'
      rc = common.run_command([sys.executable] + rest_args + sharding_args + [
        '--write-full-results-to', args.isolated_script_test_output
      ], env=env)
    except Exception:
      traceback.print_exc()
      valid = False

    if not valid:
      failures = ['(entire test suite)']
      with open(args.isolated_script_test_output, 'w') as fp:
        json.dump({
            'valid': valid,
            'failures': failures,
        }, fp)

    return rc

  finally:
    xvfb.kill(xvfb_proc)
    xvfb.kill(openbox_proc)
    xvfb.kill(xcompmgr_proc)


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
