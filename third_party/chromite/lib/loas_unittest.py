# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for LOAS helper functions."""

from __future__ import print_function

import datetime

from chromite.lib import alerts
from chromite.lib import cros_test_lib
from chromite.lib import loas
from chromite.lib import partial_mock


class TestLoas(cros_test_lib.MockTestCase):
  """General tests for the LOAS module"""

  def setUp(self):
    self.rc_mock = self.StartPatcher(cros_test_lib.RunCommandMock())
    self.email_mock = self.PatchObject(alerts, 'SendEmail')

    self.user = 'foo'
    self.email = 'some@email.com'
    self.loas = loas.Loas(self.user, self.email)

  def testCheckSuccess(self):
    """Verify Check() behavior when loas_check passes."""
    self.rc_mock.AddCmdResult(partial_mock.In('runloas'), returncode=0)
    self.loas.Check()

  def testCheckError(self):
    """Verify Check() behavior when loas_check fails."""
    self.rc_mock.AddCmdResult(partial_mock.In('runloas'), returncode=1)
    self.assertRaises(loas.LoasError, self.loas.Check)

  def testStatusError(self):
    """Verify that errors from prodcertstatus result in an e-mail."""
    self.rc_mock.AddCmdResult(
        partial_mock.In('prodcertstatus'), returncode=1,
        error='No valid SSL-ENROLLED CERT certs')
    self.loas.Status()
    self.assertEqual(self.email_mock.call_count, 1)

  def testStatusUpToDate(self):
    """Verify that up-to-date certs delay further checks for a while."""
    self.rc_mock.AddCmdResult(
        partial_mock.In('prodcertstatus'), returncode=0,
        error='SSL-ENROLLED CERT cert expires in about 39 days')

    # This should invoke prodcertstatus.
    self.loas.Status()
    self.assertEqual(self.email_mock.call_count, 1)

    # While this should return quickly.
    self.loas.Status()
    self.assertEqual(self.email_mock.call_count, 1)

  def testStatusExpiresSoon(self):
    """Verify that expiring certs generate e-mails once a day."""
    self.rc_mock.AddCmdResult(
        partial_mock.In('prodcertstatus'), returncode=0,
        error='SSL-ENROLLED CERT cert expires in about 3 days')

    # This should invoke prodcertstatus & send an e-mail.
    self.loas.Status()
    self.assertEqual(self.email_mock.call_count, 1)

    # While this should do nothing but return (only one e-mail a day).
    self.loas.Status()
    self.loas.Status()
    self.loas.Status()
    self.assertEqual(self.email_mock.call_count, 1)

    # Grub around in internal state to fast forward the clock by a day :/.
    self.loas.last_notification += datetime.timedelta(days=-1)

    # This should send out one e-mail.
    self.loas.Status()
    self.assertEqual(self.email_mock.call_count, 2)
    self.loas.Status()
    self.loas.Status()
    self.loas.Status()
    self.assertEqual(self.email_mock.call_count, 2)
