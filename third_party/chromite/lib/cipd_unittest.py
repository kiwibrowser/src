# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for cipd."""

from __future__ import print_function

import json
import hashlib

from chromite.lib import cipd
from chromite.lib import cros_test_lib

import httplib2


class CIPDTest(cros_test_lib.MockTestCase):
  """Tests for chromite.lib.cipd"""

  def testDownloadCIPD(self):
    MockHttp = self.PatchObject(httplib2, 'Http')
    first_body = json.dumps({
        'client_binary': {
            'sha1': 'bogus-sha1',
            'fetch_url': 'http://foo'}})
    response = {'status': '200'}
    MockHttp.return_value.request.side_effect = [
        (response, first_body),
        (response, b'bogus binary file')]

    sha1 = self.PatchObject(hashlib, 'sha1')
    sha1.return_value.hexdigest.return_value = 'bogus-sha1'

    # Access to a protected member XXX of a client class
    # pylint: disable=W0212
    self.assertTrue(cipd._DownloadCIPD('bogus-instance-id'))
