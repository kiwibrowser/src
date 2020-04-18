# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for milo."""

from __future__ import print_function

import json
import mock
import os

from chromite.lib import auth
from chromite.lib import cros_test_lib
from chromite.lib import milo


TESTDATA_PATH = os.path.join(os.path.dirname(__file__), 'testdata', 'milo')


class MiloClientTest(cros_test_lib.MockTestCase):
  """Tests for MiloClient."""
  def setUp(self):
    self.mock_request = self.PatchObject(milo.MiloClient, 'SendRequest')
    self.PatchObject(milo.MiloClient, '_GetHost', return_value='foo.com')
    self.PatchObject(auth, 'GetAccessToken', return_value='token')
    self.client = milo.MiloClient()

  def testGetBuildbotBuildJSON(self):
    """Test GetBuildbotBuildJSON."""
    # Datagram corresponding to: chromeos elm-paladin 1535
    with open(os.path.join(
        TESTDATA_PATH, 'MiloClientTest.testGetBuildbotBuildJSON.data')) as f:
      datagram = f.read()
    self.mock_request.return_value = {
        'data': datagram,
    }
    resp = self.client.GetBuildbotBuildJSON('chromeos', 'elm-paladin', 1535)
    self.assertEqual(resp['slave'], 'build115-m2')
    self.assertEqual(resp['properties']['buildername'], 'elm-paladin')
    self.assertEqual(resp['steps']['BuildPackages']['logs'][0][1],
                     ('https://uberchromegw.corp.google.com/i/chromeos/'
                      'builders/elm-paladin/builds/1535/steps/BuildPackages/'
                      'logs/stdio'))
    self.assertEqual(resp['masterName'], 'chromeos')
    self.mock_request.assert_called_with('prpc/milo.Buildbot',
                                         'GetBuildbotBuildJSON', mock.ANY,
                                         dryrun=False)

  def testBuildInfoGetBuildbot(self):
    """Test BuildInfoGetBuildbot."""
    # Test data file is generated via:
    # rpc call -format json luci-milo.appspot.com milo.BuildInfo.Get <<EOD
    # {
    #   "buildbot": {
    #     "masterName": "tryserver.chromium.perf",
    #     "builderName": "win_perf_bisect",
    #     "buildNumber": 7170
    #   }
    # }
    # EOD
    with open(os.path.join(
        TESTDATA_PATH, 'MiloClientTest.testBuildInfoGetBuildbot.json')) as f:
      self.mock_request.return_value = json.load(f)
    resp = self.client.BuildInfoGetBuildbot('tryserver.chromium.perf',
                                            'win_perf_bisect', 7170)
    # Look up based directly on protobufs data.
    self.assertEqual(resp['step']['status'], 'SUCCESS')
    self.assertEqual(resp['annotationStream']['prefix'],
                     'bb/tryserver.chromium.perf/win_perf_bisect/7170')
    self.assertEqual(resp['steps'].keys()[0:6],
                     [(None,), None,
                      ('setup_build',), 'setup_build',
                      ('taskkill',), 'taskkill'])
    # Root (level 0) step.
    self.assertEqual(resp['steps'][None]['status'], 'SUCCESS')
    # Level 1 step as scalar.
    self.assertEqual(resp['steps']['setup_build']['stdoutStream']['name'],
                     'recipes/steps/setup_build/0/stdout')
    # Level 1 step as full tuple.
    self.assertEqual(resp['steps'][('setup_build',)]['stdoutStream']['name'],
                     'recipes/steps/setup_build/0/stdout')
    # Level 1 non-leaf step.
    self.assertEqual(resp['steps'][('ensure_goma',)]['started'],
                     '2017-03-02T22:44:41.725726800Z')
    # Level 2 leaf step..
    self.assertEqual(resp['steps']
                     [('ensure_goma', 'ensure_goma.ensure_installed')]
                     ['stdoutStream']['name'],
                     'recipes/steps/ensure_goma/0/steps/ensure_installed/'
                     '0/stdout')
    self.mock_request.assert_called_with('prpc/milo.BuildInfo', 'Get', mock.ANY,
                                         dryrun=False)
