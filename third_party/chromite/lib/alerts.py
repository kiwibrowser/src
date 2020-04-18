# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Chromite email utility functions."""

from __future__ import print_function

import base64
import collections
import cStringIO
import gzip
import json
import os
import smtplib
import socket
import sys
import traceback

from email.mime.application import MIMEApplication
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText

from chromite.lib import cros_logging as logging
from chromite.lib import retry_util

# TODO(fdeng): Cleanup the try-catch once crbug.com/482063 is fixed.
try:
  import httplib
  import httplib2
  from apiclient.discovery import build as apiclient_build
  from apiclient import errors as apiclient_errors
  from oauth2client import file as oauth_client_fileio
  from oauth2client import client
except (RuntimeError, ImportError) as e:
  apiclient_build = None
  oauth_client_fileio = None


class MailServer(object):
  """Base class for servers."""

  def Send(self, message):
    """Send the message.

    Override by sub-classes.

    Args:
      message: A MIMEMultipart() object containing the body of the message.

    Returns:
      True if the email was sent, else False.
    """
    raise NotImplementedError('Should be implemented by sub-classes.')


class AuthenticationError(Exception):
  """Error raised when authenticating via oauth2."""


# Represent token in oauth2 token file.
RefreshToken = collections.namedtuple('RefreshToken', (
    'client_id',
    'client_secret',
    'refresh_token',
))


def ReadRefreshTokenJson(path):
  """Returns RefreshToken by reading it from the JSON file.

  Args:
    path: Path to the json file that contains the credential tokens.

  Returns:
    A RefreshToken object.

  Raises:
    AuthenticationError if failed to read from json file.
  """
  try:
    with open(path, 'r') as f:
      data = json.load(f)
      return RefreshToken(
          client_id=str(data['client_id']),
          client_secret=str(data['client_secret']),
          refresh_token=str(data['refresh_token']))
  except (IOError, ValueError) as e:
    raise AuthenticationError(
        'Failed to read refresh token from %s: %s' % (path, e))
  except KeyError as e:
    raise AuthenticationError(
        'Failed to read refresh token from %s: missing key %s' % (path, e))


class GmailServer(MailServer):
  """Gmail server."""

  TOKEN_URI = 'https://accounts.google.com/o/oauth2/token'


  def __init__(self, token_cache_file, token_json_file=None):
    """Initialize GmailServer.

    If token_cache_file contains valid credentials, it will be used.
    If not or the file doesn't exist, will try to load tokens
    from token_json_file. The loaded credentials will be stored to the
    cache file.

    Args:
      token_cache_file: Absolute path to gmail credentials cache file.
      token_json_file: Absolute path to a json file that contains
                       refresh token for gmail.
    """
    self._token_cache_file = token_cache_file
    self._token_json_file = token_json_file

  def _GetCachedCredentials(self):
    """Get credentials from cached file or json file.

    Returns:
      OAuth2Credentials object.

    Raises:
      AuthenticationError on failure to read json file.
    """
    storage = oauth_client_fileio.Storage(self._token_cache_file)
    # Try loading credentials from existing token cache file.
    if os.path.isfile(self._token_cache_file):
      credentials = storage.get()
      if credentials and not credentials.invalid:
        return credentials

    if self._token_json_file is None:
      raise AuthenticationError('Gmail token file path is not provided.')

    # Create new credentials if cache file doesn't exist or not valid.
    refresh_token_json = ReadRefreshTokenJson(self._token_json_file)
    credentials = client.OAuth2Credentials(
        access_token=None,
        client_id=refresh_token_json.client_id,
        client_secret=refresh_token_json.client_secret,
        refresh_token=refresh_token_json.refresh_token,
        token_expiry=None,
        token_uri=self.TOKEN_URI,
        user_agent=None,
        revoke_uri=None)
    credentials.set_store(storage)
    storage.put(credentials)
    return credentials

  def Send(self, message):
    """Send an e-mail via Gmail API.

    Args:
      message: A MIMEMultipart() object containing the body of the message.

    Returns:
      True if the email was sent, else False.
    """
    if not apiclient_build:
      logging.warning('Could not send email: Google API client not installed.')
      return False

    try:
      credentials = self._GetCachedCredentials()
    except AuthenticationError as e:
      logging.warning('Could not get gmail credentials: %s', e)
      return False

    http = credentials.authorize(httplib2.Http())
    service = apiclient_build('gmail', 'v1', http=http)
    try:
      # 'me' represents the default authorized user.
      payload = {'raw': base64.urlsafe_b64encode(message.as_string())}
      service.users().messages().send(userId='me', body=payload).execute()
      return True
    except (apiclient_errors.HttpError, httplib.HTTPException,
            client.Error) as error:
      logging.warning('Could not send email: %s', error)
      return False


class SmtpServer(MailServer):
  """Smtp server."""

  # Note: When importing this module from cbuildbot code that will run on
  # a builder in the golo, set this to constants.GOLO_SMTP_SERVER
  DEFAULT_SERVER = 'localhost'
  # Retry parameters for the actual smtp connection.
  SMTP_RETRY_COUNT = 3
  SMTP_RETRY_DELAY = 30

  def __init__(self, smtp_server=None):
    """Initialize SmtpServer.

    Args:
      smtp_server: The server with which to send the message.
    """
    self._smtp_server = smtp_server or self.DEFAULT_SERVER

  def Send(self, message):
    """Send an email via SMTP

    If we get a socket error (e.g. the SMTP server is not listening or
    timesout), we will retry a few times.  All socket errors will be
    caught here.

    Args:
      message: A MIMEMultipart() object containing the body of the message.

    Returns:
      True if the email was sent, else False.
    """
    def _Send():
      smtp_client = smtplib.SMTP(self._smtp_server)
      recipients = [s.strip() for s in message['To'].split(',')]
      smtp_client.sendmail(message['From'], recipients, message.as_string())
      smtp_client.quit()

    try:
      retry_util.RetryException(socket.error, self.SMTP_RETRY_COUNT, _Send,
                                sleep=self.SMTP_RETRY_DELAY)
      return True
    except socket.error as e:
      logging.warning('Could not send e-mail from %s to %s via %r: %s',
                      message['From'], message['To'], self._smtp_server, e)
      return False


def CreateEmail(subject, recipients, message='', attachment=None,
                extra_fields=None):
  """Create an email message object.

  Args:
    subject: E-mail subject.
    recipients: List of e-mail recipients.
    message: (optional) Message to put in the e-mail body.
    attachment: (optional) text to attach.
    extra_fields: (optional) A dictionary of additional message header fields
                  to be added to the message. Custom field names should begin
                  with the prefix 'X-'.

  Returns:
    A MIMEMultipart object, or None if recipients is empty.
  """
  # Ignore if the list of recipients is empty.
  if not recipients:
    logging.warning('Could not create email: recipient list is emtpy.')
    return None

  extra_fields = extra_fields or {}
  sender = socket.getfqdn()
  msg = MIMEMultipart()
  for key, val in extra_fields.iteritems():
    msg[key] = val
  msg['From'] = sender
  msg['Subject'] = subject
  msg['To'] = ', '.join(recipients)

  msg.attach(MIMEText(message))
  if attachment:
    s = cStringIO.StringIO()
    with gzip.GzipFile(fileobj=s, mode='w') as f:
      f.write(attachment)
    part = MIMEApplication(s.getvalue(), _subtype='x-gzip')
    s.close()
    part.add_header('Content-Disposition', 'attachment', filename='logs.txt.gz')
    msg.attach(part)

  return msg


def SendEmail(subject, recipients, server=None, message='',
              attachment=None, extra_fields=None):
  """Send an e-mail job notification with the given message in the body.

  Args:
    subject: E-mail subject.
    recipients: List of e-mail recipients.
    server: A MailServer instance. Default to local SmtpServer.
    message: Message to put in the e-mail body.
    attachment: Text to attach.
    extra_fields: A dictionary of additional message header fields
                  to be added to the message. Custom field names should begin
                  with the prefix 'X-'.
  """
  if server is None:
    server = SmtpServer()
  msg = CreateEmail(subject, recipients, message, attachment, extra_fields)
  if not msg:
    return
  server.Send(msg)


def SendEmailLog(subject, recipients, server=None, message='',
                 inc_trace=True, log=None, extra_fields=None):
  """Send an e-mail with a stack trace and log snippets.

  Args:
    subject: E-mail subject.
    recipients: list of e-mail recipients.
    server: A MailServer instance. Default to local SmtpServer.
    inc_trace: Append a backtrace of the current stack.
    message: Message to put at the top of the e-mail body.
    log: List of lines (log data) to include in the notice.
    extra_fields: (optional) A dictionary of additional message header fields
                  to be added to the message. Custom fields names should begin
                  with the prefix 'X-'.
  """
  if server is None:
    server = SmtpServer()
  if not message:
    message = subject
  message = message[:]

  if inc_trace:
    if sys.exc_info() != (None, None, None):
      trace = traceback.format_exc()
      message += '\n\n' + trace

  attachment = None
  if log:
    message += ('\n\n' +
                '***************************\n' +
                'Last log messages:\n' +
                '***************************\n' +
                ''.join(log[-50:]))
    attachment = ''.join(log)

  SendEmail(subject, recipients, server, message=message,
            attachment=attachment, extra_fields=extra_fields)
