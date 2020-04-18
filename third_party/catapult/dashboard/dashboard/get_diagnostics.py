# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import json
import sys

from google.appengine.ext import ndb

from dashboard import post_data_handler
from dashboard.models import histogram

class GetDiagnosticsHandler(post_data_handler.PostDataHandler):
  """URL endpoint to get diagnostics by guid or test path."""

  def post(self):
    """Fetches a diagnostic by guid or test path and start and end revisions.

    Request parameters:
      guid: GUID of requested diagnostic.
      start_revision: Start revision of requested diagnostic(s).
      end_revision: End revision of requested diagnostic(s).
      test_path: Test path of requested diagnostic(s).
      name: Name of requested diagnostic(s).

    Outputs:
      List of diagnostics.
    """
    guid = self.request.get('guid')
    start_revision = self.request.get('start_revision')
    end_revision = self.request.get('end_revision')
    test_path = self.request.get('test_path')
    name = self.request.get('name')

    if guid:
      if start_revision or end_revision or test_path or name:
        self.ReportError(
            'Fetching by GUID requires no additional parameters', status=400)
        return
      results = self._FetchByGuid(guid)
    elif test_path:
      results = self._FetchByTestPath(test_path,
                                      start_revision,
                                      end_revision,
                                      name)
    else:
      self.ReportError('Missing parameter', status=400)
      return

    diagnostics = []
    for result in results:
      diagnostics.append(
          {
              'start_revision': result.start_revision,
              'end_revision': result.end_revision,
              'test_path': result.test.id(),
              'name': result.name,
              'data': result.data,
          }
      )

    self.response.out.write(json.dumps(diagnostics))

  def _FetchByGuid(self, guid):
    try:
      diagnostic = ndb.Key('SparseDiagnostic', guid).get()
    except AssertionError:
      # Thrown if accessing internal_only as an external user.
      self.ReportError('Diagnostic not found', status=400)
      return []

    if not diagnostic:
      self.ReportError('Diagnostic not found', status=400)
      return []

    return [diagnostic]

  def _FetchByTestPath(self, test_path, start_revision, end_revision, name):
    query = histogram.SparseDiagnostic.query()
    query = query.filter(
        histogram.SparseDiagnostic.test == ndb.Key('TestMetadata', test_path))
    filter_again = False

    if end_revision == 'last':
      query = query.filter(
          histogram.SparseDiagnostic.end_revision == sys.maxint)

    elif start_revision and end_revision:
      query = query.filter(
          histogram.SparseDiagnostic.end_revision >= int(start_revision))
      filter_again = True

    else:
      self.ReportError('Missing parameter', status=400)
      return []

    query = query.order(-histogram.SparseDiagnostic.end_revision)

    results = list(query.fetch(500))

    if name:
      results = [result for result in results if result.name == name]

    if filter_again:
      # Workaround because appengine doesn't support two inequality filters
      results = [result for result in results if
                 result.start_revision <= int(end_revision)]

    if not results:
      self.ReportError('Diagnostic(s) not found', status=400)
      return []

    return results
