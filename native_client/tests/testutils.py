# Copyright 2009 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import shutil
import subprocess
import tempfile
import unittest


# From http://lackingrhoticity.blogspot.com/2008/11/tempdirtestcase-python-unittest-helper.html
class TempDirTestCase(unittest.TestCase):

  def setUp(self):
    self._on_teardown = []

  def make_temp_dir(self):
    temp_dir = tempfile.mkdtemp(prefix="tmp-%s-" % self.__class__.__name__)
    def tear_down():
      shutil.rmtree(temp_dir)
    self._on_teardown.append(tear_down)
    return temp_dir

  def tearDown(self):
    for func in reversed(self._on_teardown):
      func()


def write_file(filename, data):
  fh = open(filename, "w")
  try:
    fh.write(data)
  finally:
    fh.close()


def read_file(filename):
  fh = open(filename, "r")
  try:
    return fh.read()
  finally:
    fh.close()


# TODO: use subprocess.check_call when we have Python 2.5 on Windows.
def check_call(*args, **kwargs):
  rc = subprocess.call(*args, **kwargs)
  if rc != 0:
    raise Exception("Failed with return code %i" % rc)
