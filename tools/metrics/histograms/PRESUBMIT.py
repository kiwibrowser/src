# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details on the presubmit API built into depot_tools.
"""


def ValidationNeeded(input_api):
  """Check if validation of histograms.xml files are required."""
  for f in input_api.AffectedTextFiles():
    p = f.AbsoluteLocalPath()
    if (input_api.basename(p) in {'histograms.xml', 'enums.xml'} and
        input_api.os_path.dirname(p) == input_api.PresubmitLocalPath()):
      return True
  return False


def CheckChange(input_api, output_api):
  """Checks that histograms.xml is pretty-printed and well-formatted."""
  results = []
  if ValidationNeeded(input_api):
    cwd = input_api.PresubmitLocalPath()

    exit_code = input_api.subprocess.call(
        [input_api.python_executable, 'pretty_print.py', '--presubmit',
         '--non-interactive'],
        cwd=cwd)
    if exit_code != 0:
      results.append(output_api.PresubmitError(
          'histograms.xml is not formatted correctly; please run '
          'git cl format %s to fix.' % cwd))

    exit_code = input_api.subprocess.call(
        [input_api.python_executable, 'validate_format.py'], cwd=cwd)
    if exit_code != 0:
      results.append(output_api.PresubmitError(
          'histograms.xml is not well formatted; run %s/validate_format.py '
          'and fix the reported errors.' % cwd))

  return results


def CheckChangeOnUpload(input_api, output_api):
  return CheckChange(input_api, output_api)


def CheckChangeOnCommit(input_api, output_api):
  return CheckChange(input_api, output_api)
