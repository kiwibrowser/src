# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Manage tree status."""

from __future__ import print_function

import httplib
import json
import os
import re
import socket
import urllib
import urllib2

from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import alerts
from chromite.lib import cros_logging as logging
from chromite.lib import osutils
from chromite.lib import retry_util
from chromite.lib import timeout_util


CROS_TREE_STATUS_URL = 'https://chromiumos-status.appspot.com'
CROS_TREE_STATUS_JSON_URL = '%s/current?format=json' % CROS_TREE_STATUS_URL
CROS_TREE_STATUS_UPDATE_URL = '%s/status' % CROS_TREE_STATUS_URL

_USER_NAME = 'buildbot@chromium.org'
_PASSWORD_PATH = '/home/chrome-bot/.status_password_chromiumos'

_LUCI_MILO_BUILDBOT_URL = 'https://luci-milo.appspot.com/buildbot'
_LEGOLAND_BUILD_URL = ('http://cros-goldeneye/chromeos/healthmonitoring/'
                       'buildDetails?buildbucketId=%(buildbucket_id)s')

# The tree status json file contains the following keywords.
TREE_STATUS_STATE = 'general_state'
TREE_STATUS_USERNAME = 'username'
TREE_STATUS_MESSAGE = 'message'
TREE_STATUS_DATE = 'date'
TREE_STATUS_CAN_COMMIT = 'can_commit_freely'

# These keywords in a status message are detected automatically to
# update the tree status.
MESSAGE_KEYWORDS = ('open', 'throt', 'close', 'maint')

# This is the delimiter to separate messages from different updates.
MESSAGE_DELIMITER = '|'

# Default sleep time (seconds) for waiting for tree status
DEFAULT_WAIT_FOR_TREE_STATUS_SLEEP = 30

# Default timeout (seconds) for waiting for tree status
DEFAULT_WAIT_FOR_TREE_STATUS_TIMEOUT = 60 * 3

# Match EXPERIMENTAL= case-insensitive.
EXPERIMENTAL_BUILDERS_RE = re.compile(r'EXPERIMENTAL=(\S+)', re.IGNORECASE)


class PasswordFileDoesNotExist(Exception):
  """Raised when password file does not exist."""


class InvalidTreeStatus(Exception):
  """Raised when user wants to set an invalid tree status."""


def _GetStatusDict(status_url, raw_message=False):
  """Polls |status_url| and returns the retrieved tree status dictionary.

  This function gets a JSON response from |status_url|, and returns
  the dictionary of the tree status, if one exists and the http
  request was successful.

  The tree status dictionary contains:
    TREE_STATUS_USERNAME: User who posted the message (foo@chromium.org).
    TREE_STATUS_MESSAGE: The status message ("Tree is Open (CQ is good)").
    TREE_STATUS_CAN_COMMIT: Whether tree is commit ready ('true' or 'false').
    TREE_STATUS_STATE: one of constants.VALID_TREE_STATUSES.

  Args:
    status_url: The URL of the tree status to check.
    raw_message: Whether to return the raw message without stripping the
      "Tree is open/throttled/closed" string. Defaults to always strip.

  Returns:
    The tree status as a dictionary, if it was successfully retrieved.
    Otherwise None.
  """
  try:
    # Check for successful response code.
    response = urllib.urlopen(status_url)
    if response.getcode() == 200:
      data = json.load(response)
      if not raw_message:
        # Tree status message is usually in the form:
        #   "Tree is open/closed/throttled (reason for the tree closure)"
        # We want only the reason enclosed in the parentheses.
        # This is a best-effort parsing because user may post the message
        # in a form that we don't recognize.
        match = re.match(r'Tree is [\w\s\.]+\((.*)\)',
                         data.get(TREE_STATUS_MESSAGE, ''))
        data[TREE_STATUS_MESSAGE] = '' if not match else match.group(1)
      return data
  # We remain robust against IOError's.
  except IOError as e:
    logging.error('Could not reach %s: %r', status_url, e)


def _GetStatus(status_url):
  """Polls |status_url| and returns the retrieved tree status.

  This function gets a JSON response from |status_url|, and returns the
  value associated with the TREE_STATUS_STATE, if one exists and the
  http request was successful.

  Returns:
    The tree status, as a string, if it was successfully retrieved. Otherwise
    None.
  """
  status_dict = _GetStatusDict(status_url)
  if status_dict:
    return status_dict.get(TREE_STATUS_STATE)


def WaitForTreeStatus(status_url=None, period=1, timeout=1, throttled_ok=False):
  """Wait for tree status to be open (or throttled, if |throttled_ok|).

  Args:
    status_url: The status url to check i.e.
      'https://status.appspot.com/current?format=json'
    period: How often to poll for status updates.
    timeout: How long to wait until a tree status is discovered.
    throttled_ok: is TREE_THROTTLED an acceptable status?

  Returns:
    The most recent tree status, either constants.TREE_OPEN or
    constants.TREE_THROTTLED (if |throttled_ok|)

  Raises:
    timeout_util.TimeoutError if timeout expired before tree reached
    acceptable status.
  """
  if not status_url:
    status_url = CROS_TREE_STATUS_JSON_URL

  acceptable_states = set([constants.TREE_OPEN])
  verb = 'open'
  if throttled_ok:
    acceptable_states.add(constants.TREE_THROTTLED)
    verb = 'not be closed'

  timeout = max(timeout, 1)

  def _LogMessage(remaining):
    logging.info('Waiting for the tree to %s (%s left)...', verb, remaining)

  def _get_status():
    return _GetStatus(status_url)

  return timeout_util.WaitForReturnValue(
      acceptable_states, _get_status, timeout=timeout,
      period=period, side_effect_func=_LogMessage)


def IsTreeOpen(status_url=None, period=1, timeout=1, throttled_ok=False):
  """Wait for tree status to be open (or throttled, if |throttled_ok|).

  Args:
    status_url: The status url to check i.e.
      'https://status.appspot.com/current?format=json'
    period: How often to poll for status updates.
    timeout: How long to wait until a tree status is discovered.
    throttled_ok: Does TREE_THROTTLED count as open?

  Returns:
    True if the tree is open (or throttled, if |throttled_ok|). False if
    timeout expired before tree reached acceptable status.
  """
  if not status_url:
    status_url = CROS_TREE_STATUS_JSON_URL

  try:
    WaitForTreeStatus(status_url=status_url, period=period, timeout=timeout,
                      throttled_ok=throttled_ok)
  except timeout_util.TimeoutError:
    return False
  return True

def GetExperimentalBuilders(status_url=None, timeout=1):
  """Polls |status_url| and returns the list of experimental builders.

  This function gets a JSON response from |status_url|, and returns the
  list of builders marked as experimental in the tree status' message.

  Args:
    status_url: The status url to check i.e.
      'https://status.appspot.com/current?format=json'
    timeout: How long to wait for the tree status (in seconds).

  Returns:
    A list of strings, where each string is a builder. Returns an empty list if
    there are no experimental builders listed in the tree status.

  Raises:
    TimeoutError if the request takes longer than |timeout| to complete.
  """
  if not status_url:
    status_url = CROS_TREE_STATUS_JSON_URL

  site_config = config_lib.GetConfig()

  @timeout_util.TimeoutDecorator(timeout)
  def _get_status_dict():
    experimental = []
    status_dict = _GetStatusDict(status_url)
    if status_dict:
      for match in EXPERIMENTAL_BUILDERS_RE.findall(
          status_dict.get(TREE_STATUS_MESSAGE)):
        # The value for EXPERIMENTAL= could be a comma-separated list
        # of builders.
        for builder in match.split(','):
          if builder in site_config:
            experimental.append(builder)
          else:
            logging.warning(
                'Got unknown build config "%s" in list of '
                'EXPERIMENTAL-BUILDERS.', builder)

    if experimental:
      logging.info('Got experimental build configs %s from tree status.',
                   experimental)

    return experimental

  return retry_util.GenericRetry(lambda _: True, 3, _get_status_dict, sleep=1)

def _GetPassword():
  """Returns the password for updating tree status."""
  if not os.path.exists(_PASSWORD_PATH):
    raise PasswordFileDoesNotExist(
        'Unable to retrieve password. %s does not exist',
        _PASSWORD_PATH)

  return osutils.ReadFile(_PASSWORD_PATH).strip()


def _UpdateTreeStatus(status_url, message):
  """Updates the tree status to |message|.

  Args:
    status_url: The tree status URL.
    message: The tree status text to post .
  """
  password = _GetPassword()
  params = urllib.urlencode({
      'message': message,
      'username': _USER_NAME,
      'password': password,
  })
  headers = {'Content-Type': 'application/x-www-form-urlencoded'}
  req = urllib2.Request(status_url, data=params, headers=headers)
  try:
    urllib2.urlopen(req)
  except (urllib2.URLError, httplib.HTTPException, socket.error) as e:
    logging.error('Unable to update tree status: %s', e)
    raise e
  else:
    logging.info('Updated tree status with message: %s', message)


def UpdateTreeStatus(status, message, announcer='cbuildbot', epilogue='',
                     status_url=None, dryrun=False):
  """Updates the tree status to |status| with additional |message|.

  Args:
    status: A status in constants.VALID_TREE_STATUSES.
    message: A string to display as part of the tree status.
    announcer: The announcer the message.
    epilogue: The string to append to |message|.
    status_url: The URL of the tree status to update.
    dryrun: If set, don't update the tree status.
  """
  if status_url is None:
    status_url = CROS_TREE_STATUS_UPDATE_URL

  if status not in constants.VALID_TREE_STATUSES:
    raise InvalidTreeStatus('%s is not a valid tree status.' % status)

  if status == 'maintenance':
    # This is a special case because "Tree is maintenance" is
    # grammatically incorrect.
    status = 'under maintenance'

  text_dict = {
      'status': status,
      'epilogue': epilogue,
      'announcer': announcer,
      'message': message,
      'delimiter': MESSAGE_DELIMITER
  }
  if epilogue:
    text = ('Tree is %(status)s (%(announcer)s: %(message)s %(delimiter)s '
            '%(epilogue)s)' % text_dict)
  else:
    text = 'Tree is %(status)s (%(announcer)s: %(message)s)' % text_dict

  if dryrun:
    logging.info('Would have updated the tree status with message: %s', text)
  else:
    _UpdateTreeStatus(status_url, text)


def ThrottleOrCloseTheTree(announcer, message, internal=None, buildnumber=None,
                           dryrun=False):
  """Throttle or close the tree with |message|.

  By default, this function throttles the tree with an updated
  message. If the tree is already not open, it will keep the original
  status (closed, maintenance) and only update the message. This
  ensures that we do not lower the severity of tree closure.

  In the case where the tree is not open, the previous tree status
  message is kept by prepending it to |message|, if possible. This
  ensures that the cause of the previous tree closure remains visible.

  Args:
    announcer: The announcer the message.
    message: A string to display as part of the tree status.
    internal: Whether the build is internal or not. Append the build type
      if this is set. Defaults to None.
    buildnumber: The build number to append.
    dryrun: If set, generate the message but don't update the tree status.
  """
  # Get current tree status.
  status_dict = _GetStatusDict(CROS_TREE_STATUS_JSON_URL)
  current_status = status_dict.get(TREE_STATUS_STATE)
  current_msg = status_dict.get(TREE_STATUS_MESSAGE)

  status = constants.TREE_THROTTLED
  if (constants.VALID_TREE_STATUSES.index(current_status) >
      constants.VALID_TREE_STATUSES.index(status)):
    # Maintain the current status if it is more servere than throttled.
    status = current_status

  epilogue = ''
  # Don't prepend the current status message if the tree is open.
  if current_status != constants.TREE_OPEN and current_msg:
    # Scan the current message and discard the text by the same
    # announcer.
    chunks = [x.strip() for x in current_msg.split(MESSAGE_DELIMITER)
              if '%s' % announcer not in x.strip()]
    current_msg = MESSAGE_DELIMITER.join(chunks)

    if any(x for x in MESSAGE_KEYWORDS if x.lower() in
           current_msg.lower().split()):
      # The waterfall scans the message for keywords to change the
      # tree status. Don't prepend the current status message if it
      # contains such keywords.
      logging.warning('Cannot prepend the previous tree status message because '
                      'there are keywords that may affect the tree state.')
    else:
      epilogue = current_msg

  if internal is not None:
    # 'p' stands for 'public.
    announcer += '-i' if internal else '-p'

  if buildnumber:
    announcer = '%s-%d' % (announcer, buildnumber)

  UpdateTreeStatus(status, message, announcer=announcer, epilogue=epilogue,
                   dryrun=dryrun)


def _OpenSheriffURL(sheriff_url):
  """Returns the content of |sheriff_url| or None if failed to open it."""
  try:
    response = urllib.urlopen(sheriff_url)
    if response.getcode() == 200:
      return response.read()
  except IOError as e:
    logging.error('Could not reach %s: %r', sheriff_url, e)


def GetSheriffEmailAddresses(sheriff_type):
  """Get the email addresses of the sheriffs or deputy.

  Args:
    sheriff_type: Type of the sheriff to look for. See the keys in
    constants.SHERIFF_TYPE_TO_URL.
      - 'tree': tree sheriffs
      - 'chrome': chrome gardener

  Returns:
    A list of email addresses.
  """
  if sheriff_type not in constants.SHERIFF_TYPE_TO_URL:
    raise ValueError('Unknown sheriff type: %s' % sheriff_type)

  urls = constants.SHERIFF_TYPE_TO_URL.get(sheriff_type)
  sheriffs = []
  for url in urls:
    # The URL displays a line: document.write('taco, burrito')
    raw_line = _OpenSheriffURL(url)
    if raw_line is not None:
      match = re.search(r'\'(.*)\'', raw_line)
      if match and match.group(1) != 'None (channel is sheriff)':
        sheriffs.extend(x.strip() for x in match.group(1).split(','))

  return ['%s%s' % (x, constants.GOOGLE_EMAIL) for x in sheriffs]


def GetHealthAlertRecipients(builder_run):
  """Returns a list of email addresses of the health alert recipients."""
  recipients = []
  for entry in builder_run.config.health_alert_recipients:
    if '@' in entry:
      # If the entry is an email address, add it to the list.
      recipients.append(entry)
    else:
      # Perform address lookup for a non-email entry.
      recipients.extend(GetSheriffEmailAddresses(entry))

  return recipients


def SendHealthAlert(builder_run, subject, body, extra_fields=None):
  """Send a health alert.

  Health alerts are only sent for regular buildbots and Pre-CQ buildbots.

  Args:
    builder_run: BuilderRun for the main cbuildbot run.
    subject: The subject of the health alert email.
    body: The body of the health alert email.
    extra_fields: (optional) A dictionary of additional message header fields
                  to be added to the message. Custom field names should begin
                  with the prefix 'X-'.
  """
  if builder_run.InEmailReportingEnvironment():
    server = alerts.GmailServer(
        token_cache_file=constants.GMAIL_TOKEN_CACHE_FILE,
        token_json_file=constants.GMAIL_TOKEN_JSON_FILE)
    alerts.SendEmail(subject,
                     GetHealthAlertRecipients(builder_run),
                     server=server,
                     message=body,
                     extra_fields=extra_fields)


def ConstructLegolandBuildURL(buildbucket_id):
  """Return a Legoland build URL.

  Args:
    buildbucket_id: Buildbucket id of the build to link.

  Returns:
    The fully formed URL.
  """
  return _LEGOLAND_BUILD_URL % {'buildbucket_id': buildbucket_id}


def ConstructDashboardURL(buildbot_master_name, builder_name, build_number):
  """Return the dashboard (luci-milo) URL for this run

  Args:
    buildbot_master_name: Name of buildbot master, e.g. chromeos
    builder_name: Builder name on buildbot dashboard.
    build_number: Build number for this validation attempt.

  Returns:
    The fully formed URL.
  """
  url_suffix = '%s/%s' % (builder_name, str(build_number))
  url_suffix = urllib.quote(url_suffix)
  return os.path.join(
      _LUCI_MILO_BUILDBOT_URL, buildbot_master_name, url_suffix)


# TODO(akeshet): This method still produces links to stage logs as hosted on
# buildbot (rather then the newer replacement, LogDog). We will transition these
# links to point at LogDog at a later date.
def ConstructBuildStageURL(buildbot_url, builder_name, build_number,
                           stage=None):
  """Return the dashboard (buildbot) URL for this run

  Args:
    buildbot_url: Base URL for the waterfall.
    builder_name: Builder name on buildbot dashboard.
    build_number: Build number for this validation attempt.
    stage: Link directly to a stage log, else use the general landing page.

  Returns:
    The fully formed URL.
  """
  url_suffix = 'builders/%s/builds/%s' % (builder_name, str(build_number))
  if stage:
    url_suffix += '/steps/%s/logs/stdio' % (stage,)
  url_suffix = urllib.quote(url_suffix)
  return os.path.join(buildbot_url, url_suffix)


def ConstructViceroyBuildDetailsURL(build_id):
  """Return the dashboard (viceroy) URL for this run.

  Args:
    build_id: CIDB id for the master build.

  Returns:
    The fully formed URL.
  """
  _link = ('https://viceroy.corp.google.com/'
           'chromeos/build_details?build_id=%(build_id)s')
  return _link % {'build_id': build_id}


def ConstructGoldenEyeSuiteDetailsURL(job_id=None, build_id=None):
  """Return the dashboard (goldeneye) URL of suite details for job or build.

  Args:
    job_id: AFE job id.
    build_id: CIDB id for the master build.

  Returns:
    The fully formed URL.
  """
  if job_id is None and build_id is None:
    return None
  _link = 'http://cros-goldeneye/healthmonitoring/suiteDetails?'
  if job_id:
    return _link + 'suiteId=%d' % int(job_id)
  else:
    return _link + 'cidbBuildId=%d' % int(build_id)


def ConstructGoldenEyeBuildDetailsURL(build_id):
  """Return the dashboard (goldeneye) URL for this run.

  Args:
    build_id: CIDB id for the build.

  Returns:
    The fully formed URL.
  """
  _link = ('http://go/goldeneye/'
           'chromeos/healthmonitoring/buildDetails?id=%(build_id)s')
  return _link % {'build_id': build_id}


def ConstructAnnotatorURL(build_id):
  """Return the build annotator URL for this run.

  Args:
    build_id: CIDB id for the master build.

  Returns:
    The fully formed URL.
  """
  _link = ('https://chromiumos-build-annotator.googleplex.com/'
           'build_annotations/edit_annotations/master-paladin/%(build_id)s/?')
  return _link % {'build_id': build_id}
