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

"""Provides a method for fetching Service Configuration from Google Service
Management API."""


import logging
import json
import os
import urllib3

from apitools.base.py import encoding
import google.api.gen.servicecontrol_v1_messages as messages
from oauth2client import client
from urllib3.contrib import appengine


logger = logging.getLogger(__name__)

_GOOGLE_API_SCOPE = "https://www.googleapis.com/auth/cloud-platform"

_SERVICE_MGMT_URL_TEMPLATE = ("https://servicemanagement.googleapis.com"
                              "/v1/services/{}/configs/{}")

_SERVICE_NAME_ENV_KEY = "ENDPOINTS_SERVICE_NAME"
_SERVICE_VERSION_ENV_KEY = "ENDPOINTS_SERVICE_VERSION"


def fetch_service_config(service_name=None, service_version=None):
  """Fetches the service config from Google Serivce Management API.

  Args:
    service_name: the service name. When this argument is unspecified, this
      method uses the value of the "SERVICE_NAME" environment variable as the
      service name, and raises ValueError if the environment variable is unset.
    service_version: the service version. When this argument is unspecified,
      this method uses the value of the "SERVICE_VERSION" environment variable
      as the service version, and raises ValueError if the environment variable
      is unset.

  Returns: the fetched service config JSON object.

  Raises:
    ValueError: when the service name/version is neither provided as an
      argument or set as an environment variable; or when the fetched service
      config fails validation.
    Exception: when the Google Service Management API returns non-200 response.
  """
  if not service_name:
    service_name = _get_env_var_or_raise(_SERVICE_NAME_ENV_KEY)
  if not service_version:
    service_version = _get_env_var_or_raise(_SERVICE_VERSION_ENV_KEY)

  service_mgmt_url = _SERVICE_MGMT_URL_TEMPLATE.format(service_name,
                                                       service_version)

  access_token = _get_access_token()
  headers = {"Authorization": "Bearer {}".format(access_token)}

  http_client = _get_http_client()
  response = http_client.request("GET", service_mgmt_url, headers=headers)

  status_code = response.status
  if status_code != 200:
    message_template = "Fetching service config failed (status code {})"
    _log_and_raise(Exception, message_template.format(status_code))

  logger.debug('obtained service json from the management api:\n%s', response.data)
  service = encoding.JsonToMessage(messages.Service, response.data)
  _validate_service_config(service, service_name, service_version)
  return service


def _get_access_token():
  credentials = client.GoogleCredentials.get_application_default()
  if credentials.create_scoped_required():
    credentials = credentials.create_scoped(_GOOGLE_API_SCOPE)
  return credentials.get_access_token().access_token


def _get_http_client():
  if appengine.is_appengine_sandbox():
    return appengine.AppEngineManager()
  else:
    return urllib3.PoolManager()


def _get_env_var_or_raise(env_variable_name):
  if env_variable_name not in os.environ:
    message_template = 'The "{}" environment variable is not set'
    _log_and_raise(ValueError, message_template.format(env_variable_name))
  return os.environ[env_variable_name]


def _validate_service_config(service, expected_service_name,
                             expected_service_version):
  service_name = service.name
  if not service_name:
    _log_and_raise(ValueError, "No service name in the service config")
  if service_name != expected_service_name:
    message_template = "Unexpected service name in service config: {}"
    _log_and_raise(ValueError, message_template.format(service_name))

  service_version = service.id
  if not service_version:
    _log_and_raise(ValueError, "No service version in the service config")
  if service_version != expected_service_version:
    message_template = "Unexpected service version in service config: {}"
    _log_and_raise(ValueError, message_template.format(service_version))


def _log_and_raise(exception_class, message):
  logger.error(message)
  raise exception_class(message)
