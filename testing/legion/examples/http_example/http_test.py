# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import logging
import os
import socket
import sys
import time

TESTING_DIR = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), '..', '..', '..')
sys.path.append(TESTING_DIR)

from legion import legion_test_case


class HttpTest(legion_test_case.TestCase):
  """Example HTTP test case."""

  @classmethod
  def GetArgs(cls):
    """Get command line args."""
    parser = argparse.ArgumentParser()
    parser.add_argument('--http-server')
    parser.add_argument('--http-client')
    parser.add_argument('--os', default='Ubuntu-14.04')
    args, _ = parser.parse_known_args()
    return args

  @classmethod
  def CreateTask(cls, name, task_hash, os_type):
    """Create a new task."""
    #pylint: disable=unexpected-keyword-arg,no-value-for-parameter
    #pylint: disable=arguments-differ
    task = super(HttpTest, cls).CreateTask(
        name=name,
        isolated_hash=task_hash,
        dimensions={'os': os_type})
    task.Create()
    return task

  @classmethod
  def setUpClass(cls):
    """Creates the task machines and waits until they connect."""
    args = cls.GetArgs()
    cls.http_server = cls.CreateTask(
        'http_server', args.http_server, args.os)
    cls.http_client = cls.CreateTask(
        'http_client', args.http_client, args.os)
    cls.http_server.WaitForConnection()
    cls.http_client.WaitForConnection()

  def CanConnectToServerPort(self, server_port):
    """Connect to a port on the http_server.

    Returns:
      True if the connection succeeded, False otherwise.
    """
    try:
      s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      s.connect((self.http_server.ip_address, server_port))
      return True
    except socket.error:
      return False

  def FindOpenPortOnServer(self):
    """Find an open port on the server and return it.

    Returns:
      The value of an open port.
    """
    for server_port in  xrange(2000, 20000):
      if not self.CanConnectToServerPort(server_port):
        return server_port
    self.fail('Unable to find an open port on the server.')

  def StartServer(self, server_port):
    """Starts the http_server process.

    Returns:
      The server process.
    """
    def WaitForServer():
      timeout = time.time() + 5
      while timeout > time.time():
        if self.CanConnectToServerPort(server_port):
          return
      self.fail('Server process failed to start')

    cmd = [
        self.http_server.executable,
        'http_server.py',
        '--port', str(server_port)
        ]
    proc = self.http_server.Process(cmd)
    WaitForServer()
    return proc

  def StartClient(self, server_port):
    """Starts the http_client process.

    Returns:
      The client process.
    """
    cmd = [
        self.http_client.executable,
        'http_client.py',
        '--server', self.http_server.ip_address,
        '--port', str(server_port)
        ]
    return self.http_client.Process(cmd)

  def testHttpWorks(self):
    """Tests that the client process can talk to the server process."""
    server_proc = None
    client_proc = None
    try:
      server_port = self.FindOpenPortOnServer()
      logging.info('Starting server at %s:%s', self.http_server.ip_address,
                   server_port)
      server_proc = self.StartServer(server_port)
      logging.info('Connecting to server at %s:%s', self.http_server.ip_address,
                   server_port)
      client_proc = self.StartClient(server_port)
      client_proc.Wait()
      logging.info('client_proc.stdout: %s', client_proc.ReadStdout())
      logging.info('client_proc.stderr: %s', client_proc.ReadStderr())
      self.assertEqual(client_proc.GetReturncode(), 0)
    finally:
      if server_proc:
        server_proc.Kill()
        server_proc.Delete()
      if client_proc:
        client_proc.Kill()
        client_proc.Delete()


if __name__ == '__main__':
  legion_test_case.main()
