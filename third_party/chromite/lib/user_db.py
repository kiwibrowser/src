# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Logic to read the set of users and groups installed on a system."""

from __future__ import print_function

import collections
import os

from chromite.lib import cros_logging as logging
from chromite.lib import locking
from chromite.lib import osutils


# These fields must be in the order expected in /etc/passwd entries.
User = collections.namedtuple(
    'User', ('user', 'password', 'uid', 'gid', 'gecos', 'home', 'shell'))

# These fields must be in the order expected in /etc/group entries.
Group = collections.namedtuple(
    'Group', ('group', 'password', 'gid', 'users'))


def UserToEntry(user):
  """Returns the database file entry corresponding to |user|."""
  return ':'.join([user.user, user.password, str(user.uid), str(user.gid),
                   user.gecos, user.home, user.shell])


def GroupToEntry(group):
  """Returns the database file entry corresponding to |group|."""
  return ':'.join([group.group, group.password,
                   str(group.gid), ','.join(group.users)])


class UserDB(object):
  """An object that understands the users and groups installed on a system."""

  # Number of times to attempt to acquire the write lock on a database.
  # The max wait time for the lock is the nth triangular number of seconds.
  # So in this case, T(24) * 1 second = 300 seconds.
  _DB_LOCK_RETRIES = 24

  def __init__(self, sysroot):
    self._sysroot = sysroot
    self._user_cache = None
    self._group_cache = None

  @property
  def _user_db_file(self):
    """Returns path to user database (aka /etc/passwd in the sysroot)."""
    return os.path.join(self._sysroot, 'etc', 'passwd')

  @property
  def _group_db_file(self):
    """Returns path to group database (aka /etc/group in the sysroot)."""
    return os.path.join(self._sysroot, 'etc', 'group')

  @property
  def _users(self):
    """Returns a list of User tuples."""
    if self._user_cache is not None:
      return self._user_cache

    self._user_cache = {}
    passwd_contents = osutils.ReadFile(self._user_db_file)

    for line in passwd_contents.splitlines():
      pieces = line.split(':')
      if len(pieces) != 7:
        logging.warning('Ignored invalid line in users file: "%s"', line)
        continue

      user, password, uid, gid, gecos, home, shell = pieces

      try:
        uid_as_int = int(uid)
        gid_as_int = int(gid)
      except ValueError:
        logging.warning('Ignored invalid uid (%s) or gid (%s).', uid, gid)
        continue

      if user in self._user_cache:
        logging.warning('Ignored duplicate user definition for "%s".', user)
        continue

      self._user_cache[user] = User(user=user, password=password,
                                    uid=uid_as_int, gid=gid_as_int,
                                    gecos=gecos, home=home, shell=shell)
    return self._user_cache

  @property
  def _groups(self):
    """Returns a list of Group tuples."""
    if self._group_cache is not None:
      return self._group_cache

    self._group_cache = {}
    group_contents = osutils.ReadFile(self._group_db_file)

    for line in group_contents.splitlines():
      pieces = line.split(':')
      if len(pieces) != 4:
        logging.warning('Ignored invalid line in group file: "%s"', line)
        continue

      group, password, gid, users = pieces

      try:
        gid_as_int = int(gid)
      except ValueError:
        logging.warning('Ignored invalid or gid (%s).', gid)
        continue

      if group in self._group_cache:
        logging.warning('Ignored duplicate group definition for "%s".',
                        group)
        continue

      users = users.split(',')

      self._group_cache[group] = Group(group=group, password=password,
                                       gid=gid_as_int, users=users)
    return self._group_cache

  def GetUserEntry(self, username, skip_lock=False):
    """Returns a user's database entry.

    Args:
      username: name of user to get the entry for.
      skip_lock: True iff we should skip getting a lock before reading the
        database.

    Returns:
      database entry as a string.
    """
    if skip_lock:
      return UserToEntry(self._users[username])

    # Clear the user cache to force ourselves to reparse while holding a lock.
    self._user_cache = None

    with locking.PortableLinkLock(
        self._user_db_file + '.lock', max_retry=self._DB_LOCK_RETRIES):
      return UserToEntry(self._users[username])

  def GetGroupEntry(self, groupname, skip_lock=False):
    """Returns a group's database entry.

    Args:
      groupname: name of group to get the entry for.
      skip_lock: True iff we should skip getting a lock before reading the
        database.

    Returns:
      database entry as a string.
    """
    if skip_lock:
      return GroupToEntry(self._groups[groupname])

    # Clear the group cache to force ourselves to reparse while holding a lock.
    self._group_cache = None

    with locking.PortableLinkLock(
        self._group_db_file + '.lock', max_retry=self._DB_LOCK_RETRIES):
      return GroupToEntry(self._groups[groupname])

  def UserExists(self, username):
    """Returns True iff a user called |username| exists in the database.

    Args:
      username: name of a user (e.g. 'root')

    Returns:
      True iff the given |username| has an entry in /etc/passwd.
    """
    return username in self._users

  def GroupExists(self, groupname):
    """Returns True iff a group called |groupname| exists in the database.

    Args:
      groupname: name of a group (e.g. 'root')

    Returns:
      True iff the given |groupname| has an entry in /etc/group.
    """
    return groupname in self._groups

  def ResolveUsername(self, username):
    """Resolves a username to a uid.

    Args:
      username: name of a user (e.g. 'root')

    Returns:
      The uid of the given username.  Raises ValueError on failure.
    """
    user = self._users.get(username)
    if user:
      return user.uid

    raise ValueError('Could not resolve unknown user "%s" to uid.' % username)

  def ResolveGroupname(self, groupname):
    """Resolves a groupname to a gid.

    Args:
      groupname: name of a group (e.g. 'wheel')

    Returns:
      The gid of the given groupname.  Raises ValueError on failure.
    """
    group = self._groups.get(groupname)
    if group:
      return group.gid

    raise ValueError('Could not resolve unknown group "%s" to gid.' % groupname)

  def AddUser(self, user):
    """Atomically add a user to the database.

    If a user named |user.user| already exists, this method will simply return.

    Args:
      user: user_db.User object to add to database.
    """
    # Try to avoid grabbing the lock in the common case that a user already
    # exists.
    if self.UserExists(user.user):
      logging.info('Not installing user "%s" because it already existed.',
                   user.user)
      return

    # Clear the user cache to force ourselves to reparse.
    self._user_cache = None

    with locking.PortableLinkLock(self._user_db_file + '.lock',
                                  max_retry=self._DB_LOCK_RETRIES):
      # Check that |user| exists under the lock in case we're racing to create
      # this user.
      if self.UserExists(user.user):
        logging.info('Not installing user "%s" because it already existed.',
                     user.user)
        return

      self._users[user.user] = user
      new_users = sorted(self._users.itervalues(), key=lambda u: u.uid)
      contents = '\n'.join([UserToEntry(u) for u in new_users])
      osutils.WriteFile(self._user_db_file, contents, atomic=True, sudo=True)
      print('Added user "%s" to %s:' % (user.user, self._user_db_file))
      print(' - password entry: %s' % user.password)
      print(' - id: %d' % user.uid)
      print(' - group id: %d' % user.gid)
      print(' - gecos: %s' % user.gecos)
      print(' - home: %s' % user.home)
      print(' - shell: %s' % user.shell)

  def AddGroup(self, group):
    """Atomically add a group to the database.

    If a group named |group.group| already exists, this method will simply
    return.

    Args:
      group: user_db.Group object to add to database.
    """
    # Try to avoid grabbing the lock in the common case that a group already
    # exists.
    if self.GroupExists(group.group):
      logging.info('Not installing group "%s" because it already existed.',
                   group.group)
      return

    # Clear the group cache to force ourselves to reparse.
    self._group_cache = None

    with locking.PortableLinkLock(self._group_db_file + '.lock',
                                  max_retry=self._DB_LOCK_RETRIES):
      # Check that |group| exists under the lock in case we're racing to create
      # this group.
      if self.GroupExists(group.group):
        logging.info('Not installing group "%s" because it already existed.',
                     group.group)
        return

      self._groups[group.group] = group
      new_groups = sorted(self._groups.itervalues(), key=lambda g: g.gid)
      contents = '\n'.join([GroupToEntry(g) for g in new_groups])
      osutils.WriteFile(self._group_db_file, contents, atomic=True, sudo=True)
      print('Added group "%s" to %s:' % (group.group, self._group_db_file))
      print(' - group id: %d' % group.gid)
      print(' - password entry: %s' % group.password)
      print(' - user list: %s' % ','.join(group.users))
