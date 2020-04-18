# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import httplib2
import json
import logging
import oauth2client.client
import oauth2client.file
from oauth2client import service_account  # pylint: disable=no-name-in-module
import oauth2client.tools
import os
import urllib

from py_utils import retry_util  # pylint: disable=import-error


class RequestError(OSError):
  """Exception class for errors while making a request."""
  def __init__(self, response, content):
    self.response = response
    self.content = content
    try:
      # Try to find error message within returned content.
      message = json.loads(content)['error']
    except StandardError:
      # Otherwise use the entire content itself.
      message = content
    super(RequestError, self).__init__(
        'Request returned HTTP Error %s: %s' % (response['status'], message))


class ClientError(RequestError):
  """Exception for 4xx HTTP client errors."""
  pass

class ServerError(RequestError):
  """Exception for 5xx HTTP server errors."""
  pass


def BuildRequestError(response, content):
  """Build the correct RequestError depending on the response status."""
  if response['status'].startswith('4'):
    return ClientError(response, content)
  elif response['status'].startswith('5'):
    return ServerError(response, content)
  else:
    # Fall back to the base class.
    return RequestError(response, content)


class PerfDashboardCommunicator(object):

  REQUEST_URL = 'https://chromeperf.appspot.com/api/'
  OAUTH_CLIENT_ID = (
      '62121018386-h08uiaftreu4dr3c4alh3l7mogskvb7i.apps.googleusercontent.com')
  OAUTH_CLIENT_SECRET = 'vc1fZfV1cZC6mgDSHV-KSPOz'
  SCOPES = ['https://www.googleapis.com/auth/userinfo.email']

  def __init__(self, flags):
    self._credentials = None
    if flags.service_account_json:
      self._AuthorizeAccountServiceAccount(flags.service_account_json)
    else:
      self._AuthorizeAccountUserAccount(flags)

  @property
  def has_credentials(self):
    return self._credentials and not self._credentials.invalid

  def _AuthorizeAccountServiceAccount(self, json_keyfile):
    """Used to create service account credentials for the dashboard.

    Args:
      json_keyfile: Path to json file that contains credentials.
    """
    self._credentials = (
        service_account.ServiceAccountCredentials.from_json_keyfile_name(
            json_keyfile, self.SCOPES))

  def _AuthorizeAccountUserAccount(self, flags):
    """Used to create user account credentials for the performance dashboard.

    Args:
      flags: An argparse.Namespace as returned by argparser.parse_args; in
        addition to oauth2client.tools.argparser flags should also have set a
        user_credentials_json flag.
    """
    store = oauth2client.file.Storage(flags.user_credentials_json)
    if os.path.exists(flags.user_credentials_json):
      self._credentials = store.locked_get()
    if not self.has_credentials:
      flow = oauth2client.client.OAuth2WebServerFlow(
          self.OAUTH_CLIENT_ID, self.OAUTH_CLIENT_SECRET, self.SCOPES,
          access_type='offline', include_granted_scopes='true',
          prompt='consent')
      self._credentials = oauth2client.tools.run_flow(flow, store, flags)

  @retry_util.RetryOnException(ServerError, retries=3)
  def _MakeApiRequest(self, request, retries=None):
    """Used to communicate with perf dashboard.

    Args:
      request: String that contains POST request to dashboard.
    Returns:
      Contents of the response from the dashboard.
    """
    del retries  # Handled by the decorator.
    assert self.has_credentials
    http = httplib2.Http()
    if self._credentials.access_token_expired:
      self._credentials.refresh(http)
    http = self._credentials.authorize(http)
    logging.info('Making API request: %s', request)
    resp, content = http.request(
        self.REQUEST_URL + request,
        method="POST",
        headers={'Content-length': 0})
    if resp['status'] != '200':
      raise BuildRequestError(resp, content)
    return json.loads(content)

  def ListTestPaths(self, benchmark, sheriff):
    """Lists test paths for the given benchmark.

    args:
      benchmark: Benchmark to get paths for.
      sheriff:
          Filters test paths to only ones monitored by the given sheriff
          rotation.
    returns:
      A list of test paths. Ex. ['TestPath1', 'TestPath2']
    """
    options = urllib.urlencode({'sheriff': sheriff})
    return self._MakeApiRequest('list_timeseries/%s?%s' % (benchmark, options))

  def GetTimeseries(self, test_path, days=30):
    """Get timeseries for the given test path.

    args:
      test_path: test path to get timeseries for.
      days: Number of days to get data points for.
    returns:
      A dict in the format:
      {'revision_logs':{
          r_commit_pos: {... data ...},
          r_chromium_rev: {... data ...},
          ...},
       'timeseries': [
           [revision, value, timestamp, r_commit_pos, r_webkit_rev],
           ...
           ],
       'test_path': test_path}
    """
    options = urllib.urlencode({'num_days': days})
    r = 'timeseries/%s?%s' % (urllib.quote(test_path), options)
    return self._MakeApiRequest(r)

  def GetBugData(self, bug_ids):
    """Yields data for a given bug id or sequence of bug ids."""
    if not hasattr(bug_ids, '__iter__'):
      bug_ids = [bug_ids]
    for bug_id in bug_ids:
      yield self._MakeApiRequest('bugs/%d' % bug_id)

  def GetAlertData(self, benchmark, sheriff, days=30):
    """Returns alerts for the given benchmark."""
    options = urllib.urlencode({'benchmark': benchmark, 'sheriff': sheriff})
    return self._MakeApiRequest('alerts/history/%d?%s' % (days, options))
