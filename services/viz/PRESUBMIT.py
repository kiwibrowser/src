# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Top-level presubmit script for services/viz."""

def CheckChangeOnUpload(input_api, output_api):
  import sys
  original_sys_path = sys.path
  sys.path = sys.path + [input_api.os_path.join(
      input_api.change.RepositoryRoot(),
      'components', 'viz')]

  import presubmit_checks as ps
  white_list=(r'^services[\\/]viz[\\/].*\.(cc|h)$',)
  return ps.RunAllChecks(input_api, output_api, white_list)

def PostUploadHook(cl, change, output_api):
  """git cl upload will call this hook after the issue is created/modified.

  This hook modifies the CL description in order to run extra GPU
  tests (in particular, on Android) in addition to the regular CQ try
  bots. These tests don't yet run by default on
  android_n5x_swarming_rel, but viz changes need to run them.

  When adding/removing tests here, ensure that both gpu/PRESUBMIT.py and
  ui/gl/PRESUBMIT.py are updated.
  """
  return output_api.EnsureCQIncludeTrybotsAreAdded(
    cl,
    [
      'luci.chromium.try:android_optional_gpu_tests_rel',
    ],
    'Automatically added optional GPU tests to run on CQ.')
