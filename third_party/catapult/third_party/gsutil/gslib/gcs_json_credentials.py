# -*- coding: utf-8 -*-
# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Credentials logic for JSON CloudApi implementation."""

# This module duplicates some logic in third_party gcs_oauth2_boto_plugin
# because apitools credentials lib has its own mechanisms for file-locking
# and credential storage.  As such, it doesn't require most of the
# gcs_oauth2_boto_plugin logic.


import json
import logging
import os
import traceback

from apitools.base.py import credentials_lib
from apitools.base.py import exceptions as apitools_exceptions
from boto import config
from gslib.cred_types import CredTypes
from gslib.exception import CommandException
from gslib.util import GetBotoConfigFileList
from gslib.util import GetGceCredentialCacheFilename
from gslib.util import GetGsutilClientIdAndSecret
import oauth2client
from oauth2client.client import HAS_CRYPTO
from oauth2client.contrib import devshell
from oauth2client.service_account import ServiceAccountCredentials

from six import BytesIO


DEFAULT_GOOGLE_OAUTH2_PROVIDER_AUTHORIZATION_URI = (
    'https://accounts.google.com/o/oauth2/auth')
DEFAULT_GOOGLE_OAUTH2_PROVIDER_TOKEN_URI = (
    'https://accounts.google.com/o/oauth2/token')

DEFAULT_SCOPES = [
    u'https://www.googleapis.com/auth/cloud-platform',
    u'https://www.googleapis.com/auth/cloud-platform.read-only',
    u'https://www.googleapis.com/auth/devstorage.full_control',
    u'https://www.googleapis.com/auth/devstorage.read_only',
    u'https://www.googleapis.com/auth/devstorage.read_write'
]

GOOGLE_OAUTH2_DEFAULT_FILE_PASSWORD = 'notasecret'


def CheckAndGetCredentials(logger):
  """Returns credentials from the configuration file, if any are present.

  Args:
    logger: logging.Logger instance for outputting messages.

  Returns:
    OAuth2Credentials object if any valid ones are found, otherwise None.
  """
  configured_cred_types = []
  try:
    if _HasOauth2UserAccountCreds():
      configured_cred_types.append(CredTypes.OAUTH2_USER_ACCOUNT)
    if _HasOauth2ServiceAccountCreds():
      configured_cred_types.append(CredTypes.OAUTH2_SERVICE_ACCOUNT)
    if len(configured_cred_types) > 1:
      # We only allow one set of configured credentials. Otherwise, we're
      # choosing one arbitrarily, which can be very confusing to the user
      # (e.g., if only one is authorized to perform some action) and can
      # also mask errors.
      # Because boto merges config files, GCE credentials show up by default
      # for GCE VMs. We don't want to fail when a user creates a boto file
      # with their own credentials, so in this case we'll use the OAuth2
      # user credentials.
      failed_cred_type = None
      raise CommandException(
          ('You have multiple types of configured credentials (%s), which is '
           'not supported. One common way this happens is if you run gsutil '
           'config to create credentials and later run gcloud auth, and '
           'create a second set of credentials. Your boto config path is: '
           '%s. For more help, see "gsutil help creds".')
          % (configured_cred_types, GetBotoConfigFileList()))

    failed_cred_type = CredTypes.OAUTH2_USER_ACCOUNT
    user_creds = _GetOauth2UserAccountCredentials()
    failed_cred_type = CredTypes.OAUTH2_SERVICE_ACCOUNT
    service_account_creds = _GetOauth2ServiceAccountCredentials()
    failed_cred_type = CredTypes.GCE
    gce_creds = _GetGceCreds()
    failed_cred_type = CredTypes.DEVSHELL
    devshell_creds = _GetDevshellCreds()
    return user_creds or service_account_creds or gce_creds or devshell_creds
  except:  # pylint: disable=bare-except
    # If we didn't actually try to authenticate because there were multiple
    # types of configured credentials, don't emit this warning.
    if failed_cred_type:
      if logger.isEnabledFor(logging.DEBUG):
        logger.debug(traceback.format_exc())
      if os.environ.get('CLOUDSDK_WRAPPER') == '1':
        logger.warn(
            'Your "%s" credentials are invalid. Please run\n'
            '  $ gcloud auth login', failed_cred_type)
      else:
        logger.warn(
            'Your "%s" credentials are invalid. For more help, see '
            '"gsutil help creds", or re-run the gsutil config command (see '
            '"gsutil help config").', failed_cred_type)

    # If there's any set of configured credentials, we'll fail if they're
    # invalid, rather than silently falling back to anonymous config (as
    # boto does). That approach leads to much confusion if users don't
    # realize their credentials are invalid.
    raise


def GetCredentialStoreKey(credentials, api_version):
  """Disambiguates a credential for caching in a credential store.

  Different credential types have different fields that identify them.
  This function assembles relevant information in a string to be used as the key
  for accessing a credential.

  Args:
    credentials: An OAuth2Credentials object.
    api_version: JSON API version being used.

  Returns:
    A string that can be used as the key to identify a credential, e.g.
    "v1-909320924072.apps.googleusercontent.com-1/rEfrEshtOkEn"
  """
  # TODO: If scopes ever become available in the credentials themselves,
  # include them in the key.
  key_parts = [api_version]
  # pylint: disable=protected-access
  if isinstance(credentials, devshell.DevshellCredentials):
    key_parts.append(credentials.user_email)
  elif isinstance(credentials, ServiceAccountCredentials):
    key_parts.append(credentials._service_account_email)
  elif isinstance(credentials, oauth2client.client.OAuth2Credentials):
    if credentials.client_id and credentials.client_id != 'null':
      key_parts.append(credentials.client_id)
    else:
      key_parts.append('noclientid')
    key_parts.append(credentials.refresh_token or 'norefreshtoken')
  # pylint: enable=protected-access

  return '-'.join(key_parts)


def _GetProviderTokenUri():
  return config.get(
      'OAuth2', 'provider_token_uri', DEFAULT_GOOGLE_OAUTH2_PROVIDER_TOKEN_URI)


def _HasOauth2ServiceAccountCreds():
  return config.has_option('Credentials', 'gs_service_key_file')


def _HasOauth2UserAccountCreds():
  return config.has_option('Credentials', 'gs_oauth2_refresh_token')


def _HasGceCreds():
  return config.has_option('GoogleCompute', 'service_account')


def _GetOauth2ServiceAccountCredentials():
  """Retrieves OAuth2 service account credentials for a private key file."""
  if not _HasOauth2ServiceAccountCreds():
    return

  provider_token_uri = _GetProviderTokenUri()
  service_client_id = config.get('Credentials', 'gs_service_client_id', '')
  private_key_filename = config.get('Credentials', 'gs_service_key_file', '')
  private_key = None
  with open(private_key_filename, 'rb') as private_key_file:
    private_key = private_key_file.read()

  json_key_dict = None
  try:
    json_key_dict = json.loads(private_key)
  except ValueError:
    pass
  if json_key_dict:
    # Key file is in JSON format.
    for json_entry in ('client_id', 'client_email', 'private_key_id',
                       'private_key'):
      if json_entry not in json_key_dict:
        raise Exception('The JSON private key file at %s '
                        'did not contain the required entry: %s' %
                        (private_key_filename, json_entry))
    return ServiceAccountCredentials.from_json_keyfile_dict(
        json_key_dict, scopes=DEFAULT_SCOPES, token_uri=provider_token_uri)
  else:
    # Key file is in P12 format.
    if HAS_CRYPTO:
      if not service_client_id:
        raise Exception('gs_service_client_id must be set if '
                        'gs_service_key_file is set to a .p12 key file')
      key_file_pass = config.get(
          'Credentials', 'gs_service_key_file_password',
          GOOGLE_OAUTH2_DEFAULT_FILE_PASSWORD)
      # We use _from_p12_keyfile_contents to avoid reading the key file
      # again unnecessarily.
      return ServiceAccountCredentials.from_p12_keyfile_buffer(
          service_client_id, BytesIO(private_key),
          private_key_password=key_file_pass, scopes=DEFAULT_SCOPES,
          token_uri=provider_token_uri)


def _GetOauth2UserAccountCredentials():
  """Retrieves OAuth2 service account credentials for a refresh token."""
  if not _HasOauth2UserAccountCreds():
    return

  provider_token_uri = _GetProviderTokenUri()
  gsutil_client_id, gsutil_client_secret = GetGsutilClientIdAndSecret()
  client_id = config.get('OAuth2', 'client_id',
                         os.environ.get('OAUTH2_CLIENT_ID', gsutil_client_id))
  client_secret = config.get('OAuth2', 'client_secret',
                             os.environ.get('OAUTH2_CLIENT_SECRET',
                                            gsutil_client_secret))
  return oauth2client.client.OAuth2Credentials(
      None, client_id, client_secret,
      config.get('Credentials', 'gs_oauth2_refresh_token'), None,
      provider_token_uri, None)


def _GetGceCreds():
  if not _HasGceCreds():
    return

  try:
    return credentials_lib.GceAssertionCredentials(
        service_account_name=config.get(
            'GoogleCompute', 'service_account', 'default'),
        cache_filename=GetGceCredentialCacheFilename())
  except apitools_exceptions.ResourceUnavailableError, e:
    if 'service account' in str(e) and 'does not exist' in str(e):
      return None
    raise


def _GetDevshellCreds():
  try:
    return devshell.DevshellCredentials()
  except devshell.NoDevshellServer:
    return None
  except:
    raise
