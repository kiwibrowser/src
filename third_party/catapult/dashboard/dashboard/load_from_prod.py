# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Provides a web interface for loading graph data from the production server.

This is meant to be used on a dev server only.
"""

import base64
import json
import os
import urllib

from google.appengine.api import app_identity
from google.appengine.api import urlfetch
from google.appengine.ext import ndb
from google.appengine.ext.ndb import model

from dashboard import update_test_suites
from dashboard.common import datastore_hooks
from dashboard.common import request_handler

_DEV_APP_ID = 'dev~' + app_identity.get_application_id()

_PROD_DUMP_GRAPH_JSON_URL = 'https://chromeperf.appspot.com/dump_graph_json'


class LoadFromProdHandler(request_handler.RequestHandler):
  """Debugging handler to load data from the production instance."""

  def get(self):
    if 'Development' not in os.environ['SERVER_SOFTWARE']:
      self.RenderHtml('result.html', {
          'errors': [
              'This should not be run in production, only on dev server.']})
      return
    self.RenderHtml('load_from_prod.html', {})

  def post(self):
    """Loads the requested data from the production server."""
    if 'Development' not in os.environ['SERVER_SOFTWARE']:
      self.RenderHtml('result.html', {
          'errors': [
              'This should not be run in production, only on dev server.']})
      return

    sheriff = self.request.get('sheriff')
    test_path = self.request.get('test_path')
    raw_json = self.request.get('raw_json')
    protos = None
    if test_path:
      num_points = self.request.get('num_points')
      end_rev = self.request.get('end_rev')
      url = ('%s?test_path=%s&num_points=%s' %
             (_PROD_DUMP_GRAPH_JSON_URL, urllib.quote(test_path), num_points))
      if end_rev:
        url += '&end_rev=%s' % end_rev
    elif sheriff:
      sheriff_name = self.request.get('sheriff')
      num_alerts = self.request.get('num_alerts')
      num_points = self.request.get('num_points')
      url = ('%s?sheriff=%s&num_alerts=%s&num_points=%s' %
             (_PROD_DUMP_GRAPH_JSON_URL, urllib.quote(sheriff_name), num_alerts,
              num_points))
    elif raw_json:
      protos = json.loads(raw_json)
    else:
      self.RenderHtml('result.html', {
          'errors': [
              'Need to specify a test_path, sheriff or json data file.']})
      return

    if not protos:
      # This takes a while.
      response = urlfetch.fetch(url, deadline=60)
      if response.status_code != 200:
        msg_template = 'Could not fetch %s (Status: %s)'
        err_msg = msg_template % (url, response.status_code)
        self.RenderHtml('result.html', {'errors': [err_msg]})
        return
      protos = json.loads(response.content)

    kinds = ['Master', 'Bot', 'TestMetadata', 'Row', 'Sheriff', 'Anomaly']
    entities = {k: [] for k in kinds}
    for proto in protos:
      pb = model.entity_pb.EntityProto(base64.b64decode(proto))
      # App ID is set in the key and all the ReferenceProperty keys to
      # 's~chromeperf'. It won't be found in queries unless we use the
      # devserver app ID.
      key = pb.mutable_key()
      key.set_app(_DEV_APP_ID)
      for prop in pb.property_list():
        val = prop.mutable_value()
        if val.has_referencevalue():
          ref = val.mutable_referencevalue()
          ref.set_app(_DEV_APP_ID)
      entity = ndb.ModelAdapter().pb_to_entity(pb)
      entities[entity.key.kind()].append(entity)

    for kind in kinds:
      ndb.put_multi(entities[kind])

    update_test_suites.UpdateTestSuites(datastore_hooks.INTERNAL)
    update_test_suites.UpdateTestSuites(datastore_hooks.EXTERNAL)

    num_entities = sum(len(entities[kind]) for kind in kinds)
    self.RenderHtml('result.html', {
        'results': [
            {
                'name': 'Added data',
                'value': '%d entities' % num_entities
            }
        ]
    })
