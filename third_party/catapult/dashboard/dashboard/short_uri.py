# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Provides an endpoint for handling storing and retrieving page states."""

import hashlib
import json

from google.appengine.ext import ndb

from dashboard.common import request_handler
from dashboard.models import page_state


class ShortUriHandler(request_handler.RequestHandler):
  """Handles short URI."""

  def get(self):
    """Handles getting page states."""
    state_id = self.request.get('sid')

    if not state_id:
      self.ReportError('Missing required parameters.', status=400)
      return

    state = ndb.Key(page_state.PageState, state_id).get()

    if not state:
      self.ReportError('Invalid sid.', status=400)
      return

    self.response.out.write(state.value)

  def post(self):
    """Handles saving page states and getting state id."""

    state = self.request.get('page_state')

    if not state:
      self.ReportError('Missing required parameters.', status=400)
      return

    state_id = GetOrCreatePageState(state)

    self.response.out.write(json.dumps({'sid': state_id}))


def GetOrCreatePageState(state):
  state = state.encode('utf-8')
  state_id = GenerateHash(state)
  if not ndb.Key(page_state.PageState, state_id).get():
    page_state.PageState(id=state_id, value=state).put()
  return state_id


def GenerateHash(state_string):
  """Generates a hash for a state string."""
  return hashlib.sha256(state_string).hexdigest()
