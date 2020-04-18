# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit script for changes affecting chrome/app/

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools.
"""

import os

def _CheckNoProductNameInGeneratedResources(input_api, output_api):
  """Check that no PRODUCT_NAME placeholders are found in resources files.

  These kinds of strings prevent proper localization in some languages. For
  more information, see the following chromium-dev thread:
  https://groups.google.com/a/chromium.org/forum/#!msg/chromium-dev/PBs5JfR0Aoc/NOcIHII9u14J
  """

  problems = []
  filename_filter = lambda x: x.LocalPath().endswith('.grd')

  for f, line_num, line in input_api.RightHandSideLines(filename_filter):
    if 'PRODUCT_NAME' in line:
      problems.append('%s:%d' % (f.LocalPath(), line_num))

  if problems:
    return [output_api.PresubmitPromptWarning(
        "Don't use PRODUCT_NAME placeholders in string resources. Instead, add "
        "separate strings to google_chrome_strings.grd and "
        "chromium_strings.grd. See http://goo.gl/6614MQ for more information."
        "Problems with this check? Contact dubroy@chromium.org.",
        items=problems)]
  return []

def _CheckFlagsMessageNotTranslated(input_api, output_api):
  """Check: all about:flags messages are marked as not requiring translation.

  This assumes that such messages are only added to generated_resources.grd and
  that all such messages have names starting with IDS_FLAGS_. The expected mark
  for not requiring translation is 'translateable="false"'.
  """

  problems = []
  filename_filter = lambda x: x.LocalPath().endswith("generated_resources.grd")

  for f, line_num, line in input_api.RightHandSideLines(filename_filter):
    if "name=\"IDS_FLAGS_" in line and not "translateable=\"false\"" in line:
      problems.append("Missing translateable=\"false\" in %s:%d"
                      % (f.LocalPath(), line_num))
      problems.append(line)

  if problems:
    return [output_api.PresubmitError(
        "If you define a flag name, description or value, mark it as not "
        "requiring translation by adding the 'translateable' attribute with "
        "value \"false\". See https://crbug.com/587272 for more context.",
        items=problems)]
  return []

def _CommonChecks(input_api, output_api):
  """Checks common to both upload and commit."""
  results = []
  results.extend(_CheckNoProductNameInGeneratedResources(input_api, output_api))
  results.extend(_CheckFlagsMessageNotTranslated(input_api, output_api))
  return results

def CheckChangeOnUpload(input_api, output_api):
  return _CommonChecks(input_api, output_api)

def CheckChangeOnCommit(input_api, output_api):
  return _CommonChecks(input_api, output_api)
