# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test manual_evaluator module."""

from __future__ import print_function

import os

from chromite.cros_bisect import common
from chromite.cros_bisect import manual_evaluator
from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import osutils


class TestManualEvaluator(cros_test_lib.MockTempDirTestCase,
                          cros_test_lib.OutputTestCase):
  """Tests ManualEvaluator class."""
  BUILD_LABEL = 'test_build'
  BUILD_LABEL2 = 'test_build2'


  def setUp(self):
    """Sets up default evaluator."""
    options = cros_test_lib.EasyAttr(base_dir=self.tempdir, reuse_eval=True)
    self.evaluator = manual_evaluator.ManualEvaluator(options)

  def testGetReportPath(self):
    """Tests GetReportPath()."""
    self.assertEqual(
        os.path.join(self.tempdir, 'reports',
                     'manual.%s.report' % self.BUILD_LABEL),
        self.evaluator.GetReportPath(self.BUILD_LABEL))

  def testEvaluate(self):
    """Tests Evaluate()."""
    report_path = self.evaluator.GetReportPath(self.BUILD_LABEL)

    m = self.PatchObject(cros_build_lib, 'GetInput')
    m.return_value = 'yes'
    self.assertEqual(common.Score([1.0]),
                     self.evaluator.Evaluate(None, self.BUILD_LABEL))
    self.assertEqual('1', osutils.ReadFile(report_path))

    m.return_value = 'no'
    self.assertEqual(common.Score([0.0]),
                     self.evaluator.Evaluate(None, self.BUILD_LABEL))
    self.assertEqual('0', osutils.ReadFile(report_path))

  def testCheckLastEvaluate(self):
    """Tests CheckLastEvaluate()."""
    # Report does not exist.
    self.assertFalse(self.evaluator.CheckLastEvaluate(self.BUILD_LABEL))

    # Generate a report for BUILD_LABEL
    m = self.PatchObject(cros_build_lib, 'GetInput')
    m.return_value = 'yes'
    self.evaluator.Evaluate(None, self.BUILD_LABEL)

    # Found latest evaluation result.
    self.assertEqual(common.Score([1.0]),
                     self.evaluator.CheckLastEvaluate(self.BUILD_LABEL))

    # Yet another unseen build.
    self.assertFalse(self.evaluator.CheckLastEvaluate(self.BUILD_LABEL2))

    # Generate a report for BUILD_LABEL2
    m.return_value = 'no'
    self.evaluator.Evaluate(None, self.BUILD_LABEL2)

    # Found latest evaluation result.
    self.assertEqual(common.Score([1.0]),
                     self.evaluator.CheckLastEvaluate(self.BUILD_LABEL))
    self.assertEqual(common.Score([0.0]),
                     self.evaluator.CheckLastEvaluate(self.BUILD_LABEL2))


  def testCheckLastLabelWithReuseEvalOptionUnset(self):
    """Tests CheckLastEvaluate() with options.reuse_eval unset."""
    options = cros_test_lib.EasyAttr(base_dir=self.tempdir, reuse_eval=False)
    self.evaluator = manual_evaluator.ManualEvaluator(options)

    # Report does not exist.
    self.assertFalse(self.evaluator.CheckLastEvaluate(self.BUILD_LABEL))

    # Generate a report for BUILD_LABEL
    m = self.PatchObject(cros_build_lib, 'GetInput')
    m.return_value = 'yes'
    self.evaluator.Evaluate(None, self.BUILD_LABEL)

    # Unlike testCheckLastEvaluate(), it returns empty Score() object.
    self.assertFalse(self.evaluator.CheckLastEvaluate(self.BUILD_LABEL))
