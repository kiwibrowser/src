# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import sys
import webapp2
import webtest

from google.appengine.ext import ndb

from dashboard import get_diagnostics
from dashboard.common import testing_common
from dashboard.models import histogram

_TEST_DIAGNOSTIC_DATA = [
    {
        'start_revision': 1,
        'end_revision': 3,
        'test_path': 'foobar',
        'name': 'tag_map',
        'data': {'type': 'GenericSet', 'guid': 'abc', 'values': ['foo', 'bar']}
    }, {
        'start_revision': 5,
        'end_revision': 7,
        'test_path': 'foobar',
        'name': 'tag_map',
        'data': {'type': 'GenericSet', 'guid': 'def', 'values': ['foo', 'bar']}
    }, {
        'start_revision': 12,
        'end_revision': 15,
        'test_path': 'foobar',
        'name': 'tag_map',
        'data': {'type': 'GenericSet', 'guid': 'ghi', 'values': ['foo', 'bar']}
    }, {
        'start_revision': 3,
        'end_revision': 8,
        'test_path': 'foobar',
        'name': 'tag_map',
        'data': {'type': 'GenericSet', 'guid': 'jkl', 'values': ['foo', 'bar']}
    }, {
        'start_revision': 10,
        'end_revision': sys.maxint,
        'test_path': 'abcdef',
        'name': 'tag_map',
        'data': {'type': 'GenericSet', 'guid': 'mno', 'values': ['foo', 'bar']}
    }
]


class GetDiagnosticsTest(testing_common.TestCase):

  def setUp(self):
    super(GetDiagnosticsTest, self).setUp()
    app = webapp2.WSGIApplication([
        ('/get_diagnostics', get_diagnostics.GetDiagnosticsHandler)])
    self.testapp = webtest.TestApp(app)
    testing_common.SetIsInternalUser('foo@bar.com', True)
    self.SetCurrentUser('foo@bar.com', is_admin=True)

  def _AddMockData(self, diagnostic_data):
    for diagnostic in diagnostic_data:
      histogram.SparseDiagnostic(
          id=diagnostic['data']['guid'],
          test=ndb.Key('TestMetadata', diagnostic['test_path']),
          start_revision=diagnostic['start_revision'],
          end_revision=diagnostic['end_revision'],
          data=diagnostic['data'],
          name=diagnostic['name']).put()

  def _GenerateData(self, num_entries):
    mock_data = []
    for entry in range(num_entries):
      mock_data.append({
          'start_revision': entry,
          'end_revision': entry + 10,
          'test_path': 'foobar',
          'name': 'tag_map',
          'data': {
              'type': 'GenericSet',
              'guid': 'guid' + str(entry),
              'values': ['foo', 'bar']
              }
          })
    return mock_data

  def testGetDiagnosticsByGuid(self):
    self._AddMockData(_TEST_DIAGNOSTIC_DATA)

    response = self.testapp.post(
        '/get_diagnostics', {'guid': _TEST_DIAGNOSTIC_DATA[0]['data']['guid']})
    data = json.loads(response.body)

    self.assertEqual([_TEST_DIAGNOSTIC_DATA[0]], data)

  def testGetDiagnosticsByGuidAndOtherParamsFails(self):
    self._AddMockData(_TEST_DIAGNOSTIC_DATA)

    self.testapp.post(
        '/get_diagnostics', {
            'guid': _TEST_DIAGNOSTIC_DATA[0]['data']['guid'],
            'test_path': 'foobar'
        }, status=400)

  def testGetMultipleDiagnosticsByRevisionRange(self):
    self._AddMockData(_TEST_DIAGNOSTIC_DATA)

    response = self.testapp.post(
        '/get_diagnostics', {
            'start_revision': 4,
            'end_revision': 10,
            'test_path': 'foobar'
        }
    )
    data = json.loads(response.body)

    self.assertEqual([_TEST_DIAGNOSTIC_DATA[3], _TEST_DIAGNOSTIC_DATA[1]], data)

  def testGetDiagnosticsByStartRevisionFails(self):
    self._AddMockData(_TEST_DIAGNOSTIC_DATA)

    self.testapp.post(
        '/get_diagnostics', {
            'start_revision': 4,
            'test_path': 'foobar'
        }, status=400)

  def testGetMultipleDiagnosticsByEndRevision(self):
    self._AddMockData(_TEST_DIAGNOSTIC_DATA)
    self.testapp.post(
        '/get_diagnostics', {
            'end_revision': 10,
            'test_path': 'foobar'
        }, status=400)

  def testGetLastDiagnostic(self):
    self._AddMockData(_TEST_DIAGNOSTIC_DATA)

    response = self.testapp.post(
        '/get_diagnostics', {
            'end_revision': 'last',
            'test_path': 'abcdef',
            'name': 'tag_map'
        }
    )
    data = json.loads(response.body)

    self.assertEqual([_TEST_DIAGNOSTIC_DATA[4]], data)

  def testGetLastDiagnosticFails(self):
    self._AddMockData(_TEST_DIAGNOSTIC_DATA)

    self.testapp.post(
        '/get_diagnostics', {
            'end_revision': 'last',
            'test_path': 'foobar',
            'name': 'tag_map'
        }, status=400)

  def testGetMultipleDiagnosticsByTestPath(self):
    self._AddMockData(_TEST_DIAGNOSTIC_DATA)

    self.testapp.post('/get_diagnostics', {'test_path': 'foobar'}, status=400)

  def testGetDiagnostics_limit_500_diagnostics(self):
    self._AddMockData(self._GenerateData(501))

    response = self.testapp.post(
        '/get_diagnostics', {
            'test_path': 'foobar',
            'start_revision': 0,
            'end_revision': 1000
        })
    data = json.loads(response.body)
    self.assertTrue(len(data) == 500)

  def testGetDiagnostics_WrongGuid(self):
    self.testapp.post('/get_diagnostics', {'guid': 'foo'}, status=400)

  def testGetDiagnostics_MissingParam(self):
    self.testapp.post('/get_diagnostics', status=400)
