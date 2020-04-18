# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import httplib2
import json
from oauth2client import service_account  # pylint: disable=no-name-in-module
import os
import sys


DEFAULT_SCOPES = ['https://www.googleapis.com/auth/userinfo.email']
MILO_ENDPOINT = 'https://luci-milo.appspot.com/prpc/milo.Buildbot/'
LOGDOG_ENDPOINT = 'https://luci-logdog.appspot.com/prpc/logdog.Logs/'


class _MiloLogdogConfig(object):
  """Config class used to hold credentials shared by all API resquests."""
  credentials = None


class RequestError(Exception):
  pass


def IsOkStatus(status):
  return 200 <= int(status) <= 299


def SetCredentials(json_keyfile, scopes=None):
  """Configure the credentials used to access the milo/logdog API."""
  filepath = os.path.expanduser(json_keyfile)
  if not os.path.isfile(filepath):
    sys.stderr.write('Credentials not found: %s\n' % json_keyfile)
    sys.stderr.write('You need a json keyfile for a service account with '
                     'milo/logdog access.\n')
    sys.exit(1)

  if scopes is None:
    scopes = DEFAULT_SCOPES

  _MiloLogdogConfig.credentials = (
      service_account.ServiceAccountCredentials.from_json_keyfile_name(
          filepath, scopes))


def _Request(url, params):
  if _MiloLogdogConfig.credentials is None:
    # Try to use some default credentials if they haven't been explicitly set.
    SetCredentials('~/.default_service_credentials.json')

  http = _MiloLogdogConfig.credentials.authorize(httplib2.Http())
  body = json.dumps(params).encode('utf-8')
  response, content = http.request(url, 'POST', body, headers={
      'Accept': 'application/json', 'Content-Type': 'application/json'})
  if not IsOkStatus(response['status']):
    raise RequestError('Server returned %s response' % response['status'])

  # Need to skip over the 4 characters jsonp header.
  return json.loads(content[4:].decode('utf-8'))


def MiloRequest(method, params):
  return _Request(MILO_ENDPOINT + method, params)


def LogdogRequest(method, params):
  return _Request(LOGDOG_ENDPOINT + method, params)
