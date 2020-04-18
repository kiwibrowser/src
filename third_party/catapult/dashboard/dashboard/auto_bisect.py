# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""URL endpoint for a cron job to automatically run bisects."""

import logging

from dashboard import can_bisect
from dashboard import pinpoint_request
from dashboard import start_try_job
from dashboard.common import namespaced_stored_object
from dashboard.common import request_handler
from dashboard.common import utils
from dashboard.models import anomaly
from dashboard.models import graph_data
from dashboard.models import try_job
from dashboard.services import pinpoint_service


class NotBisectableError(Exception):
  """An error indicating that a bisect couldn't be automatically started."""
  pass


def StartNewBisectForBug(bug_id):
  """Tries to trigger a bisect job for the alerts associated with a bug.

  Args:
    bug_id: A bug ID number.

  Returns:
    If successful, a dict containing "issue_id" and "issue_url" for the
    bisect job. Otherwise, a dict containing "error", with some description
    of the reason why a job wasn't started.
  """
  try:
    return _StartBisectForBug(bug_id)
  except NotBisectableError as e:
    logging.info('New bisect errored out with message: ' + e.message)
    return {'error': e.message}


def _StartBisectForBug(bug_id):
  anomalies = anomaly.Anomaly.query(anomaly.Anomaly.bug_id == bug_id).fetch()
  if not anomalies:
    raise NotBisectableError('No Anomaly alerts found for this bug.')

  test_anomaly = _ChooseTest(anomalies)
  test = None
  if test_anomaly:
    test = test_anomaly.GetTestMetadataKey().get()
  if not test or not can_bisect.IsValidTestForBisect(test.test_path):
    raise NotBisectableError('Could not select a test.')

  bot_configurations = namespaced_stored_object.Get('bot_configurations')

  if test.bot_name in bot_configurations.keys():
    return _StartPinpointBisect(bug_id, test_anomaly, test)

  return _StartRecipeBisect(bug_id, test_anomaly, test)


def _StartPinpointBisect(bug_id, test_anomaly, test):
  # Convert params to Pinpoint compatible
  params = {
      'test_path': test.test_path,
      'start_commit': test_anomaly.start_revision - 1,
      'end_commit': test_anomaly.end_revision,
      'bug_id': bug_id,
      'bisect_mode': 'performance',
      'story_filter': start_try_job.GuessStoryFilter(test.test_path),
  }

  try:
    results = pinpoint_service.NewJob(
        pinpoint_request.PinpointParamsFromBisectParams(params))
  except pinpoint_request.InvalidParamsError as e:
    raise NotBisectableError(e.message)

  # For compatibility with existing bisect, switch these to issueId/url
  if 'jobId' in results:
    results['issue_id'] = results['jobId']
    test_anomaly.pinpoint_bisects.append(str(results['jobId']))
    test_anomaly.put()
    del results['jobId']

  if 'jobUrl' in results:
    results['issue_url'] = results['jobUrl']
    del results['jobUrl']

  return results


def _StartRecipeBisect(bug_id, test_anomaly, test):
  bisect_job = _MakeBisectTryJob(bug_id, test_anomaly, test)
  bisect_job_key = bisect_job.put()

  try:
    bisect_result = start_try_job.PerformBisect(bisect_job)
  except request_handler.InvalidInputError as e:
    bisect_result = {'error': e.message}
  if 'error' in bisect_result:
    bisect_job_key.delete()
  return bisect_result


def _MakeBisectTryJob(bug_id, test_anomaly, test):
  """Tries to automatically select parameters for a bisect job.

  Args:
    bug_id: A bug ID which some alerts are associated with.

  Returns:
    A TryJob entity, which has not yet been put in the datastore.

  Raises:
    NotBisectableError: A valid bisect config could not be created.
  """
  good_revision = GetRevisionForBisect(test_anomaly.start_revision - 1, test)
  bad_revision = GetRevisionForBisect(test_anomaly.end_revision, test)
  if not can_bisect.IsValidRevisionForBisect(good_revision):
    raise NotBisectableError('Invalid "good" revision: %s.' % good_revision)
  if not can_bisect.IsValidRevisionForBisect(bad_revision):
    raise NotBisectableError('Invalid "bad" revision: %s.' % bad_revision)
  if test_anomaly.start_revision == test_anomaly.end_revision:
    raise NotBisectableError(
        'Same "good"/"bad" revisions, bisect skipped')

  metric = start_try_job.GuessMetric(test.test_path)
  story_filter = start_try_job.GuessStoryFilter(test.test_path)

  bisect_bot = start_try_job.GuessBisectBot(test.master_name, test.bot_name)
  if not bisect_bot:
    raise NotBisectableError(
        'Could not select a bisect bot: %s for (%s, %s)' % (
            bisect_bot, test.master_name, test.bot_name))

  new_bisect_config = start_try_job.GetBisectConfig(
      bisect_bot=bisect_bot,
      master_name=test.master_name,
      suite=test.suite_name,
      metric=metric,
      story_filter=story_filter,
      good_revision=good_revision,
      bad_revision=bad_revision,
      repeat_count=10,
      max_time_minutes=20,
      bug_id=bug_id)

  if 'error' in new_bisect_config:
    raise NotBisectableError('Could not make a valid config.')

  config_python_string = utils.BisectConfigPythonString(new_bisect_config)

  bisect_job = try_job.TryJob(
      bot=bisect_bot,
      config=config_python_string,
      bug_id=bug_id,
      master_name=test.master_name,
      internal_only=test.internal_only,
      job_type='bisect')

  return bisect_job


def _ChooseTest(anomalies):
  """Chooses a test to use for a bisect job.

  The particular TestMetadata chosen determines the command and metric name that
  is chosen. The test to choose could depend on which of the anomalies has the
  largest regression size.

  Ideally, the choice of bisect bot to use should be based on bisect bot queue
  length, and the choice of metric should be based on regression size and noise
  level.

  However, we don't choose bisect bot and metric independently, since some
  regressions only happen for some tests on some platforms; we should generally
  only bisect with a given bisect bot on a given metric if we know that the
  regression showed up on that platform for that metric.

  Args:
    anomalies: A non-empty list of Anomaly entities.

  Returns:
    An Anomaly entity, or None if no valid entity could be chosen.
  """
  if not anomalies:
    return None
  anomalies.sort(cmp=_CompareAnomalyBisectability)
  for anomaly_entity in anomalies:
    if can_bisect.IsValidTestForBisect(
        utils.TestPath(anomaly_entity.GetTestMetadataKey())):
      return anomaly_entity
  return None


def _CompareAnomalyBisectability(a1, a2):
  """Compares two Anomalies to decide which Anomaly's TestMetadata is better to
     use.

  Note: Potentially, this could be made more sophisticated by using
  more signals:
   - Bisect bot queue length
   - Platform
   - Test run time
   - Stddev of test

  Args:
    a1: The first Anomaly entity.
    a2: The second Anomaly entity.

  Returns:
    Negative integer if a1 is better than a2, positive integer if a2 is better
    than a1, or zero if they're equally good.
  """
  if a1.percent_changed > a2.percent_changed:
    return -1
  elif a1.percent_changed < a2.percent_changed:
    return 1
  return 0


def GetRevisionForBisect(revision, test_key):
  """Gets a start or end revision value which can be used when bisecting.

  Note: This logic is parallel to that in elements/chart-container.html
  in the method getRevisionForBisect.

  Args:
    revision: The ID of a Row, not necessarily an actual revision number.
    test_key: The ndb.Key for a TestMetadata.

  Returns:
    An int or string value which can be used when bisecting.
  """
  row_parent_key = utils.GetTestContainerKey(test_key)
  row = graph_data.Row.get_by_id(revision, parent=row_parent_key)
  if row and hasattr(row, 'a_default_rev') and hasattr(row, row.a_default_rev):
    return getattr(row, row.a_default_rev)
  return revision


def _PrintStartedAndFailedBisectJobs():
  """Prints started and failed bisect jobs in datastore."""
  failed_jobs = try_job.TryJob.query(
      try_job.TryJob.status == 'failed').fetch()
  started_jobs = try_job.TryJob.query(
      try_job.TryJob.status == 'started').fetch()

  return {
      'headline': 'Bisect Jobs',
      'results': [
          _JobsListResult('Failed jobs', failed_jobs),
          _JobsListResult('Started jobs', started_jobs),
      ]
  }


def _JobsListResult(title, jobs):
  """Returns one item in a list of results to be displayed on result.html."""
  return {
      'name': '%s: %d' % (title, len(jobs)),
      'value': '\n'.join(_JobLine(job) for job in jobs),
      'class': 'results-pre'
  }


def _JobLine(job):
  """Returns a string with information about one TryJob entity."""
  config = job.config.replace('\n', '') if job.config else 'No config.'
  return 'Bug ID %d. %s' % (job.bug_id, config)
