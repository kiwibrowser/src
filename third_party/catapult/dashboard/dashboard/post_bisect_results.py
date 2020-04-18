# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""URL endpoint to allow bisect bots to post results to the dashboard."""

import json
import logging

from google.appengine.api import app_identity
from google.appengine.ext import ndb

from dashboard import post_data_handler
from dashboard.common import datastore_hooks
from dashboard.common import utils
from dashboard.models import try_job

_EXPECTED_RESULT_PROPERTIES = {
    'status': ['pending', 'started', 'completed', 'failed', 'aborted'],
}


class BadRequestError(Exception):
  """An error indicating that a 400 response status should be returned."""
  pass


class PostBisectResultsHandler(post_data_handler.PostDataHandler):

  def post(self):
    """Validates data parameter and saves to TryJob entity.

    Bisect results come from a "data" parameter, which is a JSON encoding of a
    dictionary.

    The required fields are "master", "bot", "test".

    Request parameters:
      data: JSON encoding of a dictionary.

    Outputs:
      Empty 200 response with if successful,
      200 response with warning message if optional data is invalid,
      403 response with error message if sender IP is not white-listed,
      400 response with error message if required data is invalid.
      500 with error message otherwise.
    """
    datastore_hooks.SetPrivilegedRequest()
    if not self._CheckIpAgainstWhitelist():
      return

    data = self.request.get('data')
    if not data:
      self.ReportError('Missing "data" parameter.', status=400)
      return

    logging.info('Received data: %s', data)

    try:
      data = json.loads(self.request.get('data'))
    except ValueError:
      self.ReportError('Invalid JSON string.', status=400)
      return

    try:
      _ValidateResultsData(data)
      job = _GetTryJob(data)
      if not job:
        self.ReportWarning('No try job found.')
        return
      _UpdateTryJob(job, data)
    except BadRequestError as error:
      self.ReportError(error.message, status=400)


def _ValidateResultsData(results_data):
  utils.Validate(_EXPECTED_RESULT_PROPERTIES, results_data)
  # TODO(chrisphan): Validate other values.


def _UpdateTryJob(job, results_data):
  if not job.results_data:
    job.results_data = {}
  job.results_data.update(results_data)
  job.results_data['issue_url'] = (job.results_data.get('issue_url') or
                                   _IssueURL(job))
  job.put()


def _GetTryJob(results_data):
  try_job_id = results_data.get('try_job_id')
  if not try_job_id:
    return None
  job = ndb.Key(try_job.TryJob, try_job_id).get()
  return job


def _IssueURL(job):
  """Returns a URL for information about a bisect try job."""
  hostname = app_identity.get_default_version_hostname()
  job_id = job.buildbucket_job_id
  return 'https://%s/buildbucket_job_status/%s' % (hostname, job_id)
