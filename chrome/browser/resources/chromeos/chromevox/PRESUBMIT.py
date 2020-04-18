# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit script for ChromeVox."""

def CheckChangeOnUpload(input_api, output_api):
  paths = input_api.AbsoluteLocalPaths()

  def ShouldCheckFile(path):
    return path.endswith('.js') or path.endswith('.py')

  def ScriptFilter(path):
    return (path.endswith('check_chromevox.py') or
            path.endswith('jscompilerwrapper.py') or
            path.endswith('jsbundler.py'))

  # Only care about changes to JS files or the scripts that check them.
  paths = [p for p in paths if ShouldCheckFile(p)]
  if not paths:
    return []

  # If changing what the presubmit script uses, run the check on all
  # scripts.  Otherwise, let CheckChromeVox figure out what scripts to
  # compile, if any, based on the changed paths.
  if any((ScriptFilter(p) for p in paths)):
    paths = None

  import sys
  if not sys.platform.startswith('linux'):
    return []
  sys.path.insert(0, input_api.os_path.join(
      input_api.PresubmitLocalPath(), 'tools'))
  try:
    from check_chromevox import CheckChromeVox
  finally:
    sys.path.pop(0)
  success, output = CheckChromeVox(paths)
  if not success:
    return [output_api.PresubmitError(
        'ChromeVox closure compilation failed',
        long_text=output)]
  return []
