#!/usr/bin/env python
#
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs the CTS test APKs stored in GS."""

import argparse
import json
import os
import shutil
import sys
import tempfile
import zipfile


sys.path.append(os.path.join(
    os.path.dirname(__file__), os.pardir, os.pardir, 'build', 'android'))
import devil_chromium  # pylint: disable=import-error
from devil.utils import cmd_helper  # pylint: disable=import-error

sys.path.append(os.path.join(
    os.path.dirname(__file__), os.pardir, os.pardir, 'build'))
import find_depot_tools  # pylint: disable=import-error

# cts test archives for all platforms are stored in this bucket
# contents need to be updated if there is an important fix to any of
# the tests
_CTS_BUCKET = 'gs://chromium-cts'

_GSUTIL_PATH = os.path.join(find_depot_tools.DEPOT_TOOLS_PATH, 'gsutil.py')
_TEST_RUNNER_PATH = os.path.join(
    os.path.dirname(__file__), os.pardir, os.pardir,
    'build', 'android', 'test_runner.py')

_EXPECTED_FAILURES_FILE = os.path.join(
    os.path.dirname(__file__), 'cts_config', 'expected_failure_on_bot.json')
_WEBVIEW_CTS_GCS_PATH_FILE = os.path.join(
    os.path.dirname(__file__), 'cts_config', 'webview_cts_gcs_path.json')


def GetCtsInfo(arch, platform, item):
  """Gets contents of CTS Info for arch and platform.

  See _WEBVIEW_CTS_GCS_PATH_FILE
  """
  with open(_WEBVIEW_CTS_GCS_PATH_FILE) as f:
    cts_gcs_path_info = json.load(f)
  try:
    return cts_gcs_path_info[arch][platform][item]
  except KeyError:
    raise Exception('No %s info available for arch:%s, android:%s' %
                    (item, arch, platform))


def GetExpectedFailures():
  """Gets list of tests expected to fail in <class>#<method> format.

  See _EXPECTED_FAILURES_FILE
  """
  with open(_EXPECTED_FAILURES_FILE) as f:
    expected_failures_info = json.load(f)
  expected_failures = []
  for class_name, methods in expected_failures_info.iteritems():
    expected_failures.extend(['%s#%s' % (class_name, m['name'])
                              for m in methods])
  return expected_failures


def RunCTS(test_runner_args, local_cts_dir, apk, test_filter,
           skip_expected_failures=True, json_results_file=None):
  """Run tests in apk using test_runner script at _TEST_RUNNER_PATH.

  Returns the script result code,
  tests expected to fail will be skipped unless skip_expected_failures
  is set to False, test results will be stored in
  the json_results_file file if specified
  """
  local_test_runner_args = test_runner_args + ['--test-apk',
                                               os.path.join(local_cts_dir, apk)]

  # TODO(mikecase): This doesn't work at all with the
  # --gtest-filter test runner option currently. The
  # filter options will just override eachother.
  if skip_expected_failures:
    local_test_runner_args += ['-f=-%s' % ':'.join(GetExpectedFailures())]
  # The preferred method is to specify test filters per release in
  # the CTS_GCS path file.  It will override any
  # previous filters, including ones in expected failures
  # file.
  if test_filter:
    local_test_runner_args += ['-f=' + test_filter]
  if json_results_file:
    local_test_runner_args += ['--json-results-file=%s' %
                               json_results_file]
  return cmd_helper.RunCmd(
      [_TEST_RUNNER_PATH, 'instrumentation'] + local_test_runner_args)


def MergeTestResults(existing_results_json, additional_results_json):
  """Appends results in additional_results_json to existing_results_json."""
  for k, v in additional_results_json.iteritems():
    if k not in existing_results_json:
      existing_results_json[k] = v
    else:
      if type(v) != type(existing_results_json[k]):
        raise NotImplementedError(
            "Can't merge results field %s of different types" % v)
      if type(v) is dict:
        existing_results_json[k].update(v)
      elif type(v) is list:
        existing_results_json[k].extend(v)
      else:
        raise NotImplementedError(
            "Can't merge results field %s that is not a list or dict" % v)


def DownloadAndExtractCTSZip(args):
  """Download and extract the CTS tests from _CTS_BUCKET.

  Downloads the CTS zip file from _CTS_BUCKET and extract contents to
  apk_dir if specified, or a new temporary directory if not.
  Returns following tuple (local_cts_dir, base_cts_dir, delete_cts_dir):
    local_cts_dir - CTS extraction location for current arch and platform
    base_cts_dir - Root directory for all the arches and platforms
    delete_cts_dir - Set if the base_cts_dir was created as a temporary
    directory
  """
  base_cts_dir = None
  delete_cts_dir = False
  relative_cts_zip_path = GetCtsInfo(args.arch, args.platform, 'filename')

  if args.apk_dir:
    base_cts_dir = args.apk_dir
  else:
    base_cts_dir = tempfile.mkdtemp()
    delete_cts_dir = True

  local_cts_zip_path = os.path.join(base_cts_dir, relative_cts_zip_path)
  google_storage_cts_zip_path = '%s/%s' % (
      _CTS_BUCKET, os.path.join(relative_cts_zip_path))

  # Download CTS APK if needed.
  if not os.path.exists(local_cts_zip_path):
    if cmd_helper.RunCmd([_GSUTIL_PATH, 'cp', google_storage_cts_zip_path,
                          local_cts_zip_path]):
      raise Exception('Error downloading CTS from Google Storage.')

  local_cts_dir = os.path.join(base_cts_dir,
                               GetCtsInfo(args.arch, args.platform, 'apkdir'))
  zf = zipfile.ZipFile(local_cts_zip_path, 'r')
  zf.extractall(local_cts_dir)
  return (local_cts_dir, base_cts_dir, delete_cts_dir)


def DownloadAndRunCTS(args, test_runner_args):
  """Run CTS tests downloaded from _CTS_BUCKET.

  Downloads CTS tests from bucket, runs them for the
  specified platform+arch, then creates a single
  results json file (if specified)
  Returns 0 if all tests passed, otherwise
  returns the failure code of the last failing
  test.
  """
  local_cts_dir, base_cts_dir, delete_cts_dir = DownloadAndExtractCTSZip(args)
  cts_result = 0
  json_results_file = args.json_results_file
  try:
    cts_tests_info = GetCtsInfo(args.arch, args.platform, 'tests')
    cts_results_json = {}
    for cts_tests_item in cts_tests_info:
      for relative_apk_path, test_filter in cts_tests_item.iteritems():
        iteration_cts_result = 0
        if json_results_file:
          with tempfile.NamedTemporaryFile() as iteration_json_file:
            iteration_cts_result = RunCTS(test_runner_args, local_cts_dir,
                                          relative_apk_path, test_filter,
                                          args.skip_expected_failures,
                                          iteration_json_file.name)
            with open(iteration_json_file.name) as f:
              additional_results_json = json.load(f)
              MergeTestResults(cts_results_json, additional_results_json)
        else:
          iteration_cts_result = RunCTS(test_runner_args, local_cts_dir,
                                        relative_apk_path, test_filter,
                                        args.skip_expected_failures)
        if iteration_cts_result:
          cts_result = iteration_cts_result
    if json_results_file:
      with open(json_results_file, 'w') as f:
        json.dump(cts_results_json, f, indent=2)
  finally:
    if delete_cts_dir and base_cts_dir:
      shutil.rmtree(base_cts_dir)
  return cts_result


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--arch',
      choices=['arm64'],
      default='arm64',
      help='Arch for CTS tests.')
  parser.add_argument(
      '--platform',
      choices=['L', 'M', 'N', 'O'],
      required=True,
      help='Android platform version for CTS tests.')
  parser.add_argument(
      '--skip-expected-failures',
      action='store_true',
      help='Option to skip all tests that are expected to fail.')
  parser.add_argument(
      '--apk-dir',
      help='Directory to load/save CTS APKs. Will try to load CTS APK '
           'from this directory before downloading from Google Storage '
           'and will then cache APK here.')
  parser.add_argument(
      '--test-launcher-summary-output',
      '--json-results-file',
      dest='json_results_file', type=os.path.realpath,
      help='If set, will dump results in JSON form to the specified file. '
           'Note that this will also trigger saving per-test logcats to '
           'logdog.')

  args, test_runner_args = parser.parse_known_args()
  devil_chromium.Initialize()

  return DownloadAndRunCTS(args, test_runner_args)


if __name__ == '__main__':
  sys.exit(main())
