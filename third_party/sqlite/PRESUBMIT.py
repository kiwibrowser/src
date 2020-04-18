# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Presubmit tests for /third_party/sqlite.

Runs Python unit tests in /third_party/sqlite/scripts on upload.
"""


def CheckChangeOnUpload(input_api, output_api):
  results = []

  results += input_api.RunTests(
      input_api.canned_checks.GetUnitTests(input_api, output_api, [
          'scripts/extract_sqlite_api_unittest.py'
      ]))

  return results
