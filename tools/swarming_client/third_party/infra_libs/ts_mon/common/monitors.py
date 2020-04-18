# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Classes representing the monitoring interface for tasks or devices."""


import base64
import httplib2
import json
import logging
import socket
import traceback

from googleapiclient import discovery
from googleapiclient import errors
from infra_libs import httplib2_utils
from infra_libs.ts_mon.common import interface
from infra_libs.ts_mon.common import http_metrics
from infra_libs.ts_mon.common import pb_to_popo
from infra_libs.ts_mon.protos import metrics_pb2
try: # pragma: no cover
  from oauth2client import gce
except ImportError: # pragma: no cover
  from oauth2client.contrib import gce
from oauth2client.client import GoogleCredentials
from oauth2client.file import Storage

# Special string that can be passed through as the credentials path to use the
# default Appengine or GCE service account.
APPENGINE_CREDENTIALS = ':appengine'
GCE_CREDENTIALS = ':gce'


class CredentialFactory(object):
  """Base class for things that can create OAuth2Credentials."""

  @classmethod
  def from_string(cls, path):
    """Creates an appropriate subclass from a file path or magic string."""

    if path == APPENGINE_CREDENTIALS:
      return AppengineCredentials()
    if path == GCE_CREDENTIALS:
      return GCECredentials()
    return FileCredentials(path)

  def create(self, scopes):
    raise NotImplementedError


class GCECredentials(CredentialFactory):
  def create(self, scopes):
    return gce.AppAssertionCredentials(scopes)


class AppengineCredentials(CredentialFactory):
  def create(self, scopes):  # pragma: no cover
    # This import doesn't work outside appengine, so delay it until it's used.
    from oauth2client import appengine
    return appengine.AppAssertionCredentials(scopes)


class FileCredentials(CredentialFactory):
  def __init__(self, path):
    self.path = path

  def create(self, scopes):
    with open(self.path, 'r') as fh:
      data = json.load(fh)
    if data.get('type', None):
      credentials = GoogleCredentials.from_stream(self.path)
      credentials = credentials.create_scoped(scopes)
      return credentials
    return Storage(self.path).get()


class DelegateServiceAccountCredentials(CredentialFactory):
  IAM_SCOPE = 'https://www.googleapis.com/auth/iam'

  def __init__(self, service_account_email, base):
    self.base = base
    self.service_account_email = service_account_email

  def create(self, scopes):
    logging.info('Delegating to service account %s', self.service_account_email)
    http = httplib2_utils.InstrumentedHttp('actor-credentials')
    http = self.base.create([self.IAM_SCOPE]).authorize(http)
    return httplib2_utils.DelegateServiceAccountCredentials(
        http, self.service_account_email, scopes)


class Monitor(object):
  """Abstract base class encapsulating the ability to collect and send metrics.

  This is a singleton class. There should only be one instance of a Monitor at
  a time. It will be created and initialized by process_argparse_options. It
  must exist in order for any metrics to be sent, although both Targets and
  Metrics may be initialized before the underlying Monitor. If it does not exist
  at the time that a Metric is sent, an exception will be raised.

  send() can be either synchronous or asynchronous.  If synchronous, it needs to
  make the HTTP request, wait for a response and return None.
  If asynchronous, send() should start the request and immediately return some
  object which is later passed to wait() once all requests have been started.
  """

  _SCOPES = []

  def send(self, metric_pb):
    raise NotImplementedError()

  def wait(self, state):  # pragma: no cover
    pass


class HttpsMonitor(Monitor):

  _SCOPES = ['https://www.googleapis.com/auth/prodxmon']

  def __init__(self, endpoint, credential_factory, http=None, ca_certs=None):
    self._endpoint = endpoint
    credentials = credential_factory.create(self._SCOPES)
    if http is None:
      http = httplib2_utils.RetriableHttp(
          httplib2_utils.InstrumentedHttp('acq-mon-api', ca_certs=ca_certs))
    self._http = credentials.authorize(http)

  def encode_to_json(self, metric_pb):
    return json.dumps({'payload': pb_to_popo.convert(metric_pb)})

  def send(self, metric_pb):
    body = self.encode_to_json(metric_pb)

    try:
      resp, content = self._http.request(self._endpoint,
          method='POST',
          body=body,
          headers={'Content-Type': 'application/json'})
      if resp.status != 200:
        logging.warning('HttpsMonitor.send received status %d: %s', resp.status,
                        content)
    except (ValueError, errors.Error,
            socket.timeout, socket.error, socket.herror, socket.gaierror,
            httplib2.HttpLib2Error):
      logging.exception('HttpsMonitor.send failed')


class DebugMonitor(Monitor):
  """Class which writes metrics to logs or a local file for debugging."""
  def __init__(self, filepath=None):
    if filepath is None:
      self._fh = None
    else:
      self._fh = open(filepath, 'a')

  def send(self, metric_pb):
    text = str(metric_pb)
    logging.info('Flushing monitoring metrics:\n%s', text)
    if self._fh is not None:
      self._fh.write(text + '\n\n')
      self._fh.flush()


class NullMonitor(Monitor):
  """Class that doesn't send metrics anywhere."""
  def send(self, metric_pb):
    pass
