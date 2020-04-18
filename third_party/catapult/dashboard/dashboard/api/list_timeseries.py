# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from google.appengine.ext import ndb

from dashboard.api import api_request_handler
from dashboard.common import utils
from dashboard.models import graph_data


class ListTimeseriesHandler(api_request_handler.ApiRequestHandler):
  """API handler for listing timeseries for a benchmark."""

  def AuthorizedPost(self, *args):
    """Returns list in response to API requests.

    Argument:
      benchmark: name of the benchmark to list tests for

    Outputs:
      JSON list of monitored timeseries for the benchmark, see README.md.
    """
    benchmark = args[0]
    sheriff_name = self.request.get('sheriff', 'Chromium Perf Sheriff')
    query = graph_data.TestMetadata.query()
    query = query.filter(graph_data.TestMetadata.suite_name == benchmark)
    query = query.filter(graph_data.TestMetadata.has_rows == True)
    query = query.filter(graph_data.TestMetadata.deprecated == False)
    if sheriff_name != 'all':
      sheriff = ndb.Key('Sheriff', sheriff_name)
      query = query.filter(graph_data.TestMetadata.sheriff == sheriff)
    keys = query.fetch(keys_only=True)
    return [utils.TestPath(key) for key in keys]
