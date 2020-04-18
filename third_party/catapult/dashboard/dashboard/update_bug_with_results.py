# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""URL endpoint for a cron job to update bugs after bisects."""

import datetime
import json
import logging
import re
import traceback

from google.appengine.api import mail
from google.appengine.ext import ndb

from dashboard import bisect_report
from dashboard import email_template
from dashboard import layered_cache
from dashboard.common import datastore_hooks
from dashboard.common import request_handler
from dashboard.common import utils
from dashboard.models import anomaly
from dashboard.models import bug_data
from dashboard.models import try_job
from dashboard.services import buildbucket_service
from dashboard.services import issue_tracker_service


COMPLETED, FAILED, PENDING, ABORTED = ('completed', 'failed', 'pending',
                                       'aborted')

_COMMIT_HASH_CACHE_KEY = 'commit_hash_%s'

# Amount of time to pass before deleting a try job.
_STALE_TRYJOB_DELTA = datetime.timedelta(days=7)

_AUTO_ASSIGN_MSG = """
=== Auto-CCing suspected CL author %(author)s ===

Hi %(author)s, the bisect results pointed to your CL, please take a look at the
results.

"""

_NOT_DUPLICATE_MULTIPLE_BUGS_MSG = """
Possible duplicate of crbug.com/%s, but not merging issues due to multiple
culprits in destination issue.
"""


_CONFIDENCE_LEVEL_TO_CC_AUTHOR = 95

_BUILD_FAILURE_REASON = {
    'BUILD_FAILURE': 'the build has failed.',
    'INFRA_FAILURE': 'the build has failed due to infrastructure failure.',
    'BUILDBUCKET_FAILURE': 'the buildbucket service failure.',
    'INVALID_BUILD_DEFINITION': 'incorrect bisect configuation.',
    'CANCELED_EXPLICITLY': 'the build was canceled explicitly.',
    'TIMEOUT': 'the build was canceled by buildbot on timeout.',
}

_BUILD_FAILURE_DETAIL = {
    'B4T_TEST_TIMEOUT': 'Timed out waiting for the test job.',
    'B4T_BUILD_TIMEOUT': 'Timed out waiting for the build.',
    'B4T_TEST_FAILURE': 'The test failed to produce parseable results.',
    'B4T_BUILD_FAILURE': 'The build could not be requested, or the job failed.',
    'B4T_BAD_REV': 'The revision range could not be expanded, or the commit '
                   'positions could not be resolved into commit hashes.',
    'B4T_REF_RANGE_FAIL': 'Either of the initial "good" or "bad" revisions '
                          'failed to be tested or built.',
    'B4T_BAD_CONFIG': 'There was a problem with the bisect_config dictionary '
                      'passed to the recipe. See output of the config step.',
    'B4T_CULPRIT_FOUND': 'A Culprit CL was found with "high" confidence.',
    'B4T_LO_INIT_CONF': 'Bisect aborted early for lack of confidence.',
    'B4T_MISSING_METRIC': 'The metric was not found in the test output.',
    'B4T_LO_FINAL_CONF': 'The bisect completed without a culprit.',
}


class BisectJobFailure(Exception):
  pass


class BugUpdateFailure(Exception):
  pass


class UpdateBugWithResultsHandler(request_handler.RequestHandler):
  """URL endpoint for a cron job to update bugs after bisects."""

  def get(self):
    """The get handler method is called from a cron job.

    It expects no parameters and has no output. It checks all current bisect try
    jobs and send comments to an issue on the issue tracker if a bisect job has
    completed.
    """
    issue_tracker = issue_tracker_service.IssueTrackerService(
        utils.ServiceAccountHttp())

    # Set privilege so we can also fetch internal try_job entities.
    datastore_hooks.SetPrivilegedRequest()

    jobs_to_check = try_job.TryJob.query(
        try_job.TryJob.status.IN(['started', 'pending'])).fetch()
    all_successful = True

    for job in jobs_to_check:
      try:
        _CheckJob(job, issue_tracker)
      except Exception as e:  # pylint: disable=broad-except
        logging.error('Caught Exception %s: %s\n%s',
                      type(e).__name__, e, traceback.format_exc())
        all_successful = False

    if all_successful:
      utils.TickMonitoringCustomMetric('UpdateBugWithResults')


def _CheckJob(job, issue_tracker):
  """Checks whether a try job is finished and updates a bug if applicable.

  This method returns nothing, but it may log errors.

  Args:
    job: A TryJob entity, which represents one bisect try job.
    issue_tracker: An issue_tracker_service.IssueTrackerService instance.
  """
  logging.info('Checking job %s for bug %s, with buildbucket_job_id %s',
               job.key.id(), job.bug_id, job.buildbucket_job_id)

  if not _IsJobCompleted(job):
    logging.info('Not yet COMPLETED.')
    return

  job.CheckFailureFromBuildBucket()

  if _IsStale(job):
    logging.info('Stale')
    job.SetStaled()
    # TODO(chrisphan): Add a staled TryJob log.
    # TODO(chrisphan): Do we want to send a FYI Bisect email here?
    # TODO(sullivan): Probably want to update bug as well.
    return

  results_data = job.results_data

  if job.job_type == 'perf-try':
    logging.info('Sending perf try job mail')
    _SendPerfTryJobEmail(job)
  elif job.job_type == 'bisect':
    logging.info('Checking bisect job')
    _CheckBisectJob(job, issue_tracker)
  else:
    logging.error('Unknown job type: %s - %s', job.key.id(), job.job_type)
    job.SetCompleted()
    return

  if results_data and results_data.get('status') == COMPLETED:
    job.SetCompleted()
  else:
    job.SetFailed()


def _CheckBisectJob(job, issue_tracker):
  results_data = job.results_data
  has_partial_result = ('revision_data' in results_data and
                        results_data['revision_data'])
  if results_data.get('status') == FAILED and not has_partial_result:
    _PostFailedResult(job, issue_tracker)
    return
  _PostSuccessfulResult(job, issue_tracker)


def _SendPerfTryJobEmail(job):
  """Sends an email to the user who started the perf try job."""
  if not job.email:
    return
  email_report = email_template.GetPerfTryJobEmailReport(job)
  if not email_report:
    return
  mail.send_mail(sender='gasper-alerts@google.com',
                 to=job.email,
                 subject=email_report['subject'],
                 body=email_report['body'],
                 html=email_report['html'])


def _GetMergeIssueDetails(issue_tracker, commit_cache_key):
  """Get's the issue this one might be merged into.

  Returns: A dict with the following fields:
    issue: The issue details from the issue tracker service.
    id: The id of the issue we should merge into. This may be set to None if
        either there is no other bug with this culprit, or we shouldn't try to
        merge into that bug.
    comments: Additional comments to add to the bug.
  """
  merge_issue_key = layered_cache.GetExternal(commit_cache_key)
  if not merge_issue_key:
    return {'issue': {}, 'id': None, 'comments': ''}

  merge_issue = issue_tracker.GetIssue(merge_issue_key)
  if not merge_issue:
    return {'issue': {}, 'id': None, 'comments': ''}

  # Check if we can duplicate this issue against an existing issue.
  merge_issue_id = None
  additional_comments = ""

  # We won't duplicate against an issue that itself is already
  # a duplicate though. Could follow the whole chain through but we'll
  # just keep things simple and flat for now.
  if merge_issue.get('status') != issue_tracker_service.STATUS_DUPLICATE:
    merge_issue_id = str(merge_issue.get('id'))

  # We also don't want to duplicate against an issue that already has a bunch
  # of bisects pointing at different culprits.
  if merge_issue_id:
    jobs = try_job.TryJob.query(
        try_job.TryJob.bug_id == int(merge_issue_id)).fetch()
    culprits = set([j.GetCulpritCL() for j in jobs if j.GetCulpritCL()])
    if len(culprits) >= 2:
      additional_comments += _NOT_DUPLICATE_MULTIPLE_BUGS_MSG % merge_issue_id
      merge_issue_id = None

  return {
      'issue': merge_issue,
      'id': merge_issue_id,
      'comments': additional_comments
  }


def _GetCulpritCLOwnerAndComment(job, authors_to_cc):
  """Get's the owner for a CL and the comment to update the bug with."""
  comment = bisect_report.GetReport(job)
  owner = None
  if authors_to_cc:
    message = _AUTO_ASSIGN_MSG % {'author': authors_to_cc[0]}
    comment = '{0}{1}'.format(message, comment)
    owner = authors_to_cc[0]
  return owner, comment


def _PostSuccessfulResult(job, issue_tracker):
  """Posts successful bisect results on issue tracker."""
  # From the results, get the list of people to CC (if applicable), the bug
  # to merge into (if applicable) and the commit hash cache key, which
  # will be used below.
  if job.bug_id < 0:
    return

  commit_cache_key = _GetCommitHashCacheKey(job.results_data)

  # Check to see if there's already an issue for this commit, if so we can
  # potentially merge the bugs.
  merge_details = _GetMergeIssueDetails(issue_tracker, commit_cache_key)

  # Only skip cc'ing the authors if we're going to merge this isn't another
  # issue.
  authors_to_cc = []
  if not merge_details['id']:
    authors_to_cc = _GetAuthorsToCC(job.results_data)

  # Add a friendly message to author of culprit CL.
  owner, comment = _GetCulpritCLOwnerAndComment(job, authors_to_cc)
  status = None
  if owner:
    status = 'Assigned'

  # Set restrict view label if the bisect results are internal only.
  labels = ['Restrict-View-Google'] if job.internal_only else None

  # TODO(sullivan): Remove this after monorail issue 2984 is resolved.
  # https://github.com/catapult-project/catapult/issues/3781
  logging.info('Adding comment to bug %s: %s', job.bug_id, comment)
  comment_added = issue_tracker.AddBugComment(
      job.bug_id, comment + merge_details['comments'],
      cc_list=authors_to_cc, merge_issue=merge_details['id'], labels=labels,
      owner=owner, status=status)
  if not comment_added:
    raise BugUpdateFailure('Failed to update bug %s with comment %s'
                           % (job.bug_id, comment))

  logging.info('Updated bug %s with results from %s',
               job.bug_id, job.buildbucket_job_id)

  # If the issue we were going to merge into was itself a duplicate, we don't
  # dup against it but we also don't merge existing anomalies to it or cache it.
  if merge_details['issue'].get('status') == (
      issue_tracker_service.STATUS_DUPLICATE):
    return

  _MapAnomaliesAndUpdateBug(merge_details['id'], job)
  _UpdateCacheKeyForIssue(merge_details['id'], commit_cache_key, job)


def _MapAnomaliesAndUpdateBug(merge_issue_id, job):
  if merge_issue_id:
    _MapAnomaliesToMergeIntoBug(merge_issue_id, job.bug_id)
    # Mark the duplicate bug's Bug entity status as closed so that
    # it doesn't get auto triaged.
    bug = ndb.Key('Bug', job.bug_id).get()
    if bug:
      bug.status = bug_data.BUG_STATUS_CLOSED
      bug.put()


def _UpdateCacheKeyForIssue(merge_issue_id, commit_cache_key, job):
  # Cache the commit info and bug ID to datastore when there is no duplicate
  # issue that this issue is getting merged into. This has to be done only
  # after the issue is updated successfully with bisect information.
  if commit_cache_key and not merge_issue_id:
    layered_cache.SetExternal(commit_cache_key, str(job.bug_id),
                              days_to_keep=30)
    logging.info('Cached bug id %s and commit info %s in the datastore.',
                 job.bug_id, commit_cache_key)


def _PostFailedResult(job, issue_tracker):
  """Posts failed bisect results on issue tracker."""
  bug_comment = 'Bisect failed: %s\n' % job.results_data.get(
      'buildbot_log_url', '')
  if job.results_data.get('failure_reason'):
    bug_comment += 'Failure reason: %s\n' % _BUILD_FAILURE_REASON.get(
        job.results_data.get('failure_reason'), 'Unknown')
  if job.results_data.get('extra_result_code'):
    bug_comment += 'Additional errors:\n'
    for code in job.results_data.get('extra_result_code'):
      bug_comment += '%s\n' % _BUILD_FAILURE_DETAIL.get(code, code)
  logging.info('Adding bug comment: %s', bug_comment)
  issue_tracker.AddBugComment(job.bug_id, bug_comment)


def _IsStale(job):
  if not job.last_ran_timestamp:
    return False
  time_since_last_ran = datetime.datetime.now() - job.last_ran_timestamp
  return time_since_last_ran > _STALE_TRYJOB_DELTA


def _MapAnomaliesToMergeIntoBug(dest_bug_id, source_bug_id):
  """Maps anomalies from source bug to destination bug.

  Args:
    dest_bug_id: Merge into bug (base bug) number.
    source_bug_id: The bug to be merged.
  """
  query = anomaly.Anomaly.query(
      anomaly.Anomaly.bug_id == int(source_bug_id))
  anomalies = query.fetch()

  bug_id = int(dest_bug_id)
  for a in anomalies:
    a.bug_id = bug_id

  ndb.put_multi(anomalies)


def _GetCommitHashCacheKey(results_data):
  """Gets a commit hash cache key for the given bisect results output.

  Args:
    results_data: Bisect results data.

  Returns:
    A string to use as a layered_cache key, or None if we don't want
    to merge any bugs based on this bisect result.
  """
  if results_data.get('culprit_data'):
    return _COMMIT_HASH_CACHE_KEY % results_data['culprit_data']['cl']
  return None


def _GetAuthorsToCC(results_data):
  """Makes a list of email addresses that we want to CC on the bug.

  TODO(qyearsley): Make sure that the bisect result bot doesn't cc
  non-googlers on Restrict-View-Google bugs. This might be done by making
  a request for labels for the bug (or by making a request for alerts in
  the datastore for the bug id and checking the internal-only property).

  Args:
    results_data: Bisect results data.

  Returns:
    A list of email addresses, possibly empty.
  """
  culprit_data = results_data.get('culprit_data')
  if not culprit_data:
    return []
  emails = [culprit_data['email']] if culprit_data['email'] else []
  emails.extend(_GetReviewersFromCulpritData(culprit_data))
  return emails


def _GetReviewersFromCulpritData(culprit_data):
  """Parse bisect log and gets reviewers email addresses from Rietveld issue.

  Note: This method doesn't get called when bisect reports multiple CLs by
  different authors, but will get called when there are multiple CLs by the
  same owner.

  Args:
    culprit_data: Bisect results culprit data.

  Returns:
    List of email addresses from the committed CL.
  """

  reviewer_list = []
  revisions_links = culprit_data['revisions_links']
  # Sometime revision page content consist of multiple "Review URL" strings
  # due to some reverted CLs, such CLs are prefixed with ">"(&gt;) symbols.
  # Should only parse CL link corresponding the revision found by the bisect.
  link_pattern = (r'(?<!&gt;\s)Review URL: <a href=[\'"]'
                  r'https://codereview.chromium.org/(\d+)[\'"].*>')
  for link in revisions_links:
    # Fetch the commit links in order to get codereview link.
    response = utils.FetchURL(link)
    if not response:
      continue
    rietveld_issue_ids = re.findall(link_pattern, response.content)
    for issue_id in rietveld_issue_ids:
      # Fetch codereview link, and get reviewer email addresses from the
      # response JSON.
      issue_response = utils.FetchURL(
          'https://codereview.chromium.org/api/%s' % issue_id)
      if not issue_response:
        continue
      issue_data = json.loads(issue_response.content)
      reviewer_list.extend([str(item) for item in issue_data['reviewers']])
  return reviewer_list


def _IsJobCompleted(job):
  """Checks whether the bisect job is completed."""
  # Perf try jobs triggered by dashboard still not using buildbucket APIs.
  # https://github.com/catapult-project/catapult/issues/2817
  if not job.buildbucket_job_id:
    if (job.results_data and
        job.results_data.get('status') in [COMPLETED, FAILED]):
      return True
    else:
      return False

  job_info = buildbucket_service.GetJobStatus(job.buildbucket_job_id)
  if not job_info:
    return False
  data = job_info.get('build', {})
  # The status of the build can be one of [STARTED, SCHEDULED or COMPLETED]
  # The buildbucket 'status' fields are documented here:
  # https://goto.google.com/bb_status
  if data.get('status') != 'COMPLETED':
    return False

  return True


def _ValidateBuildbucketResponse(job_info):
  """Checks and validates the response from the buildbucket service for bisect.

  Args:
    job_info: A dictionary containing the response from the buildbucket service.

  Returns:
    True if bisect job is completed successfully and False for pending job.

  Raises:
    BisectJobFailure: When job is completed but build is failed or cancelled.
  """
  job_info = job_info['build']
  json_response = json.dumps(job_info)
  if not job_info:
    raise BisectJobFailure('No response from Buildbucket.')

  if job_info.get('status') in ['SCHEDULED', 'STARTED']:
    return False

  if job_info.get('result') is None:
    raise BisectJobFailure('No "result" in try job results. '
                           'Buildbucket response: %s' % json_response)

  # There are various failure and cancellation reasons for a buildbucket
  # job to fail as listed in https://goto.google.com/bb_status.
  if (job_info.get('status') == 'COMPLETED' and
      job_info.get('result') != 'SUCCESS'):
    reason = (job_info.get('cancelation_reason') or
              job_info.get('failure_reason'))
    raise BisectJobFailure(_BUILD_FAILURE_REASON.get(reason))
  return True
