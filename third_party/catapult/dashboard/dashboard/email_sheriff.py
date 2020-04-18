# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Provides a function for emailing an alert to a sheriff on duty."""

import logging

from google.appengine.api import mail

from dashboard import email_template


def EmailSheriff(sheriff, test, anomaly):
  """Sends an email to the sheriff on duty about the given anomaly.

  Args:
    sheriff: sheriff.Sheriff entity.
    test: The graph_data.TestMetadata entity associated with the anomaly.
    anomaly: The anomaly.Anomaly entity.
  """
  receivers = email_template.GetSheriffEmails(sheriff)
  if not receivers:
    logging.warn('No email address for %s', sheriff)
    return
  anomaly_info = email_template.GetAlertInfo(anomaly, test)
  mail.send_mail(sender='gasper-alerts@google.com',
                 to=receivers,
                 subject=anomaly_info['email_subject'],
                 body=anomaly_info['email_text'],
                 html=anomaly_info['email_html'] + anomaly_info['alerts_link'])
  logging.info('Sent single mail to %s', receivers)
