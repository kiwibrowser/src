# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for accounts_lib."""

from __future__ import print_function

import json
import mock

from chromite.lib import accounts_lib
from chromite.lib import cros_test_lib
from chromite.lib import osutils
from chromite.lib import user_db


EMPTY_ACCOUNTS_DB_WITH_COMMENTS = """
{
  # This accounts spec is empty.
  "users": [
  ],
  "groups": [
  ]
}
"""

MINIMAL_DB_USER = accounts_lib.User(
    name='minimal', password='!', uid=1000, group_name='minimal',
    description='', home='/dev/null', shell='/bin/false',
    is_fixed_id=False, is_defunct=False)
MINIMAL_DB_GROUP = accounts_lib.Group(
    name='minimal', password='!', gid=1000, users=['minimal'],
    is_fixed_id=False, is_defunct=False)

MINIMAL_ACCOUNTS_DB = """
{
  "users": [
    {
      # Minimal user.
      "user": "minimal",
      "uid": 1000,
      "group_name": "minimal"
    }
  ],
  "groups": [
    {
      # Minimal group.
      "group": "minimal",
      "gid": 1000,
      "users": [ "minimal" ]
    }
  ]
}
"""

EXTRA_USER_SPEC_FIELD_DB = """
{
  "users": [
    {
      "user": "minimal",
      "uid": 1000,
      "group_name": "minimal",
      "gecos": "minimal user spec",
      "extra": "This field is not expected."
    }
  ]
}
"""

class AccountDatabaseTest(cros_test_lib.MockTestCase):
  """Tests for chromite.lib.accounts_lib.AccountDatabase."""

  def _ParseSpec(self, contents, db=None):
    """Return a AccountDatabase that has read a file with |contents|.

    Args:
      contents: desired contents of accounts database to parse.
      db: existing account db to override with new definitions.

    Returns:
      an instance of AccountDatabase.
    """
    if db is None:
      db = accounts_lib.AccountDatabase()
    with self.PatchObject(osutils, 'ReadFile', return_value=contents):
      db.AddAccountsFromDatabase('ignored')
    return db

  def _ParseSpecs(self, specs):
    """Return a AccountDatabase based on the account database stack in |specs|.

    Args:
      specs: list of json fragments (encoded as strings) to compose into a
          consistent account database.  This list is assumed to be in
          increasing priority order so that later entries override earlier
          entries.

    Returns:
      an instance of AccountDatabase.
    """
    db = accounts_lib.AccountDatabase()
    for spec in specs:
      self._ParseSpec(spec, db=db)
    return db

  def testParsesEmptyDb(self):
    """Test that we can parse an empty database."""
    self._ParseSpec(json.dumps({}))

  def testParsesDbWithComments(self):
    """Test that we handle comments properly."""
    self._ParseSpec(EMPTY_ACCOUNTS_DB_WITH_COMMENTS)

  def testRejectsUnkownDbKeys(self):
    """Test that we check the set of keys specified in the account database."""
    self.assertRaises(ValueError,
                      self._ParseSpec,
                      json.dumps({'foo': 'This is not a valid field.'}))

  def testRejectsBadKeyValues(self):
    """Check that typecheck user/group specs."""
    self.assertRaises(ValueError,
                      self._ParseSpec,
                      json.dumps({'users': 'This should be a list'}))
    self.assertRaises(ValueError,
                      self._ParseSpec,
                      json.dumps({'groups': 'This should be a list'}))

  def testRejectsExtraUserSpecFields(self):
    """Test that we check for extra user spec fields."""
    self.assertRaises(ValueError, self._ParseSpec, EXTRA_USER_SPEC_FIELD_DB)

  def testParsesMinimalDb(self):
    """Test that we can parse a basic database."""
    db = self._ParseSpec(MINIMAL_ACCOUNTS_DB)
    self.assertEqual(1, len(db.users.keys()))
    self.assertEqual(1, len(db.groups.keys()))
    self.assertIn(MINIMAL_DB_USER.name, db.users)
    self.assertIn(MINIMAL_DB_GROUP.name, db.groups)
    self.assertEqual(db.users[MINIMAL_DB_USER.name], MINIMAL_DB_USER)
    self.assertEqual(db.groups[MINIMAL_DB_GROUP.name], MINIMAL_DB_GROUP)

  def testComposesDbs(self):
    """Test that we can compose databases from multiple overlays."""
    BASE_ID = 1000
    OVERRIDE_ID = 2000
    BASE_NAME = 'base'
    OVERRIDE_NAME = 'override'
    EXTRA_USER = 'extra.user'
    base_db = json.dumps({
        'users': [
            {'user': BASE_NAME,
             'uid': BASE_ID,
             'group_name': 'base.group',
            },
            {'user': OVERRIDE_NAME,
             'uid': OVERRIDE_ID - 1,
             'group_name': 'override.group',
            },
        ],
        'groups': [
            {'group': BASE_NAME,
             'gid': BASE_ID,
             'users': ['base.user']
            },
            {'group': OVERRIDE_NAME,
             'gid': OVERRIDE_ID - 1,
             'users': ['override.user']
            },
        ],
    })
    override_db = json.dumps({
        'users': [
            {'user': OVERRIDE_NAME,
             'uid': OVERRIDE_ID,
             'group_name': 'override.group',
            },
            {'user': EXTRA_USER,
             'uid': 3000,
             'group_name': OVERRIDE_NAME,
            },
        ],
        'groups': [
            {'group': OVERRIDE_NAME,
             'gid': OVERRIDE_ID,
             'users': [OVERRIDE_NAME, EXTRA_USER],
            },
        ],
    })
    db = self._ParseSpecs([base_db, override_db])
    self.assertEqual(3, len(db.users))
    self.assertEqual(2, len(db.groups))
    self.assertEqual(BASE_ID, db.users[BASE_NAME].uid)
    self.assertEqual(BASE_ID, db.groups[BASE_NAME].gid)
    self.assertEqual(OVERRIDE_ID, db.users[OVERRIDE_NAME].uid)
    self.assertEqual(OVERRIDE_ID, db.groups[OVERRIDE_NAME].gid)
    self.assertEqual(sorted([OVERRIDE_NAME, EXTRA_USER]),
                     sorted(db.groups[OVERRIDE_NAME].users))

  def testInstallUser(self):
    """Test that we can install a user correctly."""
    db = self._ParseSpec(MINIMAL_ACCOUNTS_DB)
    mock_user_db = mock.MagicMock()
    db.InstallUser(MINIMAL_DB_USER.name, mock_user_db)
    installed_user = user_db.User(
        user=MINIMAL_DB_USER.name, password=MINIMAL_DB_USER.password,
        uid=MINIMAL_DB_USER.uid, gid=MINIMAL_DB_GROUP.gid,
        gecos=MINIMAL_DB_USER.description, home=MINIMAL_DB_USER.home,
        shell=MINIMAL_DB_USER.shell)
    self.assertEqual([mock.call.AddUser(installed_user)],
                     mock_user_db.mock_calls)

  def testInstallGroup(self):
    """Test that we can install a group correctly."""
    db = self._ParseSpec(MINIMAL_ACCOUNTS_DB)
    mock_user_db = mock.MagicMock()
    db.InstallGroup(MINIMAL_DB_GROUP.name, mock_user_db)
    installed_group = user_db.Group(
        group=MINIMAL_DB_GROUP.name, password=MINIMAL_DB_GROUP.password,
        gid=MINIMAL_DB_GROUP.gid, users=MINIMAL_DB_GROUP.users)
    self.assertEqual([mock.call.AddGroup(installed_group)],
                     mock_user_db.mock_calls)
