# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import sys

from dashboard.common import testing_common
from dashboard.common import utils
from dashboard.models import histogram
from tracing.value.diagnostics import reserved_infos


class SparseDiagnosticTest(testing_common.TestCase):
  """Test case for functions in SparseDiagnostic."""

  def setUp(self):
    super(SparseDiagnosticTest, self).setUp()
    self.SetCurrentUser('foo@bar.com', is_admin=True)

  def _AddMockData(self, test_key):
    data_samples = {
        'owners': [
            {
                'type': 'GenericSet',
                'guid': '1',
                'values': ['1']
            },
            {
                'type': 'GenericSet',
                'guid': '1',
                'values': ['2']
            },
            {
                'type': 'GenericSet',
                'guid': '1',
                'values': ['3']
            },
        ],
        'bugs': [
            {
                'type': 'GenericSet',
                'guid': '1',
                'values': ['a']
            },
            {
                'type': 'GenericSet',
                'guid': '1',
                'values': ['b']
            },
            {
                'type': 'GenericSet',
                'guid': '1',
                'values': ['c']
            },
        ]
    }
    for k, diagnostic_samples in data_samples.iteritems():
      for i in xrange(len(diagnostic_samples)):
        start_revision = i * 10
        end_revision = (i + 1) * 10 - 1
        if i == len(diagnostic_samples) - 1:
          end_revision = sys.maxint

        e = histogram.SparseDiagnostic(
            data=diagnostic_samples[i], test=test_key,
            start_revision=start_revision, end_revision=end_revision,
            name=k, internal_only=False)
        e.put()

  def testFixupDiagnostics_Middle_FixesRange(self):
    test_key = utils.TestKey('Chromium/win7/foo')

    self._AddMockData(test_key)

    data = {
        'type': 'GenericSet',
        'guid': '1',
        'values': ['10']
    }

    e = histogram.SparseDiagnostic(
        data=data, test=test_key,
        start_revision=5, end_revision=sys.maxint,
        name='owners', internal_only=False)
    e.put()

    histogram.SparseDiagnostic.FixDiagnostics(test_key).get_result()

    expected = {
        'owners': [(0, 4), (5, 9), (10, 19), (20, sys.maxint)],
        'bugs': [(0, 9), (10, 19), (20, sys.maxint)],
    }
    diags = histogram.SparseDiagnostic.query().fetch()
    for d in diags:
      self.assertIn((d.start_revision, d.end_revision), expected[d.name])
      expected[d.name].remove((d.start_revision, d.end_revision))
    self.assertEqual(0, len(expected['owners']))
    self.assertEqual(0, len(expected['bugs']))

  def testFixupDiagnostics_End_FixesRange(self):
    test_key = utils.TestKey('Chromium/win7/foo')

    self._AddMockData(test_key)

    data = {
        'type': 'GenericSet',
        'guid': '1',
        'values': ['10']
    }

    e = histogram.SparseDiagnostic(
        data=data, test=test_key,
        start_revision=100, end_revision=sys.maxint,
        name='owners', internal_only=False)
    e.put()

    histogram.SparseDiagnostic.FixDiagnostics(test_key).get_result()

    expected = {
        'owners': [(0, 9), (10, 19), (20, 99), (100, sys.maxint)],
        'bugs': [(0, 9), (10, 19), (20, sys.maxint)],
    }
    diags = histogram.SparseDiagnostic.query().fetch()
    for d in diags:
      self.assertIn((d.start_revision, d.end_revision), expected[d.name])
      expected[d.name].remove((d.start_revision, d.end_revision))
    self.assertEqual(0, len(expected['owners']))
    self.assertEqual(0, len(expected['bugs']))

  def testFixupDiagnostics_DifferentTestPath_NoChange(self):
    test_key1 = utils.TestKey('Chromium/win7/1')
    test_key2 = utils.TestKey('Chromium/win7/2')

    self._AddMockData(test_key1)
    self._AddMockData(test_key2)

    data = {
        'type': 'GenericSet',
        'guid': '1',
        'values': ['10']
    }

    e = histogram.SparseDiagnostic(
        data=data, test=test_key1,
        start_revision=5, end_revision=sys.maxint,
        name='owners', internal_only=False)
    e.put()

    histogram.SparseDiagnostic.FixDiagnostics(test_key2).get_result()

    expected = {
        'owners': [(0, 9), (10, 19), (20, sys.maxint)],
        'bugs': [(0, 9), (10, 19), (20, sys.maxint)],
    }
    diags = histogram.SparseDiagnostic.query(
        histogram.SparseDiagnostic.test == test_key2).fetch()
    for d in diags:
      self.assertIn((d.start_revision, d.end_revision), expected[d.name])
      expected[d.name].remove((d.start_revision, d.end_revision))
    self.assertEqual(0, len(expected['owners']))
    self.assertEqual(0, len(expected['bugs']))

  def testFixupDiagnostics_NotUnique_NoChange(self):
    test_key = utils.TestKey('Chromium/win7/foo')

    self._AddMockData(test_key)

    data = {
        'type': 'GenericSet',
        'guid': '1',
        'values': ['1']
    }

    e = histogram.SparseDiagnostic(
        data=data, test=test_key,
        start_revision=5, end_revision=sys.maxint,
        name='owners', internal_only=False)
    e.put()

    histogram.SparseDiagnostic.FixDiagnostics(test_key).get_result()

    expected = {
        'owners': [(0, 9), (10, 19), (20, sys.maxint)],
        'bugs': [(0, 9), (10, 19), (20, sys.maxint)],
    }
    diags = histogram.SparseDiagnostic.query(
        histogram.SparseDiagnostic.test == test_key).fetch()
    for d in diags:
      self.assertIn((d.start_revision, d.end_revision), expected[d.name])
      expected[d.name].remove((d.start_revision, d.end_revision))
    self.assertEqual(0, len(expected['owners']))
    self.assertEqual(0, len(expected['bugs']))

  def testGetMostRecentValuesByNames_ReturnAllData(self):
    data_samples = [
        {
            'type': 'GenericSet',
            'guid': 'eb212e80-db58-4cbd-b331-c2245ecbb826',
            'values': ['alice@chromium.org']
        },
        {
            'type': 'GenericSet',
            'guid': 'eb212e80-db58-4cbd-b331-c2245ecbb827',
            'values': ['abc']
        }]

    test_key = utils.TestKey('Chromium/win7/foo')
    entity = histogram.SparseDiagnostic(
        data=json.dumps(data_samples[0]), test=test_key, start_revision=1,
        end_revision=sys.maxint, id=data_samples[0]['guid'],
        name=reserved_infos.OWNERS.name)
    entity.put()

    entity = histogram.SparseDiagnostic(
        data=json.dumps(data_samples[1]), test=test_key, start_revision=1,
        end_revision=sys.maxint, id=data_samples[1]['guid'],
        name=reserved_infos.BUG_COMPONENTS.name)
    entity.put()

    lookup_result = histogram.SparseDiagnostic.GetMostRecentValuesByNames(
        test_key, set([reserved_infos.OWNERS.name,
                       reserved_infos.BUG_COMPONENTS.name]))

    self.assertEqual(lookup_result.get(reserved_infos.OWNERS.name),
                     ['alice@chromium.org'])
    self.assertEqual(lookup_result.get(reserved_infos.BUG_COMPONENTS.name),
                     ['abc'])

  def testGetMostRecentValuesByNames_ReturnsNoneIfNoneFound(self):
    data_sample = {
        'type': 'GenericSet',
        'guid': 'eb212e80-db58-4cbd-b331-c2245ecbb826',
        'values': ['alice@chromium.org']
    }

    test_key = utils.TestKey('Chromium/win7/foo')
    entity = histogram.SparseDiagnostic(
        data=json.dumps(data_sample), test=test_key, start_revision=1,
        end_revision=sys.maxint, id=data_sample['guid'],
        name=reserved_infos.OWNERS.name)
    entity.put()

    lookup_result = histogram.SparseDiagnostic.GetMostRecentValuesByNames(
        test_key, set([reserved_infos.OWNERS.name,
                       reserved_infos.BUG_COMPONENTS.name]))


    self.assertEqual(lookup_result.get(reserved_infos.OWNERS.name),
                     ['alice@chromium.org'])
    self.assertIsNone(lookup_result.get(reserved_infos.BUG_COMPONENTS.name))

  def testGetMostRecentValuesByNames_ReturnsNoneIfNoName(self):
    data_sample = {
        'guid': 'abc',
        'osName': 'linux',
        'type': 'DeviceInfo'
    }

    test_key = utils.TestKey('Chromium/win7/foo')
    entity = histogram.SparseDiagnostic(
        data=json.dumps(data_sample), test=test_key, start_revision=1,
        end_revision=sys.maxint, id=data_sample['guid'])
    entity.put()

    lookup_result = histogram.SparseDiagnostic.GetMostRecentValuesByNames(
        test_key, set([reserved_infos.OWNERS.name,
                       reserved_infos.BUG_COMPONENTS.name]))

    self.assertIsNone(lookup_result.get(reserved_infos.OWNERS.name))
    self.assertIsNone(lookup_result.get(reserved_infos.BUG_COMPONENTS.name))

  def testGetMostRecentValuesByNames_RaisesErrorIfDuplicateName(self):
    data_samples = [
        {
            'type': 'GenericSet',
            'guid': 'eb212e80-db58-4cbd-b331-c2245ecbb826',
            'values': ['alice@chromium.org']
        },
        {
            'type': 'GenericSet',
            'guid': 'eb212e80-db58-4cbd-b331-c2245ecbb827',
            'values': ['bob@chromium.org']
        }]

    test_key = utils.TestKey('Chromium/win7/foo')
    entity = histogram.SparseDiagnostic(
        data=json.dumps(data_samples[0]), test=test_key, start_revision=1,
        end_revision=sys.maxint, id=data_samples[0]['guid'],
        name=reserved_infos.OWNERS.name)
    entity.put()

    entity = histogram.SparseDiagnostic(
        data=json.dumps(data_samples[1]), test=test_key, start_revision=1,
        end_revision=sys.maxint, id=data_samples[1]['guid'],
        name=reserved_infos.OWNERS.name)
    entity.put()

    self.assertRaises(
        AssertionError,
        histogram.SparseDiagnostic.GetMostRecentValuesByNames,
        test_key,
        set([reserved_infos.OWNERS.name, reserved_infos.BUG_COMPONENTS.name]))
