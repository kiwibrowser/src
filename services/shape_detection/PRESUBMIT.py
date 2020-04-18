# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Top-level presubmit script for shape_detection.

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools.
"""

import re

def PostUploadHook(cl, change, output_api):
  """git cl upload will call this hook after the issue is created/modified.

  This hook adds an extra try bot list to the CL description in order to run
  the Mac GPU and Windows 10 bots in addition to the usual CQ try bots.
  """
  return output_api.EnsureCQIncludeTrybotsAreAdded(
    cl,
    [
      'luci.chromium.try:mac_optional_gpu_tests_rel',
      'luci.chromium.try:win10_chromium_x64_rel_ng'
    ],
    'Automatically added optional Mac GPU and Windows 10 tests to run on CQ.')
