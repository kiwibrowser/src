# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Provides the web interface for displaying an overview of jobs."""

import json
import webapp2

from dashboard.pinpoint.models import job as job_module


_MAX_JOBS_TO_FETCH = 1000


# TODO: Generalize the Jobs handler to allow the user to choose what fields to
# include and how many Jobs to fertch.

class Stats(webapp2.RequestHandler):
  """Shows an overview of recent anomalies for perf sheriffing."""

  def get(self):
    self.response.out.write(json.dumps(_GetJobs()))


def _GetJobs():
  query = job_module.Job.query().order(-job_module.Job.created)
  jobs = query.fetch(limit=_MAX_JOBS_TO_FETCH)

  job_infos = []
  for job in jobs:
    job_infos.append({
        'created': job.created.isoformat(),
        # TODO: Don't access JobState outside of the Job object.
        'differences': len(list(job.state.Differences())),
        'status': job.status,
    })

  return job_infos
