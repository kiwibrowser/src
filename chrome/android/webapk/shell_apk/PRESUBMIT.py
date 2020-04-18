# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit script for changes affecting chrome/android/webapk/shell_apk

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools.

This presubmit checks for two rules:
1. If anything in shell_apk/ directory has changed (excluding test files),
$WAM_MINT_TRIGGER_VARIABLE should be updated.
2. If $CHROME_UPDATE_TIRGGER_VARIABLE is changed in
$SHELL_APK_VERSION_LOCAL_PATH, $SHELL_APK_VERSION_LOCAL_PATH should be the
only changed file and changing $CHROME_UPDATE_TIRGGER_VARIABLE should be
the only change.
"""

WAM_MINT_TRIGGER_VARIABLE = r'template_shell_apk_version'
CHROME_UPDATE_TRIGGER_VARIABLE = r'expected_shell_apk_version'

ANDROID_MANIFEST_LOCAL_PATH = r'AndroidManifest.xml'
RES_LOCAL_PATH = r'res/'
SHELL_APK_VERSION_LOCAL_PATH = r'shell_apk_version.gni'
SRC_LOCAL_PATH = r'src/org/chromium/webapk/shell_apk/'


def _DoChangedContentsContain(changed_contents, key):
  for line_num, line in changed_contents:
    if key in line:
      return True
  return False


def _CheckChromeUpdateTriggerRule(input_api, output_api):
  for f in input_api.AffectedFiles():
    local_path = input_api.os_path.relpath(f.AbsoluteLocalPath(),
                                           input_api.PresubmitLocalPath())

    if local_path == SHELL_APK_VERSION_LOCAL_PATH:
      if _DoChangedContentsContain(f.ChangedContents(),
                                 CHROME_UPDATE_TRIGGER_VARIABLE):
        if (len(f.ChangedContents()) != 1 or
            len(input_api.AffectedFiles()) != 1):
          return [
              output_api.PresubmitError(
                  '{} in {} must be updated in a standalone CL.'.format(
                      CHROME_UPDATE_TRIGGER_VARIABLE,
                      SHELL_APK_VERSION_LOCAL_PATH))
          ]

  return []


def _CheckWamMintTriggerRule(input_api, output_api):
  problems = []

  wam_mint_trigger_update_needed = False
  wam_mint_trigger_is_updated = False
  for f in input_api.AffectedFiles():
    local_path = input_api.os_path.relpath(f.AbsoluteLocalPath(),
                                           input_api.PresubmitLocalPath())

    if (local_path == ANDROID_MANIFEST_LOCAL_PATH or
        local_path.startswith(RES_LOCAL_PATH) or
        local_path.startswith(SRC_LOCAL_PATH)):
      wam_mint_trigger_update_needed = True
      problems.append(local_path)
    elif local_path == SHELL_APK_VERSION_LOCAL_PATH:
      if _DoChangedContentsContain(f.ChangedContents(),
                                 WAM_MINT_TRIGGER_VARIABLE):
        wam_mint_trigger_is_updated = True

  if wam_mint_trigger_update_needed and not wam_mint_trigger_is_updated:
    return [output_api.PresubmitPromptWarning(
        '{} in {} needs to updated due to changes in:'.format(
            WAM_MINT_TRIGGER_VARIABLE, SHELL_APK_VERSION_LOCAL_PATH),
        items=problems)]

  return []


def _CommonChecks(input_api, output_api):
  """Checks common to both upload and commit."""
  result = []
  result.extend(_CheckChromeUpdateTriggerRule(input_api, output_api))
  result.extend(_CheckWamMintTriggerRule(input_api, output_api))

  return result


def CheckChangeOnUpload(input_api, output_api):
  return _CommonChecks(input_api, output_api)


def CheckChangeOnCommit(input_api, output_api):
  return _CommonChecks(input_api, output_api)
