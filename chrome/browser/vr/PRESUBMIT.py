# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit script for changes affecting chrome/browser/vr

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools.
"""

import re

# chrome/PRESUBMIT.py blocks several linters due to the infeasibility of
# enforcing them on a large codebase. Here we'll start by enforcing all
# linters, and add exclusions if necessary.
#
# Note that this list must be non-empty, or cpplint will use its default set of
# filters.
LINT_FILTERS = [
  '-build/include',
]

VERBOSITY_LEVEL = 4

INCLUDE_CPP_FILES_ONLY = (r'.*\.(cc|h)$',)

def _CheckChangeLintsClean(input_api, output_api):
  sources = lambda x: input_api.FilterSourceFile(
      x, white_list=INCLUDE_CPP_FILES_ONLY)
  return input_api.canned_checks.CheckChangeLintsClean(
      input_api, output_api, sources, LINT_FILTERS, VERBOSITY_LEVEL)

def CheckChangeOnUpload(input_api, output_api):
  results = []
  results.extend(_CheckChangeLintsClean(input_api, output_api))
  return results

def CheckChangeOnCommit(input_api, output_api):
  results = []
  results.extend(_CheckChangeLintsClean(input_api, output_api))
  return results

def PostUploadHook(cl, change, output_api):
  """git cl upload will call this hook after the issue is created/modified.

  This hook modifies the CL description in order to run extra GPU
  tests (in particular, the WebGL 2.0 conformance tests) in addition
  to the regular CQ try bots. This test suite is too large to run
  against all Chromium commits, but should be run against changes
  likely to affect these tests.

  Also, it compiles the Linux VR tryserver to compile the UI testapp.

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
      'master.tryserver.chromium.linux:linux_vr',
    ],
    'Automatically added optional GPU tests to run on CQ.')
