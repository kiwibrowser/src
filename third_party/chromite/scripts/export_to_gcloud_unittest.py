# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for export_to_gcloud."""

from __future__ import print_function

from gcloud import datastore
import io

from chromite.lib import cros_test_lib
from chromite.scripts import export_to_gcloud

class GetEntitiesTest(cros_test_lib.TestCase):
  """Test that GetEntities behaves correctly."""

  _BASIC_JSON = """{"id": ["Foo", 1], "foo": "bar"}
{"id": ["Bar", 1], "bar": "baz"}
{"id": ["Bar", 2], "foo": "qux", "parent": ["Bar", 1]}"""

  _DUPE_KEY_JSON = _BASIC_JSON + '\n{"id": ["Bar", 1], "bar": "baz"}'

  def testBasicFunctionality(self):
    """Tests that GetEntities handles well formed input as expected."""
    p = 'foo_project'
    with io.BytesIO(self._BASIC_JSON) as f:
      entities = list(export_to_gcloud.GetEntities(p, f))

    self.assertEqual(len(entities), 3)
    self.assertEqual(
        entities[0].key, datastore.key.Key('Foo', 1, project=p))
    self.assertEqual(
        entities[2].key, datastore.key.Key('Bar', 1, 'Bar', 2, project=p))

  def testOuterParent(self):
    """Tests that GetEntities handles outer_parent_key as expected."""
    p = 'foo_project'
    par_id = datastore.key.Key('Parent', 1, project=p)
    with io.BytesIO(self._BASIC_JSON) as f:
      entities = list(export_to_gcloud.GetEntities(p, f,
                                                   outer_parent_key=par_id))

    self.assertEqual(len(entities), 3)
    self.assertEqual(
        entities[0].key, datastore.key.Key('Parent', 1, 'Foo', 1, project=p))
    self.assertEqual(
        entities[2].key,
        datastore.key.Key('Parent', 1, 'Bar', 1, 'Bar', 2, project=p))

  def testNamespace(self):
    """Tests that GetEntities handles namespace as expected."""
    p = 'foo_project'
    n = 'foo_namespace'
    with io.BytesIO(self._BASIC_JSON) as f:
      entities = list(export_to_gcloud.GetEntities(p, f, namespace=n))

    self.assertEqual(len(entities), 3)
    self.assertEqual(
        entities[0].key, datastore.key.Key('Foo', 1, project=p, namespace=n))
    self.assertEqual(
        entities[2].key,
        datastore.key.Key('Bar', 1, 'Bar', 2, project=p, namespace=n))

  def testDuplicateKey(self):
    """Tests that GetEntities throws a ValueError on duplicate keys."""
    p = 'foo_project'
    with self.assertRaises(export_to_gcloud.DuplicateKeyError):
      with io.BytesIO(self._DUPE_KEY_JSON) as f:
        list(export_to_gcloud.GetEntities(p, f))
