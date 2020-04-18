# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit script for Android xml code.

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools.

This presubmit checks for the following:
  - Colors are defined as RRGGBB or AARRGGBB
  - No (A)RGB values are referenced outside colors.xml
  - No duplicate (A)RGB values are referenced in colors.xml
"""

from collections import defaultdict
import re

COLOR_PATTERN = re.compile(r'(>|")(#[0-9A-Fa-f]+)(<|")')
VALID_COLOR_PATTERN = re.compile(
    r'^#([0-9A-F][0-9A-E]|[0-9A-E][0-9A-F])?[0-9A-F]{6}$')


def CheckChangeOnUpload(input_api, output_api):
  return _CommonChecks(input_api, output_api)


def CheckChangeOnCommit(input_api, output_api):
  return _CommonChecks(input_api, output_api)


def _CommonChecks(input_api, output_api):
  """Checks common to both upload and commit."""
  result = []
  result.extend(_CheckColorFormat(input_api, output_api))
  result.extend(_CheckColorReferences(input_api, output_api))
  result.extend(_CheckDuplicateColors(input_api, output_api))
  # Add more checks here
  return result


def _CheckColorFormat(input_api, output_api):
  """Checks color (A)RGB values are of format either RRGGBB or AARRGGBB."""
  errors = []
  for f in input_api.AffectedFiles(include_deletes=False):
    if not f.LocalPath().endswith('.xml'):
      continue
    # Ingnore vector drawable xmls
    contents = input_api.ReadFile(f)
    if '<vector' in contents:
      continue
    for line_number, line in f.ChangedContents():
      color = COLOR_PATTERN.search(line)
      if color and not VALID_COLOR_PATTERN.match(color.group(2)):
        errors.append(
            '  %s:%d\n    \t%s' % (f.LocalPath(), line_number, line.strip()))
  if errors:
    return [output_api.PresubmitError(
  '''
  Android Color Reference Check failed:
    Your new code added (A)RGB values for colors that are not well
    formatted, listed below.

    This is banned, please define colors in format of #RRGGBB for opaque
    colors or #AARRGGBB for translucent colors. Note that they should be
    defined in chrome/android/java/res/values/colors.xml.

    See https://crbug.com/775198 for more information.
  ''',
        errors)]
  return []


def _CheckColorReferences(input_api, output_api):
  """Checks no (A)RGB values are defined outside colors.xml."""
  errors = []
  for f in input_api.AffectedFiles(include_deletes=False):
    if (not f.LocalPath().endswith('.xml') or
        f.LocalPath().endswith('/colors.xml')):
      continue
    # Ingnore vector drawable xmls
    contents = input_api.ReadFile(f)
    if '<vector' in contents:
      continue
    for line_number, line in f.ChangedContents():
      if COLOR_PATTERN.search(line):
        errors.append(
            '  %s:%d\n    \t%s' % (f.LocalPath(), line_number, line.strip()))
  if errors:
    return [output_api.PresubmitError(
  '''
  Android Color Reference Check failed:
    Your new code added new color references that are not color resources from
    chrome/android/java/res/values/colors.xml, listed below.

    This is banned, please use the existing color resources or create a new
    color resource in colors.xml, and reference the color by @color/....

    See https://crbug.com/775198 for more information.
  ''',
        errors)]
  return []


def _CheckDuplicateColors(input_api, output_api):
  """Checks colors defined by (A)RGB values in colors.xml are unique."""
  errors = []
  for f in input_api.AffectedFiles(include_deletes=False):
    if not f.LocalPath().endswith('/colors.xml'):
      continue
    colors = defaultdict(int)
    contents = input_api.ReadFile(f)
    # Get count for each color defined.
    for line in contents.splitlines(False):
      color = COLOR_PATTERN.search(line)
      if color:
        colors[color.group(2)] += 1

    # Check duplicates in changed contents.
    for line_number, line in f.ChangedContents():
      color = COLOR_PATTERN.search(line)
      if color and colors[color.group(2)] > 1:
        errors.append(
            '  %s:%d\n    \t%s' % (f.LocalPath(), line_number, line.strip()))
  if errors:
    return [output_api.PresubmitError(
  '''
  Android Duplicate Color Declaration Check failed:
    Your new code added new colors by (A)RGB values that are already defined in
    chrome/android/java/res/values/colors.xml, listed below.

    This is banned, please reference the existing color resource from colors.xml
    using @color/... and if needed, give the existing color resource a more
    general name (e.g. google_grey_100).

    See https://crbug.com/775198 for more information.
  ''',
        errors)]
  return []
