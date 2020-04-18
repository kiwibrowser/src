# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A Model that represents one bisect or perf test try job.

TryJob entities are checked in /update_bug_with_results to check completed
bisect jobs and update bugs with results.

They are also used in /auto_bisect to restart unsuccessful bisect jobs.
"""

import datetime
import json
import logging

from google.appengine.ext import ndb

from dashboard.models import bug_data
from dashboard.models import internal_only_model
from dashboard.services import buildbucket_service


class TryJob(internal_only_model.InternalOnlyModel):
  """Stores config and tracking info about a single try job."""
  bot = ndb.StringProperty()
  config = ndb.TextProperty()
  bug_id = ndb.IntegerProperty()
  email = ndb.StringProperty()
  rietveld_issue_id = ndb.IntegerProperty()
  rietveld_patchset_id = ndb.IntegerProperty()
  master_name = ndb.StringProperty(default='ChromiumPerf', indexed=False)
  buildbucket_job_id = ndb.StringProperty()
  internal_only = ndb.BooleanProperty(default=False, indexed=True)

  # Bisect run status (e.g., started, failed).
  status = ndb.StringProperty(
      default='pending',
      choices=[
          'pending',  # Created, but job start has not been confirmed.
          'started',  # Job is confirmed started.
          'failed',   # Job terminated, red build.
          'staled',   # No updates from bots.
          'completed',  # Job terminated, green build.
          'aborted',  # Job terminated with abort (purple, early abort).
      ],
      indexed=True)

  # Last time this job was started.
  last_ran_timestamp = ndb.DateTimeProperty()

  job_type = ndb.StringProperty(
      default='bisect',
      choices=['bisect', 'bisect-fyi', 'perf-try'])

  # job_name attribute is used by try jobs of bisect FYI.
  job_name = ndb.StringProperty(default=None)

  # Results data coming from bisect bots.
  results_data = ndb.JsonProperty(indexed=False)

  log_record_id = ndb.StringProperty(indexed=False)

  # Sets of emails of users who has confirmed this TryJob result is bad.
  bad_result_emails = ndb.PickleProperty()

  def SetStarted(self):
    self.status = 'started'
    self.last_ran_timestamp = datetime.datetime.now()
    self.put()
    if self.bug_id:
      bug_data.SetBisectStatus(self.bug_id, 'started')

  def SetFailed(self):
    self.status = 'failed'
    self.put()
    if self.bug_id:
      bug_data.SetBisectStatus(self.bug_id, 'failed')

  def SetStaled(self):
    self.status = 'staled'
    self.put()
    logging.info('Updated status to staled')
    # TODO(sullivan, dtu): what is the purpose of 'staled' status? Doesn't it
    # just prevent updating jobs older than 24 hours???
    if self.bug_id:
      bug_data.SetBisectStatus(self.bug_id, 'failed')

  def SetCompleted(self):
    logging.info('Updated status to completed')
    self.status = 'completed'
    self.put()
    if self.bug_id:
      bug_data.SetBisectStatus(self.bug_id, 'completed')

  def GetCulpritCL(self):
    if not self.results_data:
      return None
    # culprit_data can be undefined or explicitly set to None
    culprit_data = self.results_data.get('culprit_data') or {}
    return culprit_data.get('cl')

  def GetConfigDict(self):
    return json.loads(self.config.split('=', 1)[1])

  def CheckFailureFromBuildBucket(self):
    # Buildbucket job id is not always set.
    if not self.buildbucket_job_id:
      return
    job_info = buildbucket_service.GetJobStatus(self.buildbucket_job_id)
    data = job_info.get('build', {})

    # Since the job is completed successfully, results_data must
    # have been set appropriately by the bisector.
    # The buildbucket job's 'status' and 'result' fields are documented here:
    # https://goto.google.com/bb_status
    if data.get('status') == 'COMPLETED' and data.get('result') == 'SUCCESS':
      return

    # Proceed if the job failed or cancelled
    logging.info('Job failed. Buildbucket id %s', self.buildbucket_job_id)
    data['result_details'] = json.loads(data['result_details_json'])
    # There are various failure and cancellation reasons for a buildbucket
    # job to fail as listed in https://goto.google.com/bb_status.
    job_updates = {
        'failure_reason': (data.get('cancelation_reason') or
                           data.get('failure_reason')),
        'buildbot_log_url': data.get('url')
    }
    details = data.get('result_details')
    if details:
      properties = details.get('properties')
      if properties:
        job_updates['bisect_bot'] = properties.get('buildername')
        job_updates['extra_result_code'] = properties.get(
            'extra_result_code')
        bisect_config = properties.get('bisect_config')
        if bisect_config:
          job_updates['try_job_id'] = bisect_config.get('try_job_id')
          job_updates['bug_id'] = bisect_config.get('bug_id')
          job_updates['command'] = bisect_config.get('command')
          job_updates['test_type'] = bisect_config.get('test_type')
          job_updates['metric'] = bisect_config.get('metric')
          job_updates['good_revision'] = bisect_config.get('good_revision')
          job_updates['bad_revision'] = bisect_config.get('bad_revision')
    if not self.results_data:
      self.results_data = {}
    self.results_data.update(job_updates)
    self.status = 'failed'
    self.last_ran_timestamp = datetime.datetime.fromtimestamp(
        float(data['updated_ts'])/1000000)
    self.put()
    logging.info('updated status to failed.')

