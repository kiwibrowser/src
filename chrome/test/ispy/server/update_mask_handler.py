# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Request Handler to allow test mask updates."""

import webapp2
import re
import sys
import os

from common import constants
from common import image_tools
from common import ispy_utils

import gs_bucket


class UpdateMaskHandler(webapp2.RequestHandler):
  """Request handler to allow test mask updates."""

  def post(self):
    """Accepts post requests.

    This method will accept a post request containing device, site and
    device_id parameters. This method takes the diff of the run
    indicated by it's parameters and adds it to the mask of the run's
    test. It will then delete the run it is applied to and redirect
    to the device list view.
    """
    test_run = self.request.get('test_run')
    expectation = self.request.get('expectation')

    # Short-circuit if a parameter is missing.
    if not (test_run and expectation):
      self.response.headers['Content-Type'] = 'json/application'
      self.response.write(json.dumps(
          {'error': '\'test_run\' and \'expectation\' must be '
                    'supplied to update a mask.'}))
      return
    # Otherwise, set up the utilities.
    self.bucket = gs_bucket.GoogleCloudStorageBucket(constants.BUCKET)
    self.ispy = ispy_utils.ISpyUtils(self.bucket)
    # Short-circuit if the failure does not exist.
    if not self.ispy.FailureExists(test_run, expectation):
      self.response.headers['Content-Type'] = 'json/application'
      self.response.write(json.dumps(
        {'error': 'Could not update mask because failure does not exist.'}))
      return
    # Get the failure namedtuple (which also computes the diff).
    failure = self.ispy.GetFailure(test_run, expectation)
    # Upload the new mask in place of the original.
    self.ispy.UpdateImage(
        ispy_utils.GetExpectationPath(expectation, 'mask.png'),
        image_tools.ConvertDiffToMask(failure.diff))
    # Now that there is no diff for the two images, remove the failure.
    self.ispy.RemoveFailure(test_run, expectation)
    # Redirect back to the sites list for the test run.
    self.redirect('/?test_run=%s' % test_run)
