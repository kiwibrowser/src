# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit script for ui.

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools.
"""

def CheckX11HeaderUsage(input_api, output_api):
  """X11 headers pollute the global namespace with macros for common
names so instead code should include "ui/gfx/x/x11.h" which hide the
dangerous macros inside the x11 namespace."""

  # Only check files in ui/gl and ui/gfx for now since that is the
  # only code converted.

  source_file_filter = lambda x: input_api.FilterSourceFile(
    x,
    white_list=tuple([r'.*ui[\\/].*\.(cc|h)$']))
  errors = []
  x11_include_pattern = input_api.re.compile(r'#include\s+<X11/.*\.h>')
  for f in input_api.AffectedSourceFiles(source_file_filter):
    if f.LocalPath().endswith(input_api.os_path.normpath("ui/gfx/x/x11.h")):
      # This is the only file that is allowed to include X11 headers.
      continue
    for line_number, line in f.ChangedContents():
      if input_api.re.search(x11_include_pattern, line):
        errors.append(output_api.PresubmitError(
          '%s:%d includes an X11 header. Include "ui/gfx/x/x11.h" instead.' %
          (f.LocalPath(), line_number)))
  return errors


def CheckChange(input_api, output_api):
  results = []
  results += CheckX11HeaderUsage(input_api, output_api)
  return results


def CheckChangeOnUpload(input_api, output_api):
  return CheckChange(input_api, output_api)


def CheckChangeOnCommit(input_api, output_api):
  return CheckChange(input_api, output_api)
