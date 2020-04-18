# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

def PostUploadHook(cl, change, output_api):
  return output_api.EnsureCQIncludeTrybotsAreAdded(
    cl,
    [
      'master.tryserver.chromium.linux:closure_compilation',
    ],
    'Automatically added optional Closure bots to run on CQ.')


def CheckChangeOnUpload(input_api, output_api):
  return _CommonChecks(input_api, output_api)


def CheckChangeOnCommit(input_api, output_api):
  return _CommonChecks(input_api, output_api)

# For every modified gyp file, warn if the corresponding GN file is not updated.
def _CheckForGNUpdate(input_api, output_api):
  gyp_folders = set()
  for f in input_api.AffectedFiles():
    local_path = f.LocalPath()
    if local_path.endswith('compiled_resources2.gyp'):
      gyp_folders.add(os.path.dirname(local_path))

  for f in input_api.AffectedFiles():
    local_path = f.LocalPath()
    dir_name = os.path.dirname(local_path)
    if local_path.endswith('BUILD.gn') and dir_name in gyp_folders:
      gyp_folders.remove(dir_name)

  if not gyp_folders:
    return []

  return [output_api.PresubmitPromptWarning("""
You may have forgotten to update the BUILD.gn Closure Compilation for the
following folders:
""" + "\n".join(["- " + x for x in gyp_folders]) + """

Ping calamity@ or check go/closure-compile-gn for more details.
""")]

def _CheckForTranslations(input_api, output_api):
  shared_keywords = ['i18n(']
  html_keywords = shared_keywords + ['$118n{']
  js_keywords = shared_keywords + ['I18nBehavior', 'loadTimeData.']

  errors = []

  for f in input_api.AffectedFiles():
    local_path = f.LocalPath()
    # Allow translation in i18n_behavior.js.
    if local_path.endswith('i18n_behavior.js'):
      continue
    # Allow translation in the cr_components directory.
    if 'cr_components' in local_path:
      continue
    keywords = None
    if local_path.endswith('.js'):
      keywords = js_keywords
    elif local_path.endswith('.html'):
      keywords = html_keywords

    if not keywords:
      continue

    for lnum, line in f.ChangedContents():
      if any(line for keyword in keywords if keyword in line):
        errors.append("%s:%d\n%s" % (f.LocalPath(), lnum, line))

  if not errors:
    return []

  return [output_api.PresubmitError("\n".join(errors) + """

Don't embed translations directly in shared UI code. Instead, inject your
translation from the place using the shared code. For an example: see
<cr-dialog>#closeText (http://bit.ly/2eLEsqh).""")]


def _CommonChecks(input_api, output_api):
  results = []
  results += _CheckForTranslations(input_api, output_api)
  results += _CheckForGNUpdate(input_api, output_api)
  results += input_api.canned_checks.CheckPatchFormatted(input_api, output_api,
                                                         check_js=True)
  try:
    import sys
    old_sys_path = sys.path[:]
    cwd = input_api.PresubmitLocalPath()
    sys.path += [input_api.os_path.join(cwd, '..', '..', '..', 'tools')]
    from web_dev_style import presubmit_support
    BLACKLIST = ['ui/webui/resources/js/analytics.js',
                 'ui/webui/resources/js/jstemplate_compiled.js']
    file_filter = lambda f: f.LocalPath() not in BLACKLIST
    results += presubmit_support.CheckStyle(input_api, output_api, file_filter)
  finally:
    sys.path = old_sys_path
  return results
