# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Request handler to display an image from Google Cloud Storage."""

import json
import os
import sys
import webapp2

from common import cloud_bucket
from common import constants

import gs_bucket


class ImageHandler(webapp2.RequestHandler):
  """A request handler to avoid the Same-Origin problem in the debug view."""

  def get(self):
    """Handles get requests to the ImageHandler.

    GET Parameters:
      file_path: A path to an image resource in Google Cloud Storage.
    """
    file_path = self.request.get('file_path')
    if not file_path:
      self.error(404)
      return
    bucket = gs_bucket.GoogleCloudStorageBucket(constants.BUCKET)
    try:
      image = bucket.DownloadFile(file_path)
    except cloud_bucket.FileNotFoundError:
      self.error(404)
    else:
      self.response.headers['Content-Type'] = 'image/png'
      self.response.out.write(image)
