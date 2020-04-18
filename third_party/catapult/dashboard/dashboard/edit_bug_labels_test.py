# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import webapp2
import webtest

from google.appengine.api import users

from dashboard import edit_bug_labels
from dashboard.common import testing_common
from dashboard.common import xsrf
from dashboard.models import bug_label_patterns


class EditBugLabelsTest(testing_common.TestCase):

  def setUp(self):
    super(EditBugLabelsTest, self).setUp()
    app = webapp2.WSGIApplication(
        [('/edit_bug_labels', edit_bug_labels.EditBugLabelsHandler)])
    self.testapp = webtest.TestApp(app)
    # Set the current user to be an admin.
    self.SetCurrentUser('x@google.com', is_admin=True)

  def tearDown(self):
    super(EditBugLabelsTest, self).tearDown()
    self.UnsetCurrentUser()

  def testBugLabelPattern_AddAndRemove(self):
    self.testapp.post('/edit_bug_labels', {
        'action': 'add_buglabel_pattern',
        'buglabel_to_add': 'Performance-1',
        'pattern': '*/*/Suite1/*',
        'xsrf_token': xsrf.GenerateToken(users.get_current_user()),
    })

    # The list of patterns should now contain the pattern that was added.
    self.assertEqual(
        ['*/*/Suite1/*'],
        bug_label_patterns.GetBugLabelPatterns()['Performance-1'])

    # Add another pattern for the same bug label.
    self.testapp.post('/edit_bug_labels', {
        'action': 'add_buglabel_pattern',
        'buglabel_to_add': 'Performance-1',
        'pattern': '*/*/Suite2/*',
        'xsrf_token': xsrf.GenerateToken(users.get_current_user()),
    })

    # The list of patterns should now contain both patterns.
    self.assertEqual(
        ['*/*/Suite1/*', '*/*/Suite2/*'],
        bug_label_patterns.GetBugLabelPatterns()['Performance-1'])

    # Remove the BugLabelPattern entity.
    self.testapp.post('/edit_bug_labels', {
        'action': 'remove_buglabel_pattern',
        'buglabel_to_remove': 'Performance-1',
        'xsrf_token': xsrf.GenerateToken(users.get_current_user()),
    })

    # It should now be absent from the datastore.
    self.assertNotIn(
        'Performance-1', bug_label_patterns.GetBugLabelPatterns())


if __name__ == '__main__':
  unittest.main()
