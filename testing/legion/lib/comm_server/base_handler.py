# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Base handler class for all category handlers."""


class BaseHandler(object):
  """Sets up default verb handlers for the child class."""

  def do_PUT(self, request):
    request.send_response(501)

  def do_POST(self, request):
    request.send_response(501)

  def do_GET(self, request):
    request.send_response(501)

  def do_DELETE(self, request):
    request.send_response(501)
