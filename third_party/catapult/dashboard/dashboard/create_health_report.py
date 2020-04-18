# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Provides the web interface for adding and editing sheriff rotations."""

import logging
import json

from google.appengine.api import users
from google.appengine.ext import ndb

from dashboard.common import request_handler
from dashboard.common import utils
from dashboard.common import xsrf
from dashboard.models import table_config


class CreateHealthReportHandler(request_handler.RequestHandler):

  def get(self):
    """Renders the UI with the form fields."""
    self.RenderStaticHtml('create_health_report.html')

  def post(self):
    """POSTS the data to the datastore."""

    user = users.get_current_user()
    if not user:
      self.response.out.write(json.dumps({'error': 'User not logged in.'}))
      return
    if not utils.IsInternalUser():
      self.response.out.write(json.dumps(
          {'error':
           'Unauthorized access, please use chromium account to login.'}))
      return

    get_token = self.request.get('getToken')
    get_table_config_list = self.request.get('getTableConfigList')
    get_table_config_details = self.request.get('getTableConfigDetails')
    if get_token == 'true':
      values = {}
      self.GetDynamicVariables(values)
      self.response.out.write(json.dumps({
          'xsrf_token': values['xsrf_token'],
      }))
    elif get_table_config_list:
      self._GetTableConfigList()
    elif get_table_config_details:
      self._GetTableConfigDetails(get_table_config_details)
    else:
      self._CreateTableConfig()

  def _GetTableConfigList(self):
    query = table_config.TableConfig.query()
    table_config_list = query.fetch(keys_only=True)
    return_list = []
    for config in table_config_list:
      return_list.append(config.id())
    self.response.out.write(json.dumps({
        'table_config_list': return_list,
    }))

  def _GetTableConfigDetails(self, config_name):
    config_entity = ndb.Key('TableConfig', config_name).get()
    if config_entity:
      master_bot_list = []
      for bot in config_entity.bots:
        master_bot_list.append(bot.parent().string_id() + '/' + bot.string_id())
      self.response.out.write(json.dumps({
          'table_name': config_name,
          'table_bots': master_bot_list,
          'table_tests': config_entity.tests,
          'table_layout': config_entity.table_layout
      }))
    else:
      self.response.out.write(json.dumps({
          'error': 'Invalid config name.'
      }))

  def _CreateTableConfig(self):
    """Creates a table config. Writes a valid name or an error message."""
    self._ValidateToken()
    name = self.request.get('tableName')
    master_bot = self.request.get('tableBots').splitlines()
    tests = self.request.get('tableTests').splitlines()
    table_layout = self.request.get('tableLayout')
    override = int(self.request.get('override'))
    user = users.get_current_user()
    if not name or not master_bot or not tests or not table_layout or not user:
      self.response.out.write(json.dumps({
          'error': 'Please fill out the form entirely.'
          }))
      return

    try:
      created_table = table_config.CreateTableConfig(
          name=name, bots=master_bot, tests=tests, layout=table_layout,
          username=user.email(), override=override)
    except table_config.BadRequestError as error:
      self.response.out.write(json.dumps({
          'error': error.message,
      }))
      logging.error(error.message)
      return


    if created_table:
      self.response.out.write(json.dumps({
          'name': name,
      }))
    else:
      self.response.out.write(json.dumps({
          'error': 'Could not create table.',
      }))
      logging.error('Could not create table.')

  def _ValidateToken(self):
    user = users.get_current_user()
    token = str(self.request.get('xsrf_token'))
    if not user or not xsrf._ValidateToken(token, user):
      self.abort(403)
