# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Downloads a single time series as CSV."""

import csv
import logging
import StringIO

from dashboard.common import datastore_hooks
from dashboard.common import request_handler
from dashboard.common import utils
from dashboard.models import graph_data


class GraphCsvHandler(request_handler.RequestHandler):
  """Request handler for getting data from one series as CSV."""

  def get(self):
    """Gets CSV from data store and outputs it.

    Request parameters:
      test_path: Full test path of one trace.
      rev: End revision number; if not given, latest revision is used.
      num_points: Number of Rows to get data for.
      attr: Comma-separated list of attributes (columns) to return.

    Outputs:
      CSV file contents.
    """
    test_path = self.request.get('test_path')
    rev = self.request.get('rev')
    num_points = int(self.request.get('num_points', 500))
    attributes = self.request.get('attr', 'revision,value').split(',')

    if not test_path:
      self.ReportError('No test path given.', status=400)
      return

    logging.info('Got request to /graph_csv for test: "%s".', test_path)

    test_key = utils.TestKey(test_path)
    test = test_key.get()
    assert(datastore_hooks.IsUnalteredQueryPermitted() or
           not test.internal_only)
    datastore_hooks.SetSinglePrivilegedRequest()
    q = graph_data.Row.query()
    q = q.filter(graph_data.Row.parent_test == utils.OldStyleTestKey(test_key))
    if rev:
      q = q.filter(graph_data.Row.revision <= int(rev))
    q = q.order(-graph_data.Row.revision)
    points = reversed(q.fetch(limit=num_points))

    rows = self._GenerateRows(points, attributes)

    output = StringIO.StringIO()
    csv.writer(output).writerows(rows)
    self.response.headers['Content-Type'] = 'text/csv'
    self.response.headers['Content-Disposition'] = (
        'attachment; filename=%s.csv' % test.test_name)
    self.response.out.write(output.getvalue())

  def post(self):
    """A post request is the same as a get request for this endpoint."""
    self.get()

  def _GenerateRows(self, points, attributes):
    """Generates CSV rows based on the attributes given.

    Args:
      points: A list of Row entities.
      attributes: A list of properties of Row entities to get.

    Returns:
      A list of lists of attribute values for the given points.
    """
    rows = [attributes]
    for point in points:
      row = []
      for attr in attributes:
        row.append(getattr(point, attr, ''))
      rows.append(row)
    return rows
