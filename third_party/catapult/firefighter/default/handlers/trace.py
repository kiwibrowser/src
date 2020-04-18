# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import urllib

import webapp2

from common import jinja
from common import query_filter


class Trace(webapp2.RequestHandler):

  def get(self):
    try:
      filters = query_filter.Filters(self.request)
    except ValueError as e:
      self.response.headers['Content-Type'] = 'application/json'
      self.response.out.write({'error': str(e)})
      return

    query_parameters = []
    for filter_name, filter_values in filters.iteritems():
      if filter_name == 'start_time':
        query_parameters.append(('start_time', filter_values))
      elif filter_name == 'end_time':
        query_parameters.append(('end_time', filter_values))
      else:
        for filter_value in filter_values:
          query_parameters.append((filter_name, filter_value))
    template_values = {
        'query_string': urllib.urlencode(query_parameters),
    }

    template = jinja.ENVIRONMENT.get_template('trace.html')
    # pylint: disable=no-member
    self.response.out.write(template.render(template_values))
