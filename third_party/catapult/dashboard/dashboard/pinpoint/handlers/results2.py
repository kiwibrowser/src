# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Provides the web interface for displaying a results2 file."""

import json
import webapp2

from dashboard.pinpoint.models import job as job_module
from dashboard.pinpoint.models import results2


class Results2(webapp2.RequestHandler):
  """Shows an overview of recent anomalies for perf sheriffing."""

  def get(self, job_id):
    try:
      job = job_module.JobFromId(job_id)
      if not job:
        raise results2.Results2Error('Error: Unknown job %s' % job_id)

      if job.task:
        self.response.out.write(json.dumps({'status': 'job-incomplete'}))
        return

      url = results2.GetCachedResults2(job)
      if url:
        self.response.out.write(json.dumps({'status': 'complete', 'url': url}))
        return

      if results2.ScheduleResults2Generation(job):
        self.response.out.write(json.dumps({'status': 'pending'}))
        return

      self.response.out.write(json.dumps({'status': 'failed'}))

    except results2.Results2Error as e:
      self.response.set_status(400)
      self.response.out.write(e.message)


class Results2Generator(webapp2.RequestHandler):
  """Creates a results2 file and streams it to cloud storage."""

  def post(self, job_id):
    try:
      job = job_module.JobFromId(job_id)
      if not job:
        raise results2.Results2Error('Error: Unknown job %s' % job_id)
      results2.GenerateResults2(job)
    except results2.Results2Error as e:
      self.response.out.write(e.message)
