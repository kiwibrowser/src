# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""URL endpoint for a cron job to automatically mark alerts which recovered."""

import logging

from google.appengine.api import taskqueue
from google.appengine.ext import ndb

from dashboard import find_anomalies
from dashboard.common import math_utils
from dashboard.common import request_handler
from dashboard.common import utils
from dashboard.common import datastore_hooks
from dashboard.models import anomaly
from dashboard.models import anomaly_config
from dashboard.models import sheriff
from dashboard.services import issue_tracker_service

_TASK_QUEUE_NAME = 'auto-triage-queue'

_MAX_UNTRIAGED_ANOMALIES = 1000

# Maximum relative difference between two steps for them to be considered
# similar enough for the second to be a "recovery" of the first.
# For example, if there's an increase of 5 units followed by a decrease of 6
# units, the relative difference of the deltas is 0.2.
_MAX_DELTA_DIFFERENCE = 0.25


class MarkRecoveredAlertsHandler(request_handler.RequestHandler):
  """URL endpoint for a cron job to automatically triage anomalies and bugs."""

  def get(self):
    """A get request is the same a post request for this endpoint."""
    self.post()

  def post(self):
    """Checks if alerts have recovered, and marks them if so.

    This includes checking untriaged alerts, as well as alerts associated with
    open bugs..
    """
    datastore_hooks.SetPrivilegedRequest()

    # Handle task queue requests.
    bug_id = self.request.get('bug_id')
    if bug_id:
      bug_id = int(bug_id)
    if self.request.get('check_alert'):
      self.MarkAlertAndBugIfRecovered(self.request.get('alert_key'), bug_id)
      return
    if self.request.get('check_bug'):
      self.CheckRecoveredAlertsForBug(bug_id)
      return

    # Kick off task queue jobs for untriaged anomalies.
    alerts = self._FetchUntriagedAnomalies()
    logging.info('Kicking off tasks for %d alerts', len(alerts))
    for alert in alerts:
      taskqueue.add(
          url='/mark_recovered_alerts',
          params={'check_alert': 1, 'alert_key': alert.urlsafe()},
          queue_name=_TASK_QUEUE_NAME)

    # Kick off task queue jobs for open bugs.
    bugs = self._FetchOpenBugs()
    logging.info('Kicking off tasks for %d bugs', len(bugs))
    for bug in bugs:
      taskqueue.add(
          url='/mark_recovered_alerts',
          params={'check_bug': 1, 'bug_id': bug['id']},
          queue_name=_TASK_QUEUE_NAME)

  def _FetchUntriagedAnomalies(self):
    """Fetches recent untriaged anomalies asynchronously from all sheriffs."""
    anomalies = []
    futures = []
    sheriff_keys = sheriff.Sheriff.query().fetch(keys_only=True)

    for key in sheriff_keys:
      query = anomaly.Anomaly.query(
          anomaly.Anomaly.sheriff == key,
          anomaly.Anomaly.bug_id == None,
          anomaly.Anomaly.is_improvement == False,
          anomaly.Anomaly.recovered == False)
      query = query.order(-anomaly.Anomaly.timestamp)
      futures.append(
          query.fetch_async(limit=_MAX_UNTRIAGED_ANOMALIES, keys_only=True))
    ndb.Future.wait_all(futures)
    for future in futures:
      anomalies.extend(future.get_result())
    return anomalies

  def _FetchOpenBugs(self):
    """Fetches a list of open bugs on all sheriffing labels."""
    issue_tracker = issue_tracker_service.IssueTrackerService(
        utils.ServiceAccountHttp())
    bugs = issue_tracker.List(
        can='open',
        q='Performance=Sheriff OR Performance=Sheriff-V8',
        maxResults=1000)
    return bugs['items']

  def MarkAlertAndBugIfRecovered(self, alert_key_urlsafe, bug_id):
    """Checks whether an alert has recovered, and marks it if so.

    An alert will be considered "recovered" if there's a change point in
    the series after it with roughly equal magnitude and opposite direction.

    Args:
      alert_key_urlsafe: The original regression Anomaly.

    Returns:
      True if the Anomaly should be marked as recovered, False otherwise.
    """
    alert_entity = ndb.Key(urlsafe=alert_key_urlsafe).get()
    logging.info('Checking alert %s', alert_entity)
    if not self._IsAlertRecovered(alert_entity):
      return

    logging.info('Recovered')
    alert_entity.recovered = True
    alert_entity.put()
    if bug_id:
      unrecovered = self._GetUnrecoveredAlertsForBug(bug_id)
      if not unrecovered:
        # All alerts recovered! Update bug.
        logging.info('All alerts for bug %s recovered!', bug_id)
        comment = 'Automatic message: All alerts recovered.\nGraphs: %s' % (
            'https://chromeperf.appspot.com/group_report?bug_id=%s' % bug_id)
        issue_tracker = issue_tracker_service.IssueTrackerService(
            utils.ServiceAccountHttp())
        issue_tracker.AddBugComment(
            bug_id, comment, labels='Performance-Regression-Recovered')


  def _IsAlertRecovered(self, alert_entity):
    test = alert_entity.GetTestMetadataKey().get()
    if not test:
      logging.error('TestMetadata %s not found for Anomaly %s, deleting test.',
                    utils.TestPath(alert_entity.GetTestMetadataKey()),
                    alert_entity)
      return False
    config = anomaly_config.GetAnomalyConfigDict(test)
    max_num_rows = config.get(
        'max_window_size', find_anomalies.DEFAULT_NUM_POINTS)
    rows = [r for r in find_anomalies.GetRowsToAnalyze(test, max_num_rows)
            if r.revision > alert_entity.end_revision]
    change_points = find_anomalies.FindChangePointsForTest(rows, config)
    delta_anomaly = (alert_entity.median_after_anomaly -
                     alert_entity.median_before_anomaly)
    for change in change_points:
      delta_change = change.median_after - change.median_before
      if (self._IsOppositeDirection(delta_anomaly, delta_change) and
          self._IsApproximatelyEqual(delta_anomaly, -delta_change)):
        logging.debug('Anomaly %s recovered; recovery change point %s.',
                      alert_entity.key, change.AsDict())
        return True
    return False

  def _IsOppositeDirection(self, delta1, delta2):
    return delta1 * delta2 < 0

  def _IsApproximatelyEqual(self, delta1, delta2):
    smaller = min(delta1, delta2)
    larger = max(delta1, delta2)
    return math_utils.RelativeChange(smaller, larger) <= _MAX_DELTA_DIFFERENCE

  def CheckRecoveredAlertsForBug(self, bug_id):
    unrecovered = self._GetUnrecoveredAlertsForBug(bug_id)
    logging.info('Queueing %d alerts for bug %s', len(unrecovered), bug_id)
    for alert in unrecovered:
      taskqueue.add(
          url='/mark_recovered_alerts',
          params={
              'check_alert': 1,
              'alert_key': alert.key.urlsafe(),
              'bug_id': bug_id},
          queue_name=_TASK_QUEUE_NAME)

  def _GetUnrecoveredAlertsForBug(self, bug_id):
    alerts = anomaly.Anomaly.query(anomaly.Anomaly.bug_id == bug_id).fetch()
    unrecovered = [a for a in alerts if not a.recovered]
    return unrecovered
