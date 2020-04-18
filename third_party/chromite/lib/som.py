# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Helpers for interacting with sheriff-o-matic."""

from __future__ import print_function

import json

from chromite.lib import auth
from chromite.lib import constants
from chromite.lib import cros_logging as logging
from chromite.lib import retry_util
from chromite.cbuildbot import topology


# Methods
GET_METHOD = 'GET'
POST_METHOD = 'POST'


# The following classes are intended to marshal into json matching
# the structures in
# https://cs.chromium.org/chromium/infra/go/src/infra/monitoring/messages/alerts.go  # pylint: disable=line-too-long

class Alert(object):
  """JSON structure for a single Sheriff-o-matic Alert."""
  def __init__(self, key, title, body, severity, time, start_time, links, tags,
               atype, extension):
    self.key = key
    self.title = title
    self.body = body
    self.severity = severity
    self.time = time
    self.start_time = start_time
    self.links = links
    self.tags = tags
    self.type = atype
    self.extension = extension


class AlertsSummary(object):
  """JSON structure for full set of Sheriff-o-matic alerts."""
  def __init__(self, alerts, revision_summaries, timestamp):
    self.alerts = alerts
    self.revision_summaries = revision_summaries
    self.timestamp = timestamp


class Link(object):
  """JSON structure for a link within an alert."""
  def __init__(self, title, href):
    self.title = title
    self.href = href


class AlertedBuilder(object):
  """JSON structure for a a single failing builder within an alert."""
  def __init__(self, name, url, start_time, first_failure, latest_failure):
    self.name = name
    self.url = url
    self.start_time = start_time
    self.first_failure = first_failure
    self.latest_failure = latest_failure


# The following classes do not directly map into go structures but are
# parsed by code within
# https://cs.chromium.org/chromium/infra/go/src/infra/appengine/sheriff-o-matic/elements/  # pylint: disable=line-too-long

class CrosStageFailure(object):
  """JSON structure with details on a stage that failed."""
  def __init__(self, name, status, logs, links, notes):
    self.name = name
    self.status = status
    self.logs = logs
    self.links = links
    self.notes = notes


class CrosBuildFailure(object):
  """JSON structure with details on a build that failed."""
  def __init__(self, notes, stages, builders):
    self.notes = notes
    self.stages = stages
    self.builders = builders


class SheriffOMaticResponseException(Exception):
  """Exception got from Sheriff-o-Matic Response."""


class SheriffOMaticClient(object):
  """Sheriff-o-Matic client to interact with the Sheriff-o-Matic frontend."""

  def __init__(self, service_account=None, insecure=False, host=None):
    """Init a SheriffOMaticClient instance.

    Args:
      service_account: The path to the service account json file.
      insecure: Fall-back to insecure HTTP connection.
      host: The Sheriff-o-Matic instance to interact with.
    """
    self.http = auth.AuthorizedHttp(
        auth.GetAccessToken,
        None,
        service_account_json=service_account)
    self.insecure = insecure
    self.host = (self._GetHost() if host is None else host).strip()

  def _GetHost(self):
    """Get Sheriff-o-matic Server host from topology."""
    return topology.topology.get(topology.SHERIFFOMATIC_HOST_KEY)

  def SendRequest(self, url, method, body, dryrun):
    """Generic Sheriff-o-Matic request.

    Args:
      url: Sheriff-o-Matic url to send requests.
      method: HTTP method to perform, such as GET, POST, DELETE.
      body: The entity body to be sent with the request (a string object).
            See httplib2.Http.request for details.
      dryrun: Whether a dryrun.

    Returns:
      A dict of response entity body if the request succeeds; else, None.
      See httplib2.Http.request for details.

    Raises:
      SheriffOMaticResponseException when response['status'] is invalid.
    """
    if dryrun:
      logging.info('Dryrun mode is on; Would have made a request '
                   'with url %s method %s body:\n%s', url, method, body)
      return

    def try_method():
      response, content = self.http.request(
          url,
          method,
          body=body,
          headers={'Content-Type': 'application/json'},
      )

      if int(response['status']) // 100 != 2:
        raise SheriffOMaticResponseException(
            'Got a %s response from Sheriff-o-Matic with url: %s\n'
            'content: %s' % (response['status'], url, content))

      return content

    return retry_util.GenericRetry(lambda e: isinstance(e, Exception), 3,
                                   try_method)

  def SendAlerts(self, summary_json, tree=constants.SOM_TREE, dryrun=False):
    """Upload alerts summary to Sheriff-o-matic.

    Args:
      summary_json: JSON version of AlertsSummary structure.
      tree: Sheriff-o-Matic tree to send alerts to.
      dryrun: Whether a dryrun.

    Returns:
      Results of HTTP request.
    """
    url = '%(scheme)s://%(hostname)s/api/v1/alerts/%(tree)s' % {
        'scheme': 'http' if self.insecure else 'https',
        'hostname': self.host,
        'tree': tree,
    }
    return self.SendRequest(url, POST_METHOD, summary_json, dryrun=dryrun)

  def SendAlert(self, alert_json, key=None, tree=constants.SOM_TREE,
                dryrun=False):
    """Upload alert to Sheriff-o-matic.

    Args:
      alert_json: JSON version of Alert structure.
      key: Key for alert, defaults to alert.key if None.
      tree: Sheriff-o-Matic tree to send alerts to.
      dryrun: Whether a dryrun.

    Returns:
      Results of HTTP request.
    """
    if key is None:
      key = json.loads(alert_json)['key']

    url = '%(scheme)s://%(hostname)s/api/v1/alert/%(tree)s/%(key)s' % {
        'scheme': 'http' if self.insecure else 'https',
        'hostname': self.host,
        'tree': tree,
        'key': key,
    }
    return self.SendRequest(url, POST_METHOD, alert_json, dryrun=dryrun)

  def ResolveAlert(self, key, resolved=True, xsrf_token=None,
                   tree=constants.SOM_TREE, dryrun=False):
    """Resolve alert in Sheriff-o-matic.

    Args:
      key: Key for alert.
      resolved: Should the alert be resolved or unresolved.
      xsrf_token: XSRF token to include with request.
      tree: Sheriff-o-Matic tree of alerts.
      dryrun: Whether a dryrun.

    Returns:
      Results of HTTP request.
    """
    url = '%(scheme)s://%(hostname)s/api/v1/resolve/%(tree)s/%(key)s' % {
        'scheme': 'http' if self.insecure else 'https',
        'hostname': self.host,
        'tree': tree,
        'key': key,
    }
    if xsrf_token is None:
      xsrf_token = self.XSRFToken()
    request = {
        'xsrf_token': xsrf_token,
        'data': {
            'resolved': resolved,
        },
    }
    return self.SendRequest(url, POST_METHOD, json.dumps(request),
                            dryrun=dryrun)

  def XSRFToken(self, dryrun=False):
    """Retrieve XSRF token from Sheriff-o-matic.

    Args:
      dryrun: Whether a dryrun.

    Returns:
      XSRF token, None if response cannot be parsed.
    """
    url = '%(scheme)s://%(hostname)s/api/v1/xsrf_token' % {
        'scheme': 'http' if self.insecure else 'https',
        'hostname': self.host,
    }
    response = json.loads(self.SendRequest(url, GET_METHOD, '', dryrun=dryrun))
    return response.get('token')
