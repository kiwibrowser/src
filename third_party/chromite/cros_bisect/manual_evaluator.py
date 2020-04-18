# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Asks users if the commit is good or bad."""

from __future__ import print_function

import os

from chromite.cros_bisect import common
from chromite.cros_bisect import evaluator
from chromite.lib import cros_build_lib
from chromite.lib import osutils


class ManualEvaluator(evaluator.Evaluator):
  """Manual evaluator."""

  # Binary evaluator.
  THRESHOLD = 0.5

  def __init__(self, options):
    super(ManualEvaluator, self).__init__(options)

  def GetReportPath(self, build_label):
    """Obtains report file path.

    Args:
      build_label: current build label to run the evaluation.

    Returns:
      Report file path.
    """
    return os.path.join(self.report_base_dir, 'manual.%s.report' % build_label)

  def Evaluate(self, unused_remote, build_label, unused_repeat=1):
    """Prompts user if the build is good or bad.

    Args:
      unused_remote: Unused args.
      build_label: Build label used for part of report filename and log message.
      unused_repeat: Unused args.

    Returns:
      Score([1.0]) if it is a good build. Otherwise, Score([0.0]).
    """
    report_path = self.GetReportPath(build_label)
    prompt = 'Is %s a good build on the DUT?' % build_label
    is_good = cros_build_lib.BooleanPrompt(prompt=prompt)
    score = 1.0 if is_good else 0.0
    osutils.WriteFile(report_path, '%d' % score)

    return common.Score([score])

  def CheckLastEvaluate(self, build_label, unused_repeat=1):
    """Checks if previous evaluate report is available.

    Args:
      build_label: Build label used for part of report filename and log message.
      unused_repeat: Unused.

    Returns:
      Score([1.0]) if previous result for the build_label is 'Yes'.
      Score([0.0]) if previous result for the build_label is 'No'.
      Score() if previous result does not exist or reuse_eval is unset.
    """
    if self.reuse_eval:
      report_path = self.GetReportPath(build_label)
      if os.path.isfile(report_path):
        content = osutils.ReadFile(report_path)
        if content == '1':
          return common.Score([1.0])
        elif content == '0':
          return common.Score([0.0])

    return common.Score()
