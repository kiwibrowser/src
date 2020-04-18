# Copyright 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit script for changes affecting tools/perf/.

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools.
"""

import os


def _CommonChecks(input_api, output_api, block_on_failure=False):
  """Performs common checks, which includes running pylint.

    block_on_failure: For some failures, we would like to warn the
        user but still allow them to upload the change. However, we
        don't want them to commit code with those failures, so we
        need to block the change on commit.
  """
  results = []

  results.extend(_CheckExpectations(input_api, output_api))
  results.extend(_CheckJson(input_api, output_api))
  results.extend(_CheckPerfData(input_api, output_api, block_on_failure))
  results.extend(_CheckWprShaFiles(input_api, output_api))
  results.extend(input_api.RunTests(input_api.canned_checks.GetPylint(
      input_api, output_api, extra_paths_list=_GetPathsToPrepend(input_api),
      pylintrc='pylintrc')))
  return results


def _GetPathsToPrepend(input_api):
  perf_dir = input_api.PresubmitLocalPath()
  chromium_src_dir = input_api.os_path.join(perf_dir, '..', '..')
  telemetry_dir = input_api.os_path.join(
      chromium_src_dir, 'third_party', 'catapult', 'telemetry')
  experimental_dir = input_api.os_path.join(
      chromium_src_dir, 'third_party', 'catapult', 'experimental')
  tracing_dir = input_api.os_path.join(
      chromium_src_dir, 'third_party', 'catapult', 'tracing')
  py_utils_dir = input_api.os_path.join(
      chromium_src_dir, 'third_party', 'catapult', 'common', 'py_utils')
  android_pylib_dir = input_api.os_path.join(
      chromium_src_dir, 'build', 'android')
  return [
      telemetry_dir,
      input_api.os_path.join(telemetry_dir, 'third_party', 'mock'),
      experimental_dir,
      tracing_dir,
      py_utils_dir,
      android_pylib_dir,
  ]


def _RunArgs(args, input_api):
  p = input_api.subprocess.Popen(args, stdout=input_api.subprocess.PIPE,
                                 stderr=input_api.subprocess.STDOUT)
  out, _ = p.communicate()
  return (out, p.returncode)


def _CheckExpectations(input_api, output_api):
  results = []
  perf_dir = input_api.PresubmitLocalPath()
  vpython = 'vpython.bat' if input_api.is_windows else 'vpython'
  out, return_code = _RunArgs([
      vpython,
      input_api.os_path.join(perf_dir, 'validate_story_expectation_data')],
      input_api)
  if return_code:
    results.append(output_api.PresubmitError(
        'Validating story expectation data failed.', long_text=out))
  return results


def _CheckPerfData(input_api, output_api, block_on_failure):
  results = []
  perf_dir = input_api.PresubmitLocalPath()
  vpython = 'vpython.bat' if input_api.is_windows else 'vpython'
  out, return_code = _RunArgs([
      vpython,
      input_api.os_path.join(perf_dir, 'generate_perf_data'),
      '--validate-only'], input_api)
  if return_code:
    if block_on_failure:
      results.append(output_api.PresubmitError(
          'Validating perf data failed', long_text=out))
    else:
      results.append(output_api.PresubmitPromptWarning(
          'Validating perf data failed', long_text=out))
  return results


def _CheckWprShaFiles(input_api, output_api):
  """Check whether the wpr sha files have matching URLs."""
  perf_dir = input_api.PresubmitLocalPath()

  results = []
  wpr_archive_shas = []
  for affected_file in input_api.AffectedFiles(include_deletes=False):
    filename = affected_file.AbsoluteLocalPath()
    if not filename.endswith('.sha1'):
      continue
    wpr_archive_shas.append(filename)

  vpython = 'vpython.bat' if input_api.is_windows else 'vpython'
  out, return_code = _RunArgs([
      vpython,
      input_api.os_path.join(perf_dir, 'validate_wpr_archives')] +
      wpr_archive_shas,
      input_api)
  if return_code:
    results.append(output_api.PresubmitError(
        'Validating WPR archives failed:', long_text=out))
  return results


def _CheckJson(input_api, output_api):
  """Checks whether JSON files in this change can be parsed."""
  for affected_file in input_api.AffectedFiles(include_deletes=False):
    filename = affected_file.AbsoluteLocalPath()
    if os.path.splitext(filename)[1] != '.json':
      continue
    try:
      input_api.json.load(open(filename))
    except ValueError:
      return [output_api.PresubmitError('Error parsing JSON in %s!' % filename)]
  return []


def CheckChangeOnUpload(input_api, output_api):
  report = []
  report.extend(_CommonChecks(input_api, output_api))
  return report


def CheckChangeOnCommit(input_api, output_api):
  report = []
  report.extend(_CommonChecks(input_api, output_api, block_on_failure=True))
  return report
