# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import unittest
import urlparse

import mock

from dashboard.common import datastore_hooks
from dashboard.common import utils
from dashboard.services import pinpoint_service


class PinpointServiceTest(unittest.TestCase):

  def setUp(self):
    super(PinpointServiceTest, self).setUp()

    patcher = mock.patch.object(datastore_hooks, 'IsUnalteredQueryPermitted')
    self.addCleanup(patcher.stop)
    self.mock_hooks = patcher.start()

    patcher = mock.patch.object(utils, 'ServiceAccountHttp')
    self.addCleanup(patcher.stop)
    self.mock_request = mock.MagicMock()
    self.mock_http = patcher.start()
    self.mock_http.return_value = self.mock_request

  def testNewJob(self):
    self.mock_request.request.return_value = (None, json.dumps({}))
    self.mock_hooks.return_value = True

    pinpoint_service.NewJob({})

    self.mock_request.request.assert_called_with(
        pinpoint_service._PINPOINT_URL + '/api/new',
        body=mock.ANY, method='POST', headers={'Content-length': 0})

  def testRequest_Unprivileged_Asserts(self):
    self.mock_hooks.return_value = False

    with self.assertRaises(AssertionError):
      pinpoint_service.NewJob({})

  def testRequest_EncodesParams(self):
    self.mock_request.request.return_value = (None, json.dumps({'foo': 'bar'}))
    self.mock_hooks.return_value = True

    class UrlParamsMatcher(object):
      def __init__(self, params):
        self._params = params

      def __eq__(self, rhs):
        rhs_params = dict(urlparse.parse_qsl(rhs))
        return self._params == rhs_params

      def __repr__(self):
        return str(self._params)

    params = {'a': 'b', 'c': 'd'}

    pinpoint_service.NewJob(params)

    expected_args = mock.call(
        mock.ANY,
        body=UrlParamsMatcher(params),
        method='POST',
        headers={'Content-length': 0})
    self.assertEqual([expected_args], self.mock_request.request.call_args_list)
