# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for topology module."""

from __future__ import print_function

from chromite.cbuildbot import topology
from chromite.lib import fake_cidb
from chromite.lib import cros_test_lib


class ToplogyTest(cros_test_lib.TestCase):
  """Unit test of topology module."""

  def setUp(self):
    # Mutually isolate these tests and make them independent of
    # TOPOLOGY_DEFAULTS
    topology.topology = topology.LockedDefaultDict()

  def testWithDB(self):
    fake_db = fake_cidb.FakeCIDBConnection(fake_keyvals={'/foo': 'bar'})
    topology.FetchTopologyFromCIDB(fake_db)
    self.assertEqual(topology.topology.get('/foo'), 'bar')

  def testWithoutDB(self):
    topology.FetchTopologyFromCIDB(None)
    self.assertEqual(topology.topology.get('/foo'), None)

  def testNotFetched(self):
    with self.assertRaises(topology.LockedDictAccessException):
      topology.topology.get('/foo')


def FakeFetchTopologyFromCIDB(keyvals=None):
  """Setup topology without the need for a DB

  args:
    keyvals: optional dictionary to populate topology
  """
  keyvals = keyvals if keyvals != None else {}

  topology.FetchTopologyFromCIDB(None)
  topology.topology.update(keyvals)
  topology.topology.unlock()


class FakeFetchTopologyFromCIDBTest(cros_test_lib.TestCase):
  """Test FakeFetchToplogoyFromCIDB unittest helper function"""

  def setUp(self):
    _resetTopology()

  def testFakeTopologyFromCIDB(self):
    data = {1:'one', 2:'two', 3:'three'}
    FakeFetchTopologyFromCIDB(data)
    self.assertDictContainsSubset(data, topology.topology)

  def testFakeTopologyFromCIDBEmpty(self):
    FakeFetchTopologyFromCIDB()
    # pylint: disable=protected-access
    self.assertFalse(topology.topology._locked)


def _resetTopology():
  """Remove effects of unittests on topology"""
  topology.topology = topology.LockedDefaultDict()
  topology.topology.update(topology.TOPOLOGY_DEFAULTS)
