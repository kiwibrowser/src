# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Top-level presubmit script for testing/trigger_scripts.

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools.
"""

def CommonChecks(input_api, output_api):
  commands = [
    input_api.Command(
      name='trigger_multiple_dimensions_unittest', cmd=[
        input_api.python_executable, 'trigger_multiple_dimensions_unittest.py'],
      kwargs={}, message=output_api.PresubmitError),
  ]
  return input_api.RunTests(commands)

def CheckChangeOnUpload(input_api, output_api):
  return CommonChecks(input_api, output_api)

def CheckChangeOnCommit(input_api, output_api):
  return CommonChecks(input_api, output_api)

def PostUploadHook(cl, change, output_api):
  """git cl upload will call this hook after the issue is created/modified.

  This hook modifies the CL description in order to run extra GPU
  tests (in particular, the WebGL 2.0 conformance tests) in addition
  to the regular CQ try bots. This test suite is too large to run
  against all Chromium commits, but should be run against changes
  likely to affect these tests. The trigger_multiple_dimensions script
  is used on the chromium.gpu.fyi waterfall and associated optional
  tryservers, so it's desired to run extra tests when modifying these
  scripts.
  """
  del change # for pylint
  return output_api.EnsureCQIncludeTrybotsAreAdded(
    cl,
    [
      'luci.chromium.try:linux_optional_gpu_tests_rel',
      'luci.chromium.try:mac_optional_gpu_tests_rel',
      'luci.chromium.try:win_optional_gpu_tests_rel',
      'luci.chromium.try:android_optional_gpu_tests_rel',
    ],
    'Automatically added optional GPU tests to run on CQ.')
