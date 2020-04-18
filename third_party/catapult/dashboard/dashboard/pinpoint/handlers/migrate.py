# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import json
import webapp2

from google.appengine.api import taskqueue
from google.appengine.datastore import datastore_query
from google.appengine.ext import ndb

from dashboard.common import stored_object
from dashboard.pinpoint.models import job


_BATCH_SIZE = 10
_STATUS_KEY = 'job_migration_status'


class Migrate(webapp2.RequestHandler):

  def get(self):
    self.response.write(json.dumps(stored_object.Get(_STATUS_KEY) or {}))

  def post(self):
    query = job.Job.query(job.Job.task == None)
    status = stored_object.Get(_STATUS_KEY)

    if not status:
      self._Start(query)
      self.get()
      return

    self._Migrate(query, status)
    self.get()

  def _Start(self, query):
    status = {
        'count': 0,
        'started': datetime.datetime.now().isoformat(),
        'total': query.count(),
    }
    stored_object.Set(_STATUS_KEY, status)
    taskqueue.add(url='/api/migrate')

  def _Migrate(self, query, status):
    cursor = datastore_query.Cursor(urlsafe=self.request.get('cursor'))
    jobs, next_cursor, more = query.fetch_page(_BATCH_SIZE, start_cursor=cursor)
    ndb.put_multi(jobs)

    if more:
      status['count'] += len(jobs)
      stored_object.Set(_STATUS_KEY, status)
      params = {'cursor': next_cursor.urlsafe()}
      taskqueue.add(url='/api/migrate', params=params)
    else:
      stored_object.Set(_STATUS_KEY, None)
