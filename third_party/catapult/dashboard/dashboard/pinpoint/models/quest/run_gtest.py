# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Quest for running a GTest in Swarming."""

from dashboard.pinpoint.models.quest import run_test


class RunGTest(run_test.RunTest):

  @classmethod
  def FromDict(cls, arguments):
    swarming_extra_args = []

    test = arguments.get('test')
    if test:
      swarming_extra_args.append('--gtest_filter=' + test)

    swarming_extra_args.append('--gtest_repeat=1')

    return cls._FromDict(arguments, swarming_extra_args)
