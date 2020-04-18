# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module that generates and sends CL validation messages."""

from __future__ import print_function

from chromite.lib import constants
from chromite.lib import cros_logging as logging
from chromite.lib import gob_util
from chromite.lib import patch as cros_patch


def CreateValidationFailureMessage(pre_cq_trybot, change, suspects, messages,
                                   sanity=True, infra_fail=False,
                                   lab_fail=False, no_stat=None,
                                   retry=False, cl_status_url=None):
  """Create a message explaining why a validation failure occurred.

  Args:
    pre_cq_trybot: Whether the builder is a Pre-CQ trybot. (Note: The Pre-CQ
      launcher is NOT considered a Pre-CQ trybot.)
    change: The change we want to create a message for.
    suspects: An instance of triage_lib.SuspectChanges.
    messages: A list of build failure messages from supporting builders.
      These must be BuildFailureMessage objects or NoneType objects.
    sanity: A boolean indicating whether the build was considered sane. If
      not sane, none of the changes will have their CommitReady bit modified.
    infra_fail: The build failed purely due to infrastructure failures.
    lab_fail: The build failed purely due to test lab infrastructure failures.
    no_stat: A list of builders which failed prematurely without reporting
      status.
    retry: Whether we should retry automatically.
    cl_status_url: URL of the CL status viewer for the change.

  Returns:
    A string that communicates what happened.
  """
  msg = []
  if no_stat:
    msg.append('The following build(s) did not start or failed prematurely:')
    msg.append(', '.join(no_stat))

  if messages:
    # Build a list of error messages. We don't want to build a ridiculously
    # long comment, as Gerrit will reject it. See http://crbug.com/236831
    max_error_len = 20000 / max(1, len(messages))
    msg.append('The following build(s) failed:')
    for message in map(str, messages):
      if len(message) > max_error_len:
        message = message[:max_error_len] + '... (truncated)'
      msg.append(message)

  # Create a list of changes other than this one that might be guilty.
  # Limit the number of suspects to 20 so that the list of suspects isn't
  # ridiculously long.
  max_suspects = 20
  other_suspects = set(suspects.keys()) - set([change])
  if len(other_suspects) < max_suspects:
    other_suspects_str = cros_patch.GetChangesAsString(other_suspects)
  else:
    other_suspects_str = ('%d other changes. See the blamelist for more '
                          'details.' % (len(other_suspects),))

  if not sanity:
    msg.append('The build was consider not sane because the sanity check '
               'builder(s) failed. Your change will not be blamed for the '
               'failure.')
    assert retry
  elif lab_fail:
    msg.append('The build encountered Chrome OS Lab infrastructure issues. '
               ' Your change will not be blamed for the failure.')
    assert retry
  else:
    if infra_fail:
      msg.append('The build failure may have been caused by infrastructure '
                 'issues and/or bad %s changes.' % constants.INFRA_PROJECTS)

    if change in suspects.keys():
      if other_suspects_str:
        msg.append('Your change may have caused this failure. There are '
                   'also other changes that may be at fault: %s'
                   % other_suspects_str)
      else:
        msg.append('This failure was probably caused by your change.')

        msg.append('Please check whether the failure is your fault. If your '
                   'change is not at fault, you may mark it as ready again.')
    else:
      if len(suspects) == 1:
        msg.append('This failure was probably caused by %s'
                   % other_suspects_str)
      elif len(suspects) > 0:
        msg.append('One of the following changes is probably at fault: %s'
                   % other_suspects_str)

      assert retry

  if pre_cq_trybot and cl_status_url:
    msg.append(
        'We notify the first failure only. Please find the full status at %s.'
        % cl_status_url)

  if retry:
    bot = 'The Pre-Commit Queue' if pre_cq_trybot else 'The Commit Queue'
    msg.insert(0, 'NOTE: %s will retry your change automatically.' % bot)

  return '\n\n'.join(msg)


class PaladinMessage(object):
  """Object used to send messages to developers about their changes."""

  # URL where Paladin documentation is stored.
  _PALADIN_DOCUMENTATION_URL = ('http://www.chromium.org/developers/'
                                'tree-sheriffs/sheriff-details-chromium-os/'
                                'commit-queue-overview')

  # Gerrit can't handle commands over 32768 bytes. See http://crbug.com/236831
  MAX_MESSAGE_LEN = 32000

  def __init__(self, message, patch, helper):
    if len(message) > self.MAX_MESSAGE_LEN:
      message = message[:self.MAX_MESSAGE_LEN] + '... (truncated)'
    self.message = message
    self.patch = patch
    self.helper = helper

  def _ConstructPaladinMessage(self):
    """Adds any standard Paladin messaging to an existing message."""
    return self.message + ('\n\nCommit queue documentation: %s' %
                           self._PALADIN_DOCUMENTATION_URL)

  def Send(self, dryrun):
    """Posts a comment to a gerrit review."""
    body = {
        'message': self._ConstructPaladinMessage(),
        'notify': 'OWNER',
    }
    path = 'changes/%s/revisions/%s/review' % (
        self.patch.gerrit_number, self.patch.revision)
    if dryrun:
      logging.info('Would have sent %r to %s', body, path)
      return
    gob_util.FetchUrl(self.helper.host, path, reqtype='POST', body=body)
