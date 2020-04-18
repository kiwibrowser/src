# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Prod host metrics"""

from __future__ import absolute_import
from __future__ import print_function

import collections
import json
import subprocess

from chromite.lib import cros_logging as logging
from chromite.lib import metrics

from infra_libs import ts_mon

_METRIC_ROOT_PATH = 'prod_hosts/'
_ATEST_PROGRAM = '/usr/local/autotest/cli/atest'

logger = logging.getLogger(__name__)


def collect_prod_hosts():
  servers = list(_get_servers())
  sink = _TsMonSink(_METRIC_ROOT_PATH)
  sink.write_servers(servers)


def _get_servers():
  """Get server information from atest.

  Returns:
    Iterable of Server instances.
  """
  output = subprocess.check_output([_ATEST_PROGRAM, 'server', 'list', '--json'])
  server_dicts = json.loads(output)
  for server in server_dicts:
    yield Server(
        hostname=_get_hostname(server),
        data_center=_get_data_center(server),
        status=server['status'],
        roles=tuple(server['roles']),
        created=server['date_created'],
        modified=server['date_modified'],
        note=server['note'])


def _get_hostname(server):
  """Get server hostname from an atest dict.

  >>> server = {'hostname': 'foo.example.com'}  # from atest
  >>> _get_hostname(server)
  'foo'
  """
  return server['hostname'].partition('.')[0]


def _get_data_center(server):
  """Get server data center from an atest dict.

  >>> server = {'hostname': 'foo.mtv.example.com'}  # from atest
  >>> _get_data_center(server)
  'mtv'
  """
  try:
    return server['hostname'].split('.')[1]
  except IndexError:
    raise ValueError('%r hostname is invalid' % server)


Server = collections.namedtuple(
    'Server', 'hostname,data_center,status,roles,created,modified,note')


class _TsMonSink(object):
  """Sink using ts_mon to report Monarch metrics."""

  def __init__(self, metric_root_path):
    """Initialize instance.

    Args:
      metric_root_path: Path for ts_mon metrics.  Should end in a slash.
    """
    if not metric_root_path.endswith('/'):
      raise ValueError('metric_root_path should end with slash',
                       metric_root_path)
    self._metric_root_path = metric_root_path

  def write_servers(self, servers):
    """Write server information.

    Args:
      servers: Iterable of Server instances.
    """
    # See crbug.com/767265: Without this .reset() call, we will continue to
    # emit old presence and roles data.
    self._presence_metric.reset()
    self._roles_metric.reset()
    self._ignored_metric.reset()

    for server in servers:
      logger.debug('Server: %r', server)
      fields = {
          'target_hostname': server.hostname,
          'target_data_center': server.data_center,
      }
      self._presence_metric.set(True, fields)
      self._roles_metric.set(self._format_roles(server.roles), fields)
      self._ignored_metric.set(_is_ignored(server), fields)

  @property
  def _presence_metric(self):
    return metrics.Boolean(
        self._metric_root_path + 'presence',
        description=(
            'A boolean indicating whether a server is in the machines db.'),
        field_spec=[ts_mon.StringField('target_data_center'),
                    ts_mon.StringField('target_hostname'),])

  @property
  def _roles_metric(self):
    return metrics.String(
        self._metric_root_path + 'roles',
        description=(
            'A string indicating the role of a server in the machines db.'),
        field_spec=[ts_mon.StringField('target_data_center'),
                    ts_mon.StringField('target_hostname'),])

  @property
  def _ignored_metric(self):
    return metrics.Boolean(
        self._metric_root_path + 'ignored',
        description=(
            'A boolean, for servers ignored for test infra prod alerts.'),
        field_spec=[ts_mon.StringField('target_data_center'),
                    ts_mon.StringField('target_hostname'),])

  def _format_roles(self, roles):
    return ','.join(sorted(roles))


def _is_ignored(server):
  """Return True if the server should be ignored for test infra prod alerts."""
  if server.hostname.startswith('chromeos1-'):
    return True
  if server.hostname.startswith('chromeos15-'):
    return True
  return False
