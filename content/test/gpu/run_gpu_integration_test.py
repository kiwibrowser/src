#!/usr/bin/env vpython
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import json
import os
import sys

from gpu_tests import path_util
import gpu_project_config

path_util.SetupTelemetryPaths()

from telemetry.testing import browser_test_runner

def PostprocessJSON(file_name, run_test_args):
  # The file is not necessarily written depending on the arguments - only
  # postprocess it in case it is.
  if os.path.isfile(file_name):
    with open(file_name) as f:
      test_result = json.load(f)
    test_result['run_test_args'] = run_test_args
    with open(file_name, 'w') as f:
      json.dump(test_result, f, indent=2)

def main():
  rest_args = sys.argv[1:]
  parser = argparse.ArgumentParser(description='Extra argument parser',
                                   add_help=False)

  parser.add_argument(
    '--write-run-test-arguments',
    action='store_true',
    help=('Write the test script arguments to the results file.'))
  option, rest_args_filtered = parser.parse_known_args(rest_args)

  retval = browser_test_runner.Run(
      gpu_project_config.CONFIG, rest_args_filtered)

  # We're not relying on argparse to print the help in the normal way, because
  # we need the help output from both the argument parser here and the argument
  # parser in browser_test_runner.
  if '--help' in rest_args:
    parser.print_help()
    return retval

  # This duplicates an argument of browser_test_runner.
  parser.add_argument(
    '--write-full-results-to', metavar='FILENAME',
    action='store',
    help=('If specified, writes the full results to that path.'))

  option, _ = parser.parse_known_args(rest_args)

  # Postprocess the outputted JSON to add test arguments.
  if option.write_run_test_arguments and option.write_full_results_to:
    PostprocessJSON(option.write_full_results_to, rest_args)
  return retval

if __name__ == '__main__':
  sys.exit(main())
