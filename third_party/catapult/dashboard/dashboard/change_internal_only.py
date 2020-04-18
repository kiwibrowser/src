# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Provides the web interface for changing internal_only property of a Bot."""

import logging

from google.appengine.api import taskqueue
from google.appengine.datastore import datastore_query
from google.appengine.ext import ndb

from dashboard import add_point_queue
from dashboard.common import request_handler
from dashboard.common import datastore_hooks
from dashboard.common import stored_object
from dashboard.models import anomaly
from dashboard.models import graph_data

# Number of Row entities to process at once.
_MAX_ROWS_TO_PUT = 25

# Number of TestMetadata entities to process at once.
_MAX_TESTS_TO_PUT = 25

# Which queue to use for tasks started by this handler. Must be in queue.yaml.
_QUEUE_NAME = 'migrate-queue'


class ChangeInternalOnlyHandler(request_handler.RequestHandler):
  """Changes internal_only property of Bot, TestMetadata, and Row."""

  def get(self):
    """Renders the UI for selecting bots."""
    masters = {}
    bots = graph_data.Bot.query().fetch()
    for bot in bots:
      master_name = bot.key.parent().string_id()
      bot_name = bot.key.string_id()
      bots = masters.setdefault(master_name, [])
      bots.append({
          'name': bot_name,
          'internal_only': bot.internal_only,
      })
    logging.info('MASTERS: %s', masters)

    self.RenderHtml('change_internal_only.html', {
        'masters': masters,
    })

  def post(self):
    """Updates the selected bots internal_only property.

    POST requests will be made by the task queue; tasks are added to the task
    queue either by a kick-off POST from the front-end form, or by this handler
    itself.

    Request parameters:
      internal_only: "true" if turning on internal_only, else "false".
      bots: Bots to update. Multiple bots parameters are possible; the value
          of each should be a string like "MasterName/platform-name".
      test: An urlsafe Key for a TestMetadata entity.
      cursor: An urlsafe Cursor; this parameter is only given if we're part-way
          through processing a Bot or a TestMetadata.

    Outputs:
      A message to the user if this request was started by the web form,
      or an error message if something went wrong, or nothing.
    """
    # /change_internal_only should be only accessible if one has administrator
    # privileges, so requests are guaranteed to be authorized.
    datastore_hooks.SetPrivilegedRequest()

    internal_only_string = self.request.get('internal_only')
    if internal_only_string == 'true':
      internal_only = True
    elif internal_only_string == 'false':
      internal_only = False
    else:
      self.ReportError('No internal_only field')
      return

    bot_names = self.request.get_all('bots')
    test_key_urlsafe = self.request.get('test')
    cursor = self.request.get('cursor', None)

    if bot_names and len(bot_names) > 1:
      self._UpdateMultipleBots(bot_names, internal_only)
      self.RenderHtml('result.html', {
          'headline': ('Updating internal_only. This may take some time '
                       'depending on the data to update. Check the task queue '
                       'to determine whether the job is still in progress.'),
      })
    elif bot_names and len(bot_names) == 1:
      self._UpdateBot(bot_names[0], internal_only, cursor=cursor)
    elif test_key_urlsafe:
      self._UpdateTest(test_key_urlsafe, internal_only)

  def _UpdateBotWhitelist(self, bot_master_names, internal_only):
    """Updates the global bot_whitelist object, otherwise subsequent add_point
    calls will overwrite our work."""
    bot_whitelist = stored_object.Get(add_point_queue.BOT_WHITELIST_KEY)
    bot_names = [b.split('/')[1] for b in bot_master_names]

    if internal_only:
      bot_whitelist = [b for b in bot_whitelist if b not in bot_names]
    else:
      bot_whitelist.extend(bot_names)
      bot_whitelist = list(set(bot_whitelist))
    bot_whitelist.sort()

    stored_object.Set(add_point_queue.BOT_WHITELIST_KEY, bot_whitelist)

  def _UpdateMultipleBots(self, bot_names, internal_only):
    """Kicks off update tasks for individual bots and their tests."""

    self._UpdateBotWhitelist(bot_names, internal_only)

    for bot_name in bot_names:
      taskqueue.add(
          url='/change_internal_only',
          params={
              'bots': bot_name,
              'internal_only': 'true' if internal_only else 'false'
          },
          queue_name=_QUEUE_NAME)

  def _UpdateBot(self, bot_name, internal_only, cursor=None):
    """Starts updating internal_only for the given bot and associated data."""
    master, bot = bot_name.split('/')
    bot_key = ndb.Key('Master', master, 'Bot', bot)

    if not cursor:
      # First time updating for this Bot.
      bot_entity = bot_key.get()
      if bot_entity.internal_only != internal_only:
        bot_entity.internal_only = internal_only
        bot_entity.put()
    else:
      cursor = datastore_query.Cursor(urlsafe=cursor)

    # Fetch a certain number of TestMetadata entities starting from cursor. See:
    # https://developers.google.com/appengine/docs/python/ndb/queryclass

    # Start update tasks for each existing subordinate TestMetadata.
    test_query = graph_data.TestMetadata.query(
        graph_data.TestMetadata.master_name == master,
        graph_data.TestMetadata.bot_name == bot)
    test_keys, next_cursor, more = test_query.fetch_page(
        _MAX_TESTS_TO_PUT, start_cursor=cursor, keys_only=True)

    for test_key in test_keys:
      taskqueue.add(
          url='/change_internal_only',
          params={
              'test': test_key.urlsafe(),
              'internal_only': 'true' if internal_only else 'false',
          },
          queue_name=_QUEUE_NAME)

    if more:
      taskqueue.add(
          url='/change_internal_only',
          params={
              'bots': bot_name,
              'cursor': next_cursor.urlsafe(),
              'internal_only': 'true' if internal_only else 'false',
          },
          queue_name=_QUEUE_NAME)

  def _UpdateTest(self, test_key_urlsafe, internal_only):
    """Updates the given TestMetadata and associated Row entities."""
    test_key = ndb.Key(urlsafe=test_key_urlsafe)

    # First time updating for this TestMetadata.
    test_entity = test_key.get()
    if test_entity.internal_only != internal_only:
      test_entity.internal_only = internal_only
      test_entity.put()

    # Update all of the Anomaly entities for this test.
    # Assuming that this should be fast enough to do in one request
    # for any one test.
    anomalies = anomaly.Anomaly.GetAlertsForTest(test_key)
    for anomaly_entity in anomalies:
      if anomaly_entity.internal_only != internal_only:
        anomaly_entity.internal_only = internal_only
    ndb.put_multi(anomalies)
