# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import webapp2

from dashboard.pinpoint.models import job as job_module


class Job(webapp2.RequestHandler):

  def get(self, job_id):
    # Validate parameters.
    try:
      job = job_module.JobFromId(job_id)
    except ValueError:
      self.response.set_status(400)
      self.response.write(json.dumps({'error': 'Invalid job id.'}))
      return

    if not job:
      self.response.set_status(404)
      self.response.write(json.dumps({'error': 'Unknown job id.'}))
      return

    opts = self.request.get_all('o')
    self.response.write(json.dumps(job.AsDict(opts)))
