# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Local unittest for legion.lib.comm_server."""

import httplib

# pylint: disable=relative-import
import legion_unittest

from legion.lib.comm_server import comm_server


class CommServerTest(legion_unittest.TestCase):

  def setUp(self):
    super(CommServerTest, self).setUp()
    self.server = comm_server.CommServer()
    self.server.start()

  def tearDown(self):
    try:
      self.server.shutdown()
    finally:
      super(CommServerTest, self).tearDown()

  def Connect(self, verb, path, message=''):
    conn = httplib.HTTPConnection('localhost', self.server.port)
    conn.request(verb, path, body=message)
    return conn.getresponse()

  def testMessagesUsedAsSignals(self):
    self.assertEquals(
        self.Connect('GET', '/messages/message1').status, 404)
    self.assertEquals(
        self.Connect('PUT', '/messages/message1').status, 200)
    self.assertEquals(
        self.Connect('GET', '/messages/message1').status, 200)
    self.assertEquals(
        self.Connect('DELETE', '/messages/message1').status, 200)
    self.assertEquals(
        self.Connect('DELETE', '/messages/message1').status, 404)
    self.assertEquals(
        self.Connect('GET', '/messages/message1').status, 404)

  def testErrors(self):
    for verb in ['GET', 'PUT', 'DELETE']:
      self.assertEquals(
          self.Connect(verb, '/').status, 403)
      self.assertEquals(
          self.Connect(verb, '/foobar').status, 403)
      self.assertEquals(
          self.Connect(verb, '/foobar/').status, 405)

  def testMessagePassing(self):
    self.assertEquals(
        self.Connect('GET', '/messages/message2').status, 404)
    self.assertEquals(
        self.Connect('PUT', '/messages/message2', 'foo').status, 200)
    self.assertEquals(
        self.Connect('GET', '/messages/message2').read(), 'foo')
    self.assertEquals(
        self.Connect('DELETE', '/messages/message2').status, 200)
    self.assertEquals(
        self.Connect('DELETE', '/messages/message2').status, 404)
    self.assertEquals(
        self.Connect('GET', '/messages/message2').status, 404)


if __name__ == '__main__':
  legion_unittest.main()
