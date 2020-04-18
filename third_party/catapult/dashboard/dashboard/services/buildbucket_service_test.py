# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import unittest

import mock

from dashboard.services import buildbucket_service


_BUILD_PARAMETERS = {
    'builder_name': 'dummy_builder',
    'properties': {'bisect_config': {}}
}


class BuildbucketServiceTest(unittest.TestCase):

  def setUp(self):
    patcher = mock.patch('dashboard.services.request.RequestJson')
    self._request_json = patcher.start()
    self.addCleanup(patcher.stop)

    self._request_json.return_value = {'build': {'id': 'build id'}}

  def _AssertCorrectResponse(self, content):
    self.assertEqual(content, {'build': {'id': 'build id'}})

  def _AssertRequestMadeOnce(self, path, *args, **kwargs):
    self._request_json.assert_called_once_with(
        buildbucket_service.API_BASE_URL + path, *args, **kwargs)

  def testPut(self):
    expected_body = {
        'bucket': 'bucket_name',
        'parameters_json': json.dumps(_BUILD_PARAMETERS, separators=(',', ':')),
    }
    response = buildbucket_service.Put('bucket_name', _BUILD_PARAMETERS)
    self._AssertCorrectResponse(response)
    self._AssertRequestMadeOnce('builds', method='PUT', body=expected_body)

  def testPutJob(self):
    expected_body = {
        'bucket': buildbucket_service._BUCKET_NAME,
        'parameters_json': json.dumps(_BUILD_PARAMETERS, separators=(',', ':')),
    }
    self.assertEqual(buildbucket_service.PutJob(FakeJob()), 'build id')
    self._AssertRequestMadeOnce('builds', method='PUT', body=expected_body)

  def testGetJobStatus(self):
    response = buildbucket_service.GetJobStatus('job_id')
    self._AssertCorrectResponse(response)
    self._AssertRequestMadeOnce('builds/job_id')


class FakeJob(object):

  def GetBuildParameters(self):
    return _BUILD_PARAMETERS


if __name__ == '__main__':
  unittest.main()
