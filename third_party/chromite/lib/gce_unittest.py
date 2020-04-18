# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests the gce module."""

from __future__ import print_function

import os

from chromite.lib import cros_test_lib
from chromite.lib import gce
from chromite.lib import osutils
from googleapiclient.errors import HttpError
from googleapiclient.http import HttpMockSequence
from oauth2client.client import GoogleCredentials


class GceTest(cros_test_lib.MockTempDirTestCase):
  """Unit tests for the gce module."""

  _PROJECT = 'foo-project'
  _ZONE = 'foo-zone'

  def setUp(self):
    self.json_key_file = os.path.join(self.tempdir, 'service_account.json')
    osutils.Touch(self.json_key_file)
    for cmd in ('from_stream', 'create_scoped'):
      self.PatchObject(GoogleCredentials, cmd, autospec=True)
    self.PatchObject(gce.GceContext, 'GetZoneRegion', autospec=True,
                     return_value=self._ZONE)

  @cros_test_lib.NetworkTest()
  def testGetImage(self):
    """Tests that GetImage returns the correct image."""
    good_http = HttpMockSequence([
        ({'status': '200',}, '{"name": "foo-image"}',),
    ])

    # Assert that GetImage does not complain if an image is found.
    self.PatchObject(gce.GceContext, '_BuildRetriableRequest', autospec=True,
                     side_effect=self._MockOutBuildRetriableRequest(good_http))
    gce_context = gce.GceContext.ForServiceAccount(self._PROJECT, self._ZONE,
                                                   self.json_key_file)
    self.assertDictEqual(gce_context.GetImage('foo-image'),
                         dict(name='foo-image'))

  @cros_test_lib.NetworkTest()
  def testGetImageRaisesIfImageNotFound(self):
    """Tests that GetImage raies exception when image is not found."""
    bad_http = HttpMockSequence([
        ({'status': '404',}, 'Image not found.',),
    ])

    # Assert that GetImage raises if image is not found.
    self.PatchObject(gce.GceContext, '_BuildRetriableRequest', autospec=True,
                     side_effect=self._MockOutBuildRetriableRequest(bad_http))
    gce_context = gce.GceContext.ForServiceAccount(self._PROJECT, self._ZONE,
                                                   self.json_key_file)
    with self.assertRaises(gce.ResourceNotFoundError):
      gce_context.GetImage('not-exising-image')

  @cros_test_lib.NetworkTest()
  def testRetryOnServerErrorHttpRequest(self):
    """Tests that 500 erros are retried."""

    # Fake http sequence that does not return 200 until the third trial.
    mock_http = HttpMockSequence([
        ({'status': '502'}, 'Server error'),
        ({'status': '502'}, 'Server error'),
        ({'status': '200'}, '{"name":"foo-image"}'),
    ])

    self.PatchObject(gce.GceContext, '_BuildRetriableRequest', autospec=True,
                     side_effect=self._MockOutBuildRetriableRequest(mock_http))

    # Disable retry and expect the request to fail.
    gce.GceContext.RETRIES = 0
    gce_context = gce.GceContext.ForServiceAccount(self._PROJECT, self._ZONE,
                                                   self.json_key_file)
    with self.assertRaises(HttpError):
      gce_context.GetImage('foo-image')

    # Enable retry and expect the request to succeed.
    gce.GceContext.RETRIES = 2
    gce_context = gce.GceContext.ForServiceAccount(self._PROJECT, self._ZONE,
                                                   self.json_key_file)
    self.assertDictEqual(gce_context.GetImage('foo-image'),
                         dict(name='foo-image'))

  def _MockOutBuildRetriableRequest(self, mock_http):
    """Returns a mock closure of _BuildRetriableRequest.

    Fake a GceContext._BuildRetriableRequest() that always uses |mock_http| as
    transport.
    """
    def _BuildRetriableRequest(_self, num_retries, _http, _thread_safe,
                               _credentials, *args, **kwargs):
      return gce.RetryOnServerErrorHttpRequest(num_retries, mock_http, *args,
                                               **kwargs)
    return _BuildRetriableRequest
