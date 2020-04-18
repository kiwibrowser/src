# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from dashboard.common import testing_common
from dashboard.models import sheriff


class SheriffTest(testing_common.TestCase):

  def testSheriffUrlValidator_InvalidUrl_RaisesError(self):
    with self.assertRaises(sheriff.ValidationError):
      sheriff.Sheriff(url='oops')
    # The URL must have a scheme and a non-empty path.
    with self.assertRaises(sheriff.ValidationError):
      sheriff.Sheriff(url='x.com')
    with self.assertRaises(sheriff.ValidationError):
      sheriff.Sheriff(url='http://x.com')

  def testSheriffUrlValidator_ValidUrl_DoesntRaiseError(self):
    # By contrast, this URL should be accepted.
    sheriff.Sheriff(url='http://x.com/')

  def testSheriffEmail_InvalidEmail_RaisesError(self):
    # If the given email address is not valid, an assertion error
    # will be raised while creating the entity
    with self.assertRaises(sheriff.ValidationError):
      sheriff.Sheriff(email='oops')

  def testSheriffEmailValidator_ValidEmail_DoesntRaiseError(self):
    # Email addresses with + characters are allowed.
    sheriff.Sheriff(email='some+thing@yahoo.com').put()
    # If the Sheriff is internal only, the email address domain must be within
    # the set of allowed email address domains.
    sheriff.Sheriff(internal_only=True, email='my-alerts@google.com').put()
    sheriff.Sheriff(internal_only=True, email='alerts@chromium.org').put()

  def testSheriffEmail_InternalOnly_OtherDomain_RaisesError(self):
    # If it's not within the set of allowed domains, an exception will be raised
    # while putting.
    entity = sheriff.Sheriff(internal_only=True, email='x@notgoogle.com')
    with self.assertRaises(sheriff.ValidationError):
      entity.put()


if __name__ == '__main__':
  unittest.main()
