#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from handler import Handler
from servlet import Request

class HandlerTest(unittest.TestCase):

  def testInvalid(self):
    handler = Handler(Request.ForTest('_notreal'))
    self.assertEqual(404, handler.Get().status)

if __name__ == '__main__':
  unittest.main()
