# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""XHR endpoint to fill in navbar fields."""

import json

from dashboard.common import request_handler


class NavbarHandler(request_handler.RequestHandler):
  """XHR endpoint to fill in navbar fields."""

  def post(self):
    template_values = {}
    self.GetDynamicVariables(template_values, self.request.get('path'))
    self.response.out.write(json.dumps({
        'login_url': template_values['login_url'],
        'is_admin': template_values['is_admin'],
        'display_username': template_values['display_username'],
    }))
