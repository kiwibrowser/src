#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import js_checker
import json
import os
import sys
import unittest
import tempfile


_HERE_PATH = os.path.dirname(__file__)
sys.path.append(os.path.join(_HERE_PATH, '..', '..'))

from PRESUBMIT_test_mocks import MockInputApi, MockOutputApi, MockFile


class JsCheckerEsLintTest(unittest.TestCase):
  def tearDown(self):
    os.remove(self._tmp_file)

  def _runChecks(self, file_contents):
    tmp_args = {'suffix': '.js', 'dir': _HERE_PATH, 'delete': False}
    with tempfile.NamedTemporaryFile(**tmp_args) as f:
      self._tmp_file = f.name
      f.write(file_contents)

    input_api = MockInputApi()
    input_api.files = [MockFile(os.path.abspath(self._tmp_file), '')]
    input_api.presubmit_local_path = _HERE_PATH

    checker = js_checker.JSChecker(input_api, MockOutputApi())
    return checker.RunEsLintChecks(input_api.AffectedFiles(), format='json')

  def _assertError(self, results_json, rule_id, line):
    self.assertEqual(1, len(results_json))
    results = json.loads(results_json[0].message)
    self.assertEqual(1, len(results))
    self.assertEqual(1, len(results[0].get('messages')))
    message = results[0].get('messages')[0]
    self.assertEqual(rule_id, message.get('ruleId'))
    self.assertEqual(line, message.get('line'))

  def testGetElementByIdCheck(self):
    results_json = self._runChecks('var a = document.getElementById(\'foo\');')
    self._assertError(results_json, 'no-restricted-properties', 1)

  def testPrimitiveWrappersCheck(self):
    results_json = self._runChecks('var a = new Number(1);')
    self._assertError(results_json, 'no-new-wrappers', 1)


if __name__ == '__main__':
  unittest.main()
