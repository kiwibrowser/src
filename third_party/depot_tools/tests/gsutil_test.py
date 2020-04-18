#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test gsutil.py."""


import __builtin__
import base64
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
import urllib2
import zipfile


# Add depot_tools to path
THIS_DIR = os.path.dirname(os.path.abspath(__file__))
DEPOT_TOOLS_DIR = os.path.dirname(THIS_DIR)
sys.path.append(DEPOT_TOOLS_DIR)

import gsutil


class TestError(Exception):
  pass


class Buffer(object):
  def __init__(self, data=None):
    self.data = data or ''

  def write(self, buf):
    self.data += buf

  def read(self, amount=None):
    if not amount:
      amount = len(self.data)
    result = self.data[:amount]
    self.data = self.data[amount:]
    return result


class FakeCall(object):
  def __init__(self):
    self.expectations = []

  def add_expectation(self, *args, **kwargs):
    returns = kwargs.pop('_returns', None)
    self.expectations.append((args, kwargs, returns))

  def __call__(self, *args, **kwargs):
    if not self.expectations:
      raise TestError('Got unexpected\n%s\n%s' % (args, kwargs))
    exp_args, exp_kwargs, exp_returns = self.expectations.pop(0)
    if args != exp_args or kwargs != exp_kwargs:
      message = 'Expected:\n  args: %s\n  kwargs: %s\n' % (exp_args, exp_kwargs)
      message += 'Got:\n  args: %s\n  kwargs: %s\n' % (args, kwargs)
      raise TestError(message)
    return exp_returns


class GsutilUnitTests(unittest.TestCase):
  def setUp(self):
    self.fake = FakeCall()
    self.tempdir = tempfile.mkdtemp()
    self.old_urlopen = getattr(urllib2, 'urlopen')
    self.old_call = getattr(subprocess, 'call')
    setattr(urllib2, 'urlopen', self.fake)
    setattr(subprocess, 'call', self.fake)

  def tearDown(self):
    self.assertEqual(self.fake.expectations, [])
    shutil.rmtree(self.tempdir)
    setattr(urllib2, 'urlopen', self.old_urlopen)
    setattr(subprocess, 'call', self.old_call)

  def test_download_gsutil(self):
    version = '4.2'
    filename = 'gsutil_%s.zip' % version
    full_filename = os.path.join(self.tempdir, filename)
    fake_file = 'This is gsutil.zip'
    fake_file2 = 'This is other gsutil.zip'
    url = '%s%s' % (gsutil.GSUTIL_URL, filename)
    self.fake.add_expectation(url, _returns=Buffer(fake_file))

    self.assertEquals(
        gsutil.download_gsutil(version, self.tempdir), full_filename)
    with open(full_filename, 'r') as f:
      self.assertEquals(fake_file, f.read())

    metadata_url = gsutil.API_URL + filename
    md5_calc = hashlib.md5()
    md5_calc.update(fake_file)
    b64_md5 = base64.b64encode(md5_calc.hexdigest())
    self.fake.add_expectation(metadata_url, _returns=Buffer(json.dumps({
        'md5Hash': b64_md5
    })))
    self.assertEquals(
        gsutil.download_gsutil(version, self.tempdir), full_filename)
    with open(full_filename, 'r') as f:
      self.assertEquals(fake_file, f.read())
    self.assertEquals(self.fake.expectations, [])

    self.fake.add_expectation(metadata_url, _returns=Buffer(json.dumps({
        'md5Hash': base64.b64encode('aaaaaaa')  # Bad MD5
    })))
    self.fake.add_expectation(url, _returns=Buffer(fake_file2))
    self.assertEquals(
        gsutil.download_gsutil(version, self.tempdir), full_filename)
    with open(full_filename, 'r') as f:
      self.assertEquals(fake_file2, f.read())
    self.assertEquals(self.fake.expectations, [])

  def test_ensure_gsutil_full(self):
    version = '4.2'
    gsutil_dir = os.path.join(self.tempdir, 'gsutil_%s' % version, 'gsutil')
    gsutil_bin = os.path.join(gsutil_dir, 'gsutil')
    gsutil_flag = os.path.join(gsutil_dir, 'install.flag')
    os.makedirs(gsutil_dir)

    zip_filename = 'gsutil_%s.zip' % version
    url = '%s%s' % (gsutil.GSUTIL_URL, zip_filename)
    _, tempzip = tempfile.mkstemp()
    fake_gsutil = 'Fake gsutil'
    with zipfile.ZipFile(tempzip, 'w') as zf:
      zf.writestr('gsutil/gsutil', fake_gsutil)
    with open(tempzip, 'rb') as f:
      self.fake.add_expectation(url, _returns=Buffer(f.read()))

    # This should write the gsutil_bin with 'Fake gsutil'
    gsutil.ensure_gsutil(version, self.tempdir, False)
    self.assertTrue(os.path.exists(gsutil_bin))
    with open(gsutil_bin, 'r') as f:
      self.assertEquals(f.read(), fake_gsutil)
    self.assertTrue(os.path.exists(gsutil_flag))
    self.assertEquals(self.fake.expectations, [])

  def test_ensure_gsutil_short(self):
    version = '4.2'
    gsutil_dir = os.path.join(self.tempdir, 'gsutil_%s' % version, 'gsutil')
    gsutil_bin = os.path.join(gsutil_dir, 'gsutil')
    gsutil_flag = os.path.join(gsutil_dir, 'install.flag')
    os.makedirs(gsutil_dir)

    with open(gsutil_bin, 'w') as f:
      f.write('Foobar')
    with open(gsutil_flag, 'w') as f:
      f.write('Barbaz')
    self.assertEquals(
        gsutil.ensure_gsutil(version, self.tempdir, False), gsutil_bin)

if __name__ == '__main__':
  unittest.main()
