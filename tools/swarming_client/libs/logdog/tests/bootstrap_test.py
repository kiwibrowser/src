#!/usr/bin/env python
# Copyright 2016 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

import os
import sys
import unittest

ROOT_DIR = os.path.dirname(os.path.abspath(os.path.join(
    __file__.decode(sys.getfilesystemencoding()),
    os.pardir, os.pardir, os.pardir)))
sys.path.insert(0, ROOT_DIR)

from libs.logdog import bootstrap, stream


class BootstrapTestCase(unittest.TestCase):

  def setUp(self):
    self.env = {
        bootstrap.ButlerBootstrap._ENV_PROJECT: 'test-project',
        bootstrap.ButlerBootstrap._ENV_PREFIX: 'foo/bar',
        bootstrap.ButlerBootstrap._ENV_STREAM_SERVER_PATH: 'fake:path',
        bootstrap.ButlerBootstrap._ENV_COORDINATOR_HOST: 'example.appspot.com',
    }

  def testProbeSucceeds(self):
    bs = bootstrap.ButlerBootstrap.probe(self.env)
    self.assertEqual(bs, bootstrap.ButlerBootstrap(
      project='test-project',
      prefix='foo/bar',
      streamserver_uri='fake:path',
      coordinator_host='example.appspot.com'))

  def testProbeNoBootstrapRaisesError(self):
    self.assertRaises(bootstrap.NotBootstrappedError,
        bootstrap.ButlerBootstrap.probe, env={})

  def testProbeMissingProjectRaisesError(self):
    self.env.pop(bootstrap.ButlerBootstrap._ENV_PROJECT)
    self.assertRaises(bootstrap.NotBootstrappedError,
        bootstrap.ButlerBootstrap.probe, env=self.env)

  def testProbeMissingPrefixRaisesError(self):
    self.env.pop(bootstrap.ButlerBootstrap._ENV_PREFIX)
    self.assertRaises(bootstrap.NotBootstrappedError,
        bootstrap.ButlerBootstrap.probe, env=self.env)

  def testProbeInvalidPrefixRaisesError(self):
    self.env[bootstrap.ButlerBootstrap._ENV_PREFIX] = '!!! not valid !!!'
    self.assertRaises(bootstrap.NotBootstrappedError,
        bootstrap.ButlerBootstrap.probe, env=self.env)

  def testCreateStreamClient(self):
    class TestStreamClient(stream.StreamClient):
      @classmethod
      def _create(cls, _value, **kwargs):
        return cls(**kwargs)

      def _connect_raw(self):
        raise NotImplementedError()

    reg = stream.StreamProtocolRegistry()
    reg.register_protocol('test', TestStreamClient)
    bs = bootstrap.ButlerBootstrap(
      project='test-project',
      prefix='foo/bar',
      streamserver_uri='test:',
      coordinator_host='example.appspot.com')
    sc = bs.stream_client(reg=reg)
    self.assertEqual(sc.prefix, 'foo/bar')
    self.assertEqual(sc.coordinator_host, 'example.appspot.com')


if __name__ == '__main__':
  unittest.main()
