# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import webapp2

from dashboard.common import namespaced_stored_object


_BOT_CONFIGURATIONS = 'bot_configurations'


class Config(webapp2.RequestHandler):
  """Handler returning site configuration details."""

  def get(self):
    bot_configurations = namespaced_stored_object.Get(_BOT_CONFIGURATIONS)
    self.response.out.write(json.dumps({
        'configurations': sorted(bot_configurations.iterkeys()),
    }))
