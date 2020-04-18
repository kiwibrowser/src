# -*- coding: utf-8 -*-
# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for the boto_compat module."""

from __future__ import print_function

import ConfigParser
import os

from chromite.lib import boto_compat
from chromite.lib import cros_test_lib
from chromite.lib import osutils


class FixBotoCertsTest(cros_test_lib.TempDirTestCase):
  """Tests FixBotoCerts functionality."""

  def testCaFix(self):
    os.environ['BOTO_CONFIG'] = os.path.join(self.tempdir, 'fake')
    with boto_compat.FixBotoCerts(strict=True):
      boto_config = os.environ['BOTO_CONFIG']
      self.assertExists(boto_config)

      config = ConfigParser.SafeConfigParser()
      config.read(boto_config)

      cafile = config.get('Boto', 'ca_certificates_file')
      self.assertExists(cafile)

    self.assertNotExists(boto_config)

  def testMergeBotoConfig(self):
    boto_config = os.path.join(self.tempdir, 'boto.cfg')
    osutils.WriteFile(boto_config, '[S]\nk = v')
    os.environ['BOTO_CONFIG'] = boto_config
    
    with boto_compat.FixBotoCerts(strict=True):
      config = ConfigParser.SafeConfigParser()
      config.read(os.environ['BOTO_CONFIG'])
      self.assertEqual(config.get('S', 'k'), 'v')
      self.assertTrue(config.has_option('Boto', 'ca_certificates_file'))

    self.assertEqual(os.environ['BOTO_CONFIG'], boto_config)

  def testMergeBotoPath(self):
    cfgfile1 = os.path.join(self.tempdir, 'boto1.cfg')
    osutils.WriteFile(cfgfile1, '[S]\nk = v\nk2 = v1')
    cfgfile2 = os.path.join(self.tempdir, 'boto2.cfg')
    osutils.WriteFile(cfgfile2, '[S]\nk2 = v2')
    os.environ['BOTO_PATH'] = boto_path = '%s:%s' % (cfgfile1, cfgfile2)

    with boto_compat.FixBotoCerts(strict=True):
      config = ConfigParser.SafeConfigParser()
      config.read(os.environ['BOTO_CONFIG'])
      self.assertEqual(config.get('S', 'k'), 'v')
      self.assertEqual(config.get('S', 'k2'), 'v2')
      self.assertTrue(config.has_option('Boto', 'ca_certificates_file'))

    self.assertEqual(os.environ['BOTO_PATH'], boto_path)

  def testActivateFalse(self):
    os.environ.pop('BOTO_CONFIG', None)
    with boto_compat.FixBotoCerts(strict=True, activate=False):
      self.assertNotIn('BOTO_CONFIG', os.environ)
