# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit script for //ui/gl.

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools.
"""

def PostUploadHook(cl, change, output_api):
  """git cl upload will call this hook after the issue is created/modified.

  This hook modifies the CL description in order to run extra GPU
  tests (in particular, the WebGL 2.0 conformance tests) in addition
  to the regular CQ try bots. This test suite is too large to run
  against all Chromium commits, but should be run against changes
  likely to affect these tests.

  When adding/removing tests here, ensure that both gpu/PRESUBMIT.py and
  ui/gl/PRESUBMIT.py are updated.
  """
  return output_api.EnsureCQIncludeTrybotsAreAdded(
    cl,
    [
      'luci.chromium.try:linux_optional_gpu_tests_rel',
      'luci.chromium.try:mac_optional_gpu_tests_rel',
      'luci.chromium.try:win_optional_gpu_tests_rel',
      'luci.chromium.try:android_optional_gpu_tests_rel',
    ],
    'Automatically added optional GPU tests to run on CQ.')
