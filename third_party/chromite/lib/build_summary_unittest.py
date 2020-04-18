# -*- coding: utf-8 -*-
# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test the build_summary module."""

from __future__ import print_function

import json

from chromite.lib import build_summary
from chromite.lib import constants
from chromite.lib import cros_test_lib


class BuildSummaryTests(cros_test_lib.TestCase):
  """Test the BuildSummary class."""

  def testPartialJson(self):
    """Test that fields not present in JSON aren't changed."""
    summary = build_summary.BuildSummary()
    raw_json = '{"build_number": 10}'
    summary.from_json(raw_json)

    # Only build_number changes.
    default_summary = build_summary.BuildSummary()
    self.assertEqual(summary.build_number, 10)
    self.assertEqual(summary.master_build_id, default_summary.master_build_id)
    self.assertEqual(summary.status, default_summary.status)

  def testInvalidJson(self):
    """Test that invalid JSON raises an exception."""
    bad_json = "{"
    summary = build_summary.BuildSummary()
    self.assertRaises(ValueError, summary.from_json, bad_json)

  def testJsonUnknownKeys(self):
    """Test that unknown keys in JSON don't produce any effect."""
    raw_json = '{"unknown_key": 50}'
    summary = build_summary.BuildSummary()
    summary.from_json(raw_json)
    self.assertFalse(hasattr(summary, 'unknown_key'))

  def testJsonInvalidStatus(self):
    """Test that an invalid status in JSON is ignored."""
    raw_json = '{"status": "whoops"}'
    summary = build_summary.BuildSummary()
    summary.from_json(raw_json)
    self.assertEqual(summary.status, constants.BUILDER_STATUS_MISSING)

  def testRoundTrip(self):
    """Test that a BuildSummary can round-trip through JSON."""
    summary1 = build_summary.BuildSummary()
    summary1.build_number = 314
    summary1.buildbucket_id = 3140000
    summary1.status = constants.BUILDER_STATUS_PASSED
    summary1.master_build_id = 2718

    encoded = summary1.to_json()
    summary2 = build_summary.BuildSummary()
    summary2.from_json(encoded)

    self.assertEqual(summary1, summary2)

  def testJsonNoUnecessaryKeys(self):
    """Test that the JSON output doesn't contain keys for zero values."""
    summary = build_summary.BuildSummary(build_number=5)
    out_fields = json.loads(summary.to_json())
    self.assertSetEqual(set(out_fields.keys()), set(['build_number', 'status']))

  def testAttributesListMatchObject(self):
    """Test that all attributes are listed in _PERSIST_ATTRIBUTES."""
    summary = build_summary.BuildSummary()
    # pylint: disable=protected-access
    self.assertSetEqual(set(vars(summary).keys()),
                        set(summary._PERSIST_ATTRIBUTES))

  def testBuildSummary(self):
    summary = build_summary.BuildSummary()
    description = summary.build_description()
    self.assertEqual(description, 'local build')

    # buildbucket_id string
    summary = build_summary.BuildSummary(buildbucket_id='bb_id')
    description = summary.build_description()
    self.assertEqual(description, 'buildbucket_id=bb_id')

    # buildbucket_id int (can this really happen)
    summary = build_summary.BuildSummary(buildbucket_id=1234)
    description = summary.build_description()
    self.assertEqual(description, 'buildbucket_id=1234')

    # build_number int.
    summary = build_summary.BuildSummary(build_number=12)
    description = summary.build_description()
    self.assertEqual(description, 'build_number=12')

    # build_number str.
    summary = build_summary.BuildSummary(build_number='12')
    description = summary.build_description()
    self.assertEqual(description, 'build_number=12')
