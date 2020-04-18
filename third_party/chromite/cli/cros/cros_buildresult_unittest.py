# -*- coding: utf-8 -*-
# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests the `cros buildresult` command."""

from __future__ import print_function

import json

from chromite.cli import command_unittest
from chromite.cli.cros import cros_buildresult
from chromite.lib import cros_test_lib


FAKE_BUILD_STATUS = {
    'id': 1234,
    'buildbucket_id': 'buildbucket_value',
    'status': 'pass',
    'artifacts_url': 'fake_artifacts_url',
    'toolchain_url': 'fake_toolchain_url',
    'stages': [
        {'name': 'stage_a', 'status': 'pass'},
        {'name': 'stage_b', 'status': 'pass'},
        {'name': 'stage_c', 'status': 'pass'},
    ],
}


class MockBuildresultCommand(command_unittest.MockCommand):
  """Mock out the `cros buildresult` command."""
  TARGET = 'chromite.cli.cros.cros_buildresult.BuildResultCommand'
  TARGET_CLASS = cros_buildresult.BuildResultCommand
  COMMAND = 'buildresult'


class BuildresultTest(cros_test_lib.MockTestCase):
  """Base class for buildresult command tests."""

  def setUp(self):
    self.cmd_mock = None

  def SetupCommandMock(self, cmd_args):
    """Sets up the `cros buildresult` command mock."""
    self.cmd_mock = MockBuildresultCommand(cmd_args)
    self.StartPatcher(self.cmd_mock)

    return self.cmd_mock.inst.options


class BuildresultReportTest(BuildresultTest):
  """Test the report generation functions."""

  def setUp(self):
    self.maxDiff = None

  def testReport(self):
    result = cros_buildresult.Report([FAKE_BUILD_STATUS])
    expected = '''cidb_id: 1234
buildbucket_id: buildbucket_value
status: pass
artifacts_url: fake_artifacts_url
toolchain_url: fake_toolchain_url
stages:
  stage_a: pass
  stage_b: pass
  stage_c: pass

'''

    self.assertEqual(expected, result)

  def testReportJson(self):
    result = cros_buildresult.ReportJson([FAKE_BUILD_STATUS])
    expected = {
        'buildbucket_value': {
            'cidb_id': 1234,
            'buildbucket_id': 'buildbucket_value',
            'status': 'pass',
            'artifacts_url': 'fake_artifacts_url',
            'toolchain_url': 'fake_toolchain_url',
            'stages': {
                'stage_a': 'pass',
                'stage_b': 'pass',
                'stage_c': 'pass',
            },
        },
    }

    self.assertEqual(expected, json.loads(result))
