# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""URL endpoint for getting a histogram."""

import json

from google.appengine.ext import ndb

from dashboard import post_data_handler


class GetHistogramHandler(post_data_handler.PostDataHandler):
  """URL endpoint to get histogramby guid."""

  def post(self):
    """Fetches a histogram by guid.

    Request parameters:
      guid: GUID of requested histogram.

    Outputs:
      JSON serialized Histogram.
    """
    guid = self.request.get('guid')
    if not guid:
      self.ReportError('Missing "guid" parameter', status=400)
      return

    histogram_key = ndb.Key('Histogram', guid)
    try:
      histogram_entity = histogram_key.get()
    except AssertionError:
      # Thrown if accessing internal_only as an external user.
      self.ReportError('Histogram "%s" not found' % guid, status=400)
      return

    if not histogram_entity:
      self.ReportError('Histogram "%s" not found' % guid, status=400)
      return

    self.response.out.write(json.dumps(histogram_entity.data))
