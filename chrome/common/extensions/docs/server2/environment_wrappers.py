# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import os

from environment import IsAppEngine, IsComputeEngine, IsTest


_METADATA_SERVER = 'http://metadata/computeMetadata/v1/instance/service-accounts'
_SERVICE_ACCOUNT = 'default'

# The environment variable to use as a fallback token source.
_ACCESS_TOKEN_ENV = 'DOCSERVER_ACCESS_TOKEN'


def CreateUrlFetcher(base_path=None):
  if IsAppEngine():
    from url_fetcher_appengine import UrlFetcherAppengine
    fetcher = UrlFetcherAppengine()
  elif not IsTest():
    from url_fetcher_urllib2 import UrlFetcherUrllib2
    fetcher = UrlFetcherUrllib2()
  else:
    from url_fetcher_fake import UrlFetcherFake
    fetcher = UrlFetcherFake()
  fetcher.SetBasePath(base_path)
  return fetcher


def CreatePersistentObjectStore(namespace):
  if IsAppEngine():
    from persistent_object_store_appengine import PersistentObjectStoreAppengine
    object_store = PersistentObjectStoreAppengine(namespace)
  else:
    from persistent_object_store_fake import PersistentObjectStoreFake
    object_store = PersistentObjectStoreFake(namespace)
  return object_store


def GetAccessToken():
  '''Acquires an access token either from the metadata service (if running on
  a real CE VM) or the DOCSERVER_ACCESS_TOKEN environment variable.

  This may return None if no token is available. That should never happen while
  running on a VM.
  '''
  # Attempt to grab an access token from the VM instance's metadata.
  if IsComputeEngine():
    fetcher = CreateUrlFetcher()
    token_uri = '%s/%s/token' % (_METADATA_SERVER, _SERVICE_ACCOUNT)
    token_response = None
    try:
      token_response = fetcher.Fetch(token_uri,
          headers={'Metadata-Flavor': 'Google'})
      if token_response.status_code == 200:
        return json.loads(token_response.content)['access_token']
    except Exception as e:
      logging.warn('Failed to fetch access token from VM metadata.')
      pass

  logging.warn('Using access token from local environment.')
  access_token = os.getenv(_ACCESS_TOKEN_ENV)
  if access_token is None:
    logging.error('No access token available. '
                  'Please set %s if you want things to work.' %
                  _ACCESS_TOKEN_ENV)
  return access_token
