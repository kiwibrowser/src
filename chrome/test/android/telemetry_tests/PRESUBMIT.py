# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit script for android chrome telemetry tests.

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts for
details on the presubmit API built into depot_tools.
"""
def CommonChecks(input_api, output_api):
  current_dir = input_api.PresubmitLocalPath()
  chromium_src = input_api.os_path.join(current_dir, '..', '..', '..', '..')

  def J(*dirs):
    """Returns a path relative to chromium src directory."""
    return input_api.os_path.join(chromium_src, *dirs)

  return input_api.canned_checks.RunPylint(
      input_api,
      output_api,
      pylintrc='pylintrc',
      extra_paths_list=[
          J('third_party', 'catapult', 'common', 'py_utils'),
          J('third_party', 'catapult', 'telemetry'),
          J('tools', 'perf'),
      ])


def CheckChangeOnUpload(input_api, output_api):
  return CommonChecks(input_api, output_api)


def CheckChangeOnCommit(input_api, output_api):
  return CommonChecks(input_api, output_api)
