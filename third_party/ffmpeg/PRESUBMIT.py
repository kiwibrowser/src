# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit script for FFmpeg repository.

Does the following:
- Warns users that changes must be submitted via Gerrit.
- Warns users when a change is made without updating the README file.
"""

import re
import subprocess


def _WarnIfReadmeIsUnchanged(input_api, output_api):
  """Warn if the README file hasn't been updated with change notes."""
  has_ffmpeg_changes = False
  chromium_re = re.compile(r'.*[/\\]?chromium.*|PRESUBMIT.py$|.*\.chromium$')
  readme_re = re.compile(r'.*[/\\]?chromium[/\\]patches[/\\]README$')
  for f in input_api.AffectedFiles():
    if readme_re.match(f.LocalPath()):
      return []
    if not has_ffmpeg_changes and not chromium_re.match(f.LocalPath()):
      has_ffmpeg_changes = True

  if not has_ffmpeg_changes:
    return []

  return [output_api.PresubmitPromptWarning('\n'.join([
      'FFmpeg changes detected without any update to chromium/patches/README,',
      'it\'s good practice to update this file with a note about your changes.'
  ]))]


def _WarnIfGenerateGnTestsFail(input_api, output_api):
  """Error if generate_gn.py was changed and tests are now failing."""
  should_run_tests = False
  generate_gn_re = re.compile(r'.*generate_gn.*\.py$')
  for f in input_api.AffectedFiles():
    if generate_gn_re.match(f.LocalPath()):
      should_run_tests = True
      break;

  errors = []
  if should_run_tests:
    errors += input_api.RunTests(
        input_api.canned_checks.GetUnitTests(
            input_api, output_api,
            ['chromium/scripts/generate_gn_unittest.py']))

  return errors

def CheckChangeOnUpload(input_api, output_api):
  results = []
  results.extend(_WarnIfReadmeIsUnchanged(input_api, output_api))
  results.extend(_WarnIfGenerateGnTestsFail(input_api, output_api))
  return results
