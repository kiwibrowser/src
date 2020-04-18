# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import copy
import httmock
import json
import mock
import os
import sys
import unittest

from apitools.base.py import encoding
from google.api.config import service_config
from google.api.control import messages
from oauth2client import client

class ServiceConfigFetchTest(unittest.TestCase):

  _ACCESS_TOKEN = "test_access_token"

  _SERVICE_NAME = "test_service_name"
  _SERVICE_VERSION = "test_service_version"
  _SERVICE_CONFIG_JSON = {
      "name": _SERVICE_NAME,
      "id": _SERVICE_VERSION
  }

  _credentials = mock.MagicMock()
  _get_http_client = mock.MagicMock()

  def setUp(self):
    os.environ["ENDPOINTS_SERVICE_NAME"] = ServiceConfigFetchTest._SERVICE_NAME
    os.environ["ENDPOINTS_SERVICE_VERSION"] = ServiceConfigFetchTest._SERVICE_VERSION

    self._set_up_default_credential()

  def test_no_service_name(self):
    del os.environ["ENDPOINTS_SERVICE_NAME"]
    message = 'The "ENDPOINTS_SERVICE_NAME" environment variable is not set'
    with self.assertRaisesRegexp(ValueError, message):
      service_config.fetch_service_config()

  def test_no_service_version(self):
    del os.environ["ENDPOINTS_SERVICE_VERSION"]
    message = 'The "ENDPOINTS_SERVICE_VERSION" environment variable is not set'
    with self.assertRaisesRegexp(ValueError, message):
      service_config.fetch_service_config()

  @mock.patch("google.api.config.service_config.client.GoogleCredentials",
              _credentials)
  @mock.patch("google.api.config.service_config._get_http_client", _get_http_client)
  def test_fetch_service_config(self):
    mock_response = mock.MagicMock()
    mock_response.status = 200
    mock_response.data = json.dumps(ServiceConfigFetchTest._SERVICE_CONFIG_JSON)
    mock_http_client = mock.MagicMock()
    mock_http_client.request.return_value = mock_response
    ServiceConfigFetchTest._get_http_client.return_value = mock_http_client

    service = encoding.JsonToMessage(messages.Service,
                                     json.dumps(self._SERVICE_CONFIG_JSON))
    self.assertEqual(service, service_config.fetch_service_config())

    template = service_config._SERVICE_MGMT_URL_TEMPLATE
    url = template.format(ServiceConfigFetchTest._SERVICE_NAME,
                          ServiceConfigFetchTest._SERVICE_VERSION)
    headers={"Authorization": "Bearer " + ServiceConfigFetchTest._ACCESS_TOKEN}
    mock_http_client.request.assert_called_once_with("GET", url,
                                                     headers=headers)

  @mock.patch("google.api.config.service_config.client.GoogleCredentials",
              _credentials)
  @mock.patch("google.api.config.service_config._get_http_client", _get_http_client)
  def test_fetch_service_config_failed(self):
    mock_response = mock.MagicMock()
    mock_response.status = 403
    mock_http_client = mock.MagicMock()
    mock_http_client.request.return_value = mock_response
    ServiceConfigFetchTest._get_http_client.return_value = mock_http_client
    with self.assertRaisesRegexp(Exception, "status code 403"):
      service_config.fetch_service_config()

  @mock.patch("google.api.config.service_config.client.GoogleCredentials",
              _credentials)
  @mock.patch("google.api.config.service_config._get_http_client", _get_http_client)
  def test_fetch_service_config_with_wrong_service_name(self):
    mock_response = mock.MagicMock()
    mock_response.status = 200
    config = copy.deepcopy(ServiceConfigFetchTest._SERVICE_CONFIG_JSON)
    config["name"] = "incorrect_service_name"
    mock_response.data = json.dumps(config)
    mock_http_client = mock.MagicMock()
    mock_http_client.request.return_value = mock_response
    ServiceConfigFetchTest._get_http_client.return_value = mock_http_client

    message = ("Unexpected service name in service config: " +
               config["name"])
    with self.assertRaisesRegexp(ValueError, message):
      service_config.fetch_service_config()

  @mock.patch("google.api.config.service_config.client.GoogleCredentials",
              _credentials)
  @mock.patch("google.api.config.service_config._get_http_client", _get_http_client)
  def test_fetch_service_config_with_wrong_service_version(self):
    mock_response = mock.MagicMock()
    mock_response.status = 200
    config = copy.deepcopy(ServiceConfigFetchTest._SERVICE_CONFIG_JSON)
    config["id"] = "incorrect_service_version"
    mock_response.data = json.dumps(config)
    mock_http_client = mock.MagicMock()
    mock_http_client.request.return_value = mock_response
    ServiceConfigFetchTest._get_http_client.return_value = mock_http_client

    message = ("Unexpected service version in service config: " +
               config["id"])
    with self.assertRaisesRegexp(ValueError, message):
      service_config.fetch_service_config()

  def _set_up_default_credential(self):
    default_credential = mock.MagicMock()
    ServiceConfigFetchTest._credentials.get_application_default.return_value \
        = default_credential
    default_credential.create_scoped.return_value = default_credential
    token = ServiceConfigFetchTest._ACCESS_TOKEN
    access_token = client.AccessTokenInfo(access_token=token, expires_in=None)
    default_credential.get_access_token.return_value = access_token
