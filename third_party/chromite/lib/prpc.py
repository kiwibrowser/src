# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Helpers for interacting with pRPC service."""

from __future__ import print_function

import httplib
import httplib2
import json
import socket

from chromite.lib import auth
from chromite.lib import cros_logging as logging
from chromite.lib import retry_util


# Methods
POST_METHOD = 'POST'


class Enum(tuple):
  """Helper class to build enumerations.

  Args:
    tuple: enumeration values.
  """
  __getattr__ = tuple.index


# pRPC/gRPC response codes
# As defined in https://godoc.org/google.golang.org/grpc/codes
PRPCCode = Enum([
    'OK',
    'Canceled',
    'Unknown',
    'InvalidArgument',
    'DeadlineExceeded',
    'NotFound',
    'AlreadyExists',
    'PermissionDenied',
    'ResourceExhausted',
    'FailedPrecondition',
    'Aborted',
    'OutOfRange',
    'Unimplemented',
    'Internal',
    'Unavailable',
    'DataLoss',
    'Unauthenticated',
])


def GetCodeString(code):
  """Map pRPC response codes to string.

  Args:
    code: numeric pRPC response code.

  Returns:
    pRPC response code as a string.
  """
  if 0 <= code < len(PRPCCode):
    return PRPCCode[code]
  return 'pRPC code (%d) out of range' % (code)


class PRPCInvalidHostException(Exception):
  """Exception if the pRPC host is invalid."""


class PRPCResponseException(Exception):
  """Exception got from pRPC Response."""
  def __init__(self, message, transient=False):
    super(PRPCResponseException, self).__init__(message)
    self.transient = transient


class PRPCClient(object):
  """pRPC client to interact with a pRPC service."""
  def __init__(self, service_account=None, insecure=False, host=None):
    """Init a PRPCClient instance.

    Args:
      service_account: The path to the service account json file.
      insecure: Use to insecure HTTP connection.
      host: The server to interact with.

    Raises:
      PRPCInvalidHostException if a host cannot be determined.
    """
    self.http = auth.AuthorizedHttp(
        auth.GetAccessToken,
        None,
        service_account_json=service_account)
    self.insecure = insecure
    self.host = host
    if host is None:
      # Allow base class to be used if a host is specified.
      try:
        self.host = self._GetHost()
      except AttributeError:
        raise PRPCInvalidHostException('Unable to determine default host')
    self.host = self.host.strip()

  def GetScheme(self):
    return 'http' if self.insecure else 'https'

  def ConstructURL(self, service, method):
    """Construct a URL given the path using the configured host.

    Args:
      service: The pRPC service.
      method: The pRPC method.

    Returns:
      The full URL including scheme, host, port and path.
    """
    return '%(scheme)s://%(host)s/%(service)s/%(method)s' % {
        'scheme': self.GetScheme(),
        'host': self.host,
        'service': service,
        'method': method,
    }

  def SendRequest(self, service, method, body=None,
                  dryrun=False, timeout_secs=None,
                  retry_count=3):
    """Generic pRPC request.

    Args:
      service: The pRPC service.
      method: The pRPC method.
      body: The entity body to be sent with the request (a string object).
            See httplib2.Http.request for details.
      dryrun: Whether a dryrun.
      timeout_secs: Maximum number of seconds for remote server to process
                    request.
      retry_count: Number of retry attempts on transient failures.

    Returns:
      A dict of the decoded JSON response.

    Raises:
      PRPCResponseException if the pRPC response is invalid.
    """
    url = self.ConstructURL(service, method)

    if dryrun:
      logging.info(
          'Dryrun mode is on; Would have made a request with url %s body:\n%s',
          url, body)
      return {}

    headers = {
        'Accept': 'application/json',
        'Content-Type': 'application/json',
    }
    if timeout_secs:
      headers['X-Prpc-Timeout'] = '%dS' % (timeout_secs)

    def AllowRetry(e):
      return (isinstance(e, httplib2.ServerNotFoundError) or
              isinstance(e, socket.error) or
              isinstance(e, socket.timeout) or
              (isinstance(e, PRPCResponseException) and e.transient))

    def IsTransientHTTPStatus(status):
      return status >= 500

    def IsTransientPRPCCode(code):
      return code in (PRPCCode.Unknown, PRPCCode.Internal, PRPCCode.Unavailable)

    def TryMethod():
      response, content = self.http.request(url, POST_METHOD, body=body,
                                            headers=headers)

      # Check HTTP status code.
      if 'status' not in response:
        raise PRPCResponseException('Missing HTTP response code with url: %s\n'
                                    'content: %s' % (url, content))
      status = int(response['status'])
      if status not in (httplib.OK, httplib.NO_CONTENT):
        raise PRPCResponseException(
            'Got a %s response with url: %s\ncontent: %s' %
            (response['status'], url, content),
            transient=IsTransientHTTPStatus(status))

      # Check pRPC status code.
      if 'x-prpc-grpc-code' not in response:
        raise PRPCResponseException('Missing pRPC response code with url: %s\n'
                                    'content: %s' % (url, content))
      prpc_code = int(response['x-prpc-grpc-code'])
      if prpc_code != PRPCCode.OK:
        raise PRPCResponseException(
            'Got a %s (%s) pRPC response code with url: %s\ncontent: %s' %
            (response['x-prpc-grpc-code'], GetCodeString(prpc_code), url,
             content),
            transient=IsTransientPRPCCode(prpc_code))

      # Verify XSSI prefix.
      if content[:5] != ')]}\'\n':
        # Unwrap the gRPC message by removing XSSI prefix.
        raise PRPCResponseException('Got a non-matching XSSI prefix')

      return json.loads(content[5:])

    return retry_util.GenericRetry(AllowRetry, retry_count, TryMethod)
