# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Send alert summary emails to sheriffs on duty."""

import datetime
import logging
import sys

from google.appengine.api import mail

from dashboard import email_template
from dashboard.common import datastore_hooks
from dashboard.common import request_handler
from dashboard.models import anomaly
from dashboard.models import sheriff

# The string to use as the header in an alert summary email.
_EMAIL_HTML_TOTAL_ANOMALIES = """
<br><b>%d total performance regressions were found.</b><br>
<br>
To triage:
"""

_EMAIL_SUBJECT = '%s: %d anomalies found at %d:%d.'


class EmailSummaryHandler(request_handler.RequestHandler):
  """Summarizes alerts and sends e-mail to sheriff on duty.

  Identifies sheriffs who have the "summarize" property set to True, and gets
  anomalies related to that sheriff that were triggered in the past 24 hours.
  """

  def get(self):
    """Emails sheriffs with anomalies identified in most-recent 24 hours."""

    if self.request.get('internal_only') == '1':
      datastore_hooks.SetPrivilegedRequest()
      _QueryAndSendSummaryEmails(True)
    else:
      _QueryAndSendSummaryEmails(False)


def _QueryAndSendSummaryEmails(internal_only):
  # Get all Sheriffs that have requested an e-mail summary.
  sheriffs_to_email_query = sheriff.Sheriff.query()
  sheriffs_to_email_query = sheriffs_to_email_query.filter(
      sheriff.Sheriff.summarize == True)
  sheriffs_to_email_query = sheriffs_to_email_query.filter(
      sheriff.Sheriff.internal_only == internal_only)

  # Start time after which to get anomalies.
  start_time = datetime.datetime.now() - datetime.timedelta(hours=24)

  for sheriff_entity in sheriffs_to_email_query.fetch():
    _SendSummaryEmail(sheriff_entity, start_time)


def _SendSummaryEmail(sheriff_entity, start_time):
  """Sends a summary email for the given sheriff rotation.

  Args:
    sheriff_entity: A Sheriff entity.
    start_time: A starting datetime for anomalies to fetch.
  """
  receivers = email_template.GetSheriffEmails(sheriff_entity)
  anomalies = _RecentUntriagedAnomalies(sheriff_entity, start_time)
  logging.info('_SendSummaryEmail: %s', str(sheriff_entity.key))
  logging.info(' - receivers: %s', str(receivers))
  logging.info(' - anomalies: %d', len(anomalies))
  if not anomalies:
    return
  subject = _EmailSubject(sheriff_entity, anomalies)
  html, text = _EmailBody(anomalies, sheriff_entity.key.string_id())
  logging.info(' - body: %s', text)
  mail.send_mail(
      sender='gasper-alerts@google.com', to=receivers,
      subject=subject, body=text, html=html)


def _RecentUntriagedAnomalies(sheriff_entity, start_time):
  """Returns untriaged anomalies for |sheriff| after |start_time|."""
  recent_anomalies = anomaly.Anomaly.query(
      anomaly.Anomaly.sheriff == sheriff_entity.key,
      anomaly.Anomaly.timestamp > start_time).fetch()
  return [a for a in recent_anomalies
          if not a.is_improvement and a.bug_id is None]


def _EmailSubject(sheriff_entity, anomalies):
  """Returns the email subject string for a summary email."""
  lowest_revision, highest_revision = _MaximalRevisionRange(anomalies)
  return _EMAIL_SUBJECT % (sheriff_entity.key.string_id(), len(anomalies),
                           lowest_revision, highest_revision)


def _MaximalRevisionRange(anomalies):
  """Gets the lowest start and highest end revision for |anomalies|."""
  lowest_revision = sys.maxint
  highest_revision = 1
  for anomaly_entity in anomalies:
    if anomaly_entity.start_revision < lowest_revision:
      lowest_revision = anomaly_entity.start_revision
    if anomaly_entity.end_revision > highest_revision:
      highest_revision = anomaly_entity.end_revision
  return lowest_revision, highest_revision


def _EmailBody(anomalies, sheriff_name):
  """Returns the html and text versions of the email body."""
  assert anomalies
  html_body = []
  text_body = []
  html_body.append(_EMAIL_HTML_TOTAL_ANOMALIES % len(anomalies))

  html_body.append(email_template.GetAlertsLink(sheriff_name))

  # Join details for all anomalies to generate e-mail body.
  html = ''.join(html_body)
  text = ''.join(text_body)
  return html, text
