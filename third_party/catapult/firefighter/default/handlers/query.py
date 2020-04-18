# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import time

from google.appengine.api import urlfetch
import webapp2

from base import bigquery
from base import constants
from common import query_filter


class Query(webapp2.RequestHandler):

  def get(self):
    urlfetch.set_default_fetch_deadline(60)

    try:
      filters = query_filter.Filters(self.request)
    except ValueError as e:
      self.response.headers['Content-Type'] = 'application/json'
      self.response.out.write({'error': str(e)})
      return
    query_results = _QueryEvents(bigquery.BigQuery(), **filters)
    trace_events = list(_ConvertQueryEventsToTraceEvents(query_results))

    self.response.headers['Content-Type'] = 'application/json'
    self.response.out.write(json.dumps(trace_events, separators=(',', ':')))


def _QueryEvents(bq, **filters):
  start_time = filters.get(
      'start_time', time.time() - constants.DEFAULT_HISTORY_DURATION_SECONDS)
  query_start_time_us = int(start_time * 1000000)

  end_time = filters.get('end_time', time.time())
  query_end_time_us = int(end_time * 1000000)

  fields = (
      'name',
      'GREATEST(INTEGER(start_time), %d) AS start_time_us' %
      query_start_time_us,
      'LEAST(INTEGER(end_time), %d) AS end_time_us' % query_end_time_us,
      'builder',
      'configuration',
      'hostname',
      'status',
      'url',
  )

  tables = (constants.BUILDS_TABLE, constants.CURRENT_BUILDS_TABLE)
  tables = ['[%s.%s]' % (constants.DATASET, table) for table in tables]
  # TODO(dtu): Taking a snapshot of the table should reduce the cost of the
  # query, but some trace events appear to be missing and not sure why.
  #tables = ['[%s.%s@%d-]' % (constants.DATASET, table, query_start_time_ms)
            #for table in tables]

  conditions = []
  conditions.append('NOT LOWER(name) CONTAINS "trigger"')
  conditions.append('end_time - start_time >= 1000000')
  conditions.append('end_time > %d' % query_start_time_us)
  conditions.append('start_time < %d' % query_end_time_us)
  for filter_name, filter_values in filters.iteritems():
    if not isinstance(filter_values, list):
      continue

    if isinstance(filter_values[0], int):
      filter_values = map(str, filter_values)
    elif isinstance(filter_values[0], basestring):
      # QueryFilter handles string validation. Assume no quotes in string.
      filter_values = ['"%s"' % v for v in filter_values]
    else:
      raise NotImplementedError()

    conditions.append('%s IN (%s)' % (filter_name, ','.join(filter_values)))

  query = ('SELECT %s ' % ','.join(fields) +
           'FROM %s ' % ','.join(tables) +
           'WHERE %s ' % ' AND '.join(conditions) +
           'ORDER BY builder, start_time')
  return bq.QuerySync(query)


def _ConvertQueryEventsToTraceEvents(events):
  for row in events:
    event_start_time_us = int(row['f'][1]['v'])
    event_end_time_us = int(row['f'][2]['v'])

    status = row['f'][6]['v']
    if status:
      status = int(status)
      # TODO: Use constants from update/common/buildbot/__init__.py.
      if status == 0:
        color_name = 'cq_build_passed'
      elif status == 1:
        color_name = 'cq_build_warning'
      elif status == 2:
        color_name = 'cq_build_failed'
      elif status == 4:
        color_name = 'cq_build_exception'
      elif status == 5:
        color_name = 'cq_build_abandoned'
    else:
      color_name = 'cq_build_running'

    yield {
        'name': row['f'][0]['v'],
        'pid': row['f'][4]['v'],
        'tid': '%s [%s]' % (row['f'][3]['v'], row['f'][5]['v']),
        'ph': 'X',
        'ts': event_start_time_us,
        'dur': event_end_time_us - event_start_time_us,
        'cname': color_name,
        'args': {
            'url': row['f'][7]['v'],
        },
    }
