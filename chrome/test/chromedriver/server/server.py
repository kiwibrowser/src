# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import atexit
import os
import socket
import subprocess
import time
import urllib2


class Server(object):
  """A running ChromeDriver server."""

  def __init__(self, exe_path, log_path=None, verbose=True):
    """Starts the ChromeDriver server and waits for it to be ready.

    Args:
      exe_path: path to the ChromeDriver executable
      log_path: path to the log file
    Raises:
      RuntimeError if ChromeDriver fails to start
    """
    if not os.path.exists(exe_path):
      raise RuntimeError('ChromeDriver exe not found at: ' + exe_path)

    port = self._FindOpenPort()
    chromedriver_args = [exe_path, '--port=%d' % port]
    if log_path:
      chromedriver_args.extend(['--log-path=%s' %log_path])
      if verbose:
        chromedriver_args.extend(['--verbose',
                                  '--vmodule=*/chrome/test/chromedriver/*=4'])
    self._process = subprocess.Popen(chromedriver_args)
    self._url = 'http://127.0.0.1:%d' % port
    if self._process is None:
      raise RuntimeError('ChromeDriver server cannot be started')

    max_time = time.time() + 10
    while not self.IsRunning():
      if time.time() > max_time:
        self._process.terminate()
        raise RuntimeError('ChromeDriver server did not start')
      time.sleep(0.1)

    atexit.register(self.Kill)

  def _FindOpenPort(self):
    for port in range(9500, 10000):
      try:
        socket.create_connection(('127.0.0.1', port), 0.2).close()
      except socket.error:
        return port
    raise RuntimeError('Cannot find open port to launch ChromeDriver')

  def GetUrl(self):
    return self._url

  def IsRunning(self):
    """Returns whether the server is up and running."""
    try:
      urllib2.urlopen(self.GetUrl() + '/status')
      return True
    except urllib2.URLError:
      return False

  def Kill(self):
    """Kills the ChromeDriver server, if it is running."""
    if self._process is None:
      return

    try:
      urllib2.urlopen(self.GetUrl() + '/shutdown', timeout=10).close()
    except:
      self._process.terminate()
    self._process.wait()
    self._process = None
