# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test that our interface to the user and group database works."""

from __future__ import print_function

import os

from chromite.lib import cros_test_lib
from chromite.lib import osutils
from chromite.lib import user_db


MOCK_PASSWD_CONTENTS = 'root:x:0:0:root:/root:/bin/bash'

MOCK_GROUP_CONTENTS = 'root:x:0:'


class UserDBTest(cros_test_lib.MockTempDirTestCase):
  """Tests for chromite.lib.user_db."""

  def _SetupDatabases(self, passwd_contents, group_contents):
    osutils.WriteFile(os.path.join(self.tempdir, 'etc', 'passwd'),
                      passwd_contents, makedirs=True)
    osutils.WriteFile(os.path.join(self.tempdir, 'etc', 'group'),
                      group_contents, makedirs=True)

  def setUp(self):
    """Set up a test environment."""
    self._SetupDatabases(MOCK_PASSWD_CONTENTS, MOCK_GROUP_CONTENTS)
    self._user_db = user_db.UserDB(self.tempdir)
    self.PatchObject(os, 'getuid', return_value=0)

  def testAcceptsKnownUser(self):
    """Check that we do appropriate things with valid users."""
    self.assertTrue(self._user_db.UserExists('root'))
    self.assertEqual(0, self._user_db.ResolveUsername('root'))

  def testAcceptsKnownGroup(self):
    """Check that we do appropriate things with valid groups."""
    self.assertTrue(self._user_db.GroupExists('root'))
    self.assertEqual(0, self._user_db.ResolveGroupname('root'))

  def testRejectsUnknownUser(self):
    """Check that we do appropriate things with invalid users."""
    self.assertFalse(self._user_db.UserExists('foot'))
    self.assertRaises(ValueError, self._user_db.ResolveUsername, 'foot')

  def testRejectsUnknownGroup(self):
    """Check that we do appropriate things with invalid groups."""
    self.assertFalse(self._user_db.GroupExists('wheel'))
    self.assertRaises(ValueError, self._user_db.ResolveGroupname, 'wheel')

  def testToleratesMalformedLines(self):
    """Check that skip over invalid lines in databases."""
    bad_user_contents = '\n'.join(['no colon on this line',
                                   '::::::',
                                   'root:x:not a uid:0:root:/root:/bin/bash',
                                   'root:x:0:not a gid:root:/root:/bin/bash',
                                   'root:x:0:0:root:/root',
                                   'root:x:0:0:root:/root:/bin/bash:',
                                   'bar:x:1:1:bar user:/home/bar:/bin/sh'])
    bad_group_contents = '\n'.join(['no colon on this line',
                                    ':::',
                                    'root:x:not a gid:',
                                    'root:x:0',
                                    'root:x:0::',
                                    'bar:x:1:'])
    self._SetupDatabases(bad_user_contents, bad_group_contents)
    db = user_db.UserDB(self.tempdir)
    self.assertTrue(db.UserExists('bar'))
    self.assertTrue(db.GroupExists('bar'))
    self.assertFalse(db.UserExists('root'))
    self.assertFalse(db.GroupExists('root'))

  def testCanAddUser(self):
    """Test that we can correctly add a user to a database."""
    new_user = user_db.User(user='foo', password='!', uid=1000, gid=1000,
                            gecos='test', home='/dev/null', shell='/bin/false')
    self.assertFalse(self._user_db.UserExists(new_user.user))
    self._user_db.AddUser(new_user)
    self.assertTrue(self._user_db.UserExists(new_user.user))

    # New instances should just see the new user.
    new_db = user_db.UserDB(self.tempdir)
    self.assertTrue(new_db.UserExists(new_user.user))

  def testCanAddGroup(self):
    """Test that we can correctly add a group to a database."""
    new_group = user_db.Group(group='foo', password='!', gid=1000, users=[])
    self.assertFalse(self._user_db.GroupExists(new_group.group))
    self._user_db.AddGroup(new_group)
    self.assertTrue(self._user_db.GroupExists(new_group.group))

    # New instances should just see the new group.
    new_db = user_db.UserDB(self.tempdir)
    self.assertTrue(new_db.GroupExists(new_group.group))
