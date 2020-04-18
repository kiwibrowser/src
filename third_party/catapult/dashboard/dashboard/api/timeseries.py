# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import datetime

from dashboard.api import api_request_handler
from dashboard.common import datastore_hooks
from dashboard.common import namespaced_stored_object
from dashboard.common import utils
from dashboard.models import graph_data


class BadRequestError(Exception):
  pass


class TimeseriesHandler(api_request_handler.ApiRequestHandler):
  """API handler for getting timeseries data."""

  def AuthorizedPost(self, *args):
    """Returns timeseries data in response to API requests.

    Argument:
      test_path: Full path of test timeseries

    Outputs:
      JSON timeseries data for the test_path, see README.md.
    """
    try:
      days = int(self.request.get('num_days', 30))
    except ValueError:
      raise api_request_handler.BadRequestError(
          'Invalid num_days parameter %s' % self.request.get('num_days'))
    if days <= 0:
      raise api_request_handler.BadRequestError(
          'num_days cannot be negative (%s)' % days)
    before = datetime.datetime.now() - datetime.timedelta(days=days)

    test_path = args[0]
    test_key = utils.TestKey(test_path)
    test = test_key.get()
    if not test:
      raise api_request_handler.BadRequestError(
          'Invalid test_path %s' % test_path)

    assert(
        datastore_hooks.IsUnalteredQueryPermitted() or not test.internal_only)
    datastore_hooks.SetSinglePrivilegedRequest()

    q = graph_data.Row.query()
    q = q.filter(graph_data.Row.parent_test == utils.OldStyleTestKey(test_key))
    q = q.filter(graph_data.Row.timestamp > before)

    rows = q.fetch()
    if not rows:
      return []
    revisions = [rev for rev in rows[0].to_dict() if rev.startswith('r_')]
    header = ['revision', 'value', 'timestamp'] + revisions
    timeseries = [header]
    for row in sorted(rows, key=lambda r: r.revision):
      timeseries.append([self._GetValue(row, a) for a in header])

    return {
        'timeseries': timeseries,
        'test_path': test_path,
        'revision_logs': namespaced_stored_object.Get('revision_info'),
    }

  def _GetValue(self, row, attr):
    value = getattr(row, attr, None)
    if attr == 'timestamp':
      return value.isoformat()
    return value
