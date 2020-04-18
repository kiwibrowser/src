# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Request Handler that updates the Expectation version."""

import webapp2

import ispy_api
from common import constants

import gs_bucket


class RebaselineHandler(webapp2.RequestHandler):
  """Request handler to allow test mask updates."""

  def post(self):
    """Accepts post requests.

    Expects a test_run as a parameter and updates the associated version file to
    use the expectations associated with that test run.
    """
    test_run = self.request.get('test_run')

    # Fail if test_run parameter is missing.
    if not test_run:
      self.response.headers['Content-Type'] = 'json/application'
      self.response.write(json.dumps(
          {'error': '\'test_run\' must be supplied to rebaseline.'}))
      return
    # Otherwise, set up the utilities.
    bucket = gs_bucket.GoogleCloudStorageBucket(constants.BUCKET)
    ispy = ispy_api.ISpyApi(bucket)
    # Update versions file.
    ispy.RebaselineToTestRun(test_run)
    # Redirect back to the sites list for the test run.
    self.redirect('/?test_run=%s' % test_run)
