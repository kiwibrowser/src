#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs an isolated non-Telemetry perf test .

The main contract is that the caller passes the arguments:

  --isolated-script-test-output=[FILENAME]
json is written to that file in the format produced by
common.parse_common_test_results.

  --isolated-script-test-chartjson-output=[FILE]
stdout is written to this file containing chart results for the perf dashboard

Optional argument:

  --isolated-script-test-filter=[TEST_NAMES]

is a double-colon-separated ("::") list of test names, to run just that subset
of tests. This list is parsed by this harness and sent down via the
--gtest_filter argument.

This script is intended to be the base command invoked by the isolate,
followed by a subsequent non-python executable.  It is modeled after
run_gpu_integration_test_as_gtest.py
"""

import argparse
import json
import os
import shutil
import sys
import tempfile
import traceback

import common


def GetChromiumSrcDir():
  return os.path.abspath(
      os.path.join(os.path.abspath(__file__), '..', '..', '..'))

def GetPerfDir():
  return os.path.join(GetChromiumSrcDir(), 'tools', 'perf')
# Add src/tools/perf where generate_legacy_perf_dashboard_json.py lives
sys.path.append(GetPerfDir())

import generate_legacy_perf_dashboard_json

# Add src/testing/ into sys.path for importing xvfb and test_env.
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import xvfb
import test_env

# Unfortunately we need to copy these variables from ../test_env.py.
# Importing it and using its get_sandbox_env breaks test runs on Linux
# (it seems to unset DISPLAY).
CHROME_SANDBOX_ENV = 'CHROME_DEVEL_SANDBOX'
CHROME_SANDBOX_PATH = '/opt/chromium/chrome_sandbox'


def IsWindows():
  return sys.platform == 'cygwin' or sys.platform.startswith('win')


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--isolated-script-test-output', type=str,
      required=True)
  parser.add_argument(
      '--isolated-script-test-chartjson-output', type=str,
      required=False)
  parser.add_argument(
      '--isolated-script-test-perf-output', type=str,
      required=False)
  parser.add_argument(
      '--isolated-script-test-filter', type=str, required=False)
  parser.add_argument('--xvfb', help='Start xvfb.', action='store_true')

  args, rest_args = parser.parse_known_args()

  rc, charts, output_json = execute_perf_test(args, rest_args)

  # TODO(eakuefner): Make isolated_script_test_perf_output mandatory after
  # flipping flag in swarming.
  if args.isolated_script_test_perf_output:
    filename = args.isolated_script_test_perf_output
  else:
    filename = args.isolated_script_test_chartjson_output
  # Write the returned encoded json to a the charts output file
  with open(filename, 'w') as f:
    f.write(charts)

  with open(args.isolated_script_test_output, 'w') as fp:
    json.dump(output_json, fp)

  return rc


def execute_perf_test(args, rest_args):
  env = os.environ.copy()
  # Assume we want to set up the sandbox environment variables all the
  # time; doing so is harmless on non-Linux platforms and is needed
  # all the time on Linux.
  env[CHROME_SANDBOX_ENV] = CHROME_SANDBOX_PATH

  rc = 0
  try:
    executable = rest_args[0]
    extra_flags = []
    if len(rest_args) > 1:
      extra_flags = rest_args[1:]

    # These flags are to make sure that test output perf metrics in the log.
    if not '--verbose' in extra_flags:
      extra_flags.append('--verbose')
    if not '--test-launcher-print-test-stdio=always' in extra_flags:
      extra_flags.append('--test-launcher-print-test-stdio=always')
    if args.isolated_script_test_filter:
      filter_list = common.extract_filter_list(
        args.isolated_script_test_filter)
      extra_flags.append('--gtest_filter=' + ':'.join(filter_list))

    if IsWindows():
      executable = '.\%s.exe' % executable
    else:
      executable = './%s' % executable
    with common.temporary_file() as tempfile_path:
      env['CHROME_HEADLESS'] = '1'
      cmd = [executable] + extra_flags

      if args.xvfb:
        rc = xvfb.run_executable(cmd, env, stdoutfile=tempfile_path)
      else:
        rc = test_env.run_command_with_output(cmd, env=env,
                                              stdoutfile=tempfile_path)

      # Now get the correct json format from the stdout to write to the perf
      # results file
      results_processor = (
          generate_legacy_perf_dashboard_json.LegacyResultsProcessor())
      charts = results_processor.GenerateJsonResults(tempfile_path)
  except Exception:
    traceback.print_exc()
    rc = 1

  valid = (rc == 0)
  failures = [] if valid else ['(entire test suite)']
  output_json = {
      'valid': valid,
      'failures': failures,
    }
  return rc, charts, output_json

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

