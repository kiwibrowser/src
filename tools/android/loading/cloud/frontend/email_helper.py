# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from google.appengine.api import (mail, users)


def GetUserEmail():
  """Returns the email address of the user currently making the request or None.
  """
  user = users.get_current_user()
  if user:
    return user.email()
  return None


def SendEmailTaskComplete(to_address, tag, status, task_url, logger):
  """Sends an email to to_address notifying that the task identified by tag is
  complete.

  Args:
    to_address (str): The email address to notify.
    tag (str): The tag of the task.
    status (str): Status of the task.
    task_url (str): URL where the results of the task can be found.
    logger (logging.logger): Used for logging.
  """
  if not to_address:
    logger.error('No email address to notify for task ' + tag)
    return

  logger.info('Notify task %s complete to %s.' % (tag, to_address))
  # The sender address must be in the "Email API authorized senders", configured
  # in the Application Settings of AppEngine.
  sender_address = 'clovis-noreply@google.com'
  subject = 'Task %s complete' % tag
  body = 'Your Clovis task %s is now complete with status: %s.' % (tag, status)
  if task_url:
    body += '\nCheck the results at ' + task_url
  mail.send_mail(sender=sender_address, to=to_address, subject=subject,
                 body=body)

