# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Task-based unittest for the Legion event server."""

import argparse
import httplib
import sys
import unittest


class CommServerTest(unittest.TestCase):

  def __init__(self, *args, **kwargs):
    super(CommServerTest, self).__init__(*args, **kwargs)

    parser = argparse.ArgumentParser()
    parser.add_argument('--address')
    parser.add_argument('--port', type=int)
    self.args, _ = parser.parse_known_args()

  def Connect(self, verb, path, message=''):
    conn = httplib.HTTPConnection(self.args.address, self.args.port)
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
  unittest.main(argv=sys.argv[:1])
