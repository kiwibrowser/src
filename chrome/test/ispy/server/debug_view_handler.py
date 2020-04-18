# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Request handler to display the debug view for a Failure."""

import jinja2
import os
import sys
import webapp2

from common import ispy_utils

import views

JINJA = jinja2.Environment(
    loader=jinja2.FileSystemLoader(os.path.dirname(views.__file__)),
    extensions=['jinja2.ext.autoescape'])


class DebugViewHandler(webapp2.RequestHandler):
  """Request handler to display the debug view for a failure."""

  def get(self):
    """Handles get requests to the /debug_view page.

    GET Parameters:
      test_run: The test run.
      expectation: The expectation name.
    """
    test_run = self.request.get('test_run')
    expectation = self.request.get('expectation')
    expected_path = ispy_utils.GetExpectationPath(expectation, 'expected.png')
    actual_path = ispy_utils.GetFailurePath(test_run, expectation, 'actual.png')
    data = {}

    def _ImagePath(url):
      return '/image?file_path=%s' % url

    data['expected'] = _ImagePath(expected_path)
    data['actual'] = _ImagePath(actual_path)
    data['test_run'] = test_run
    data['expectation'] = expectation
    template = JINJA.get_template('debug_view.html')
    self.response.write(template.render(data))
