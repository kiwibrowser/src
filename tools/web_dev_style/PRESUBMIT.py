# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


def CheckChangeOnUpload(*args):
  return _CommonChecks(*args)


def CheckChangeOnCommit(*args):
  return _CommonChecks(*args)


def _CommonChecks(input_api, output_api):
  cwd = input_api.PresubmitLocalPath()
  path = input_api.os_path
  files = [path.basename(f.LocalPath()) for f in input_api.AffectedFiles()]
  tests = []

  if 'css_checker.py' in files:
    tests.append(path.join(cwd, 'css_checker_test.py'))

  utils_changed = 'regex_check.py' in files or 'test_util.py' in files

  if utils_changed or any(f for f in files if f.startswith('html_checker')):
    tests.append(path.join(cwd, 'html_checker_test.py'))

  if utils_changed or any(f for f in files if f.startswith('js_checker')):
    tests.append(path.join(cwd, 'js_checker_test.py'))
    tests.append(path.join(cwd, 'js_checker_eslint_test.py'))

  if utils_changed or any(f for f in files if f.startswith('resource_checker')):
    tests.append(path.join(cwd, 'resource_checker_test.py'))

  return input_api.canned_checks.RunUnitTests(input_api, output_api, tests)
