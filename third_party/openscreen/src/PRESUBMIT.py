# Copyright (c) 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


def _CheckDeps(input_api, output_api):
  results = []
  import sys
  original_sys_path = sys.path
  try:
    sys.path = sys.path + [input_api.os_path.join(
        input_api.PresubmitLocalPath(), 'buildtools', 'checkdeps')]
    import checkdeps
    from cpp_checker import CppChecker
    from rules import Rule
  finally:
    sys.path = original_sys_path

  added_includes = []
  for f in input_api.AffectedFiles():
    if CppChecker.IsCppFile(f.LocalPath()):
      changed_lines = [line for _, line in f.ChangedContents()]
      added_includes.append([f.AbsoluteLocalPath(), changed_lines])

  deps_checker = checkdeps.DepsChecker(input_api.PresubmitLocalPath())
  violations = deps_checker.CheckAddedCppIncludes(added_includes)
  for path, rule_type, rule_description in violations:
    relpath = input_api.os_path.relpath(path, input_api.PresubmitLocalPath())
    error_description = '%s\n    %s' % (relpath, rule_description)
    if rule_type == Rule.DISALLOW:
      results.append(output_api.PresubmitError(error_description))
    else:
      results.append(output_api.PresubmitPromptWarning(error_description))
  return results


def _CommonChecks(input_api, output_api):
  results = []
  # TODO(issues/43): Probably convert this to python so we can give more
  # detailed errors.
  presubmit_sh_result = input_api.subprocess.call(
      input_api.PresubmitLocalPath() + '/PRESUBMIT.sh')
  if presubmit_sh_result != 0:
    results.append(output_api.PresubmitError('PRESUBMIT.sh failed'))
  results.extend(_CheckDeps(input_api, output_api))
  return results


def CheckChangeOnUpload(input_api, output_api):
  results = []
  results.extend(_CommonChecks(input_api, output_api))
  results.extend(
      input_api.canned_checks.CheckChangedLUCIConfigs(input_api, output_api))
  return results


def CheckChangeOnCommit(input_api, output_api):
  results = []
  results.extend(_CommonChecks(input_api, output_api))
  return results
