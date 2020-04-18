# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This script installs users and groups into sysroots."""

from __future__ import print_function

import os

from chromite.lib import accounts_lib
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import osutils
from chromite.lib import sysroot_lib
from chromite.lib import user_db


ACCOUNT_DB_FILENAME = 'accounts.json'

ACTION_GET_ENTRY = 'get_entry'
ACTION_INSTALL_USER = 'install_user'
ACTION_INSTALL_GROUP = 'install_group'

USER_DB = 'passwd'
GROUP_DB = 'group'


def GetOptions(argv):
  """Returns the parsed command line arguments in |argv|."""
  parser = commandline.ArgumentParser(description=__doc__)
  command_parsers = parser.add_subparsers(dest='action')

  get_ent_parser = command_parsers.add_parser(
      ACTION_GET_ENTRY, help='Get an entry from an account database.')
  get_ent_parser.add_argument(
      '--nolock', action='store_true', default=False,
      help='Skip locking the database before reading it.')
  get_ent_parser.add_argument('sysroot', type='path',
                              help='Path to sysroot containing the database')
  get_ent_parser.add_argument('database', choices=(USER_DB, GROUP_DB),
                              help='Name of database to get')
  get_ent_parser.add_argument('name', type=str, help='Name of account to get')

  user_parser = command_parsers.add_parser(
      ACTION_INSTALL_USER, help='Install a user to a sysroot')
  user_parser.add_argument('name', type=str,
                           help='Name of user to install')
  user_parser.add_argument('--uid', type=int,
                           help='UID of the user')
  user_parser.add_argument('--shell', type='path',
                           help='Shell of user')
  user_parser.add_argument('--home', type='path',
                           help='Home directory of user')
  user_parser.add_argument('--primary_group', type=str,
                           help='Name of primary group for user')

  group_parser = command_parsers.add_parser(
      ACTION_INSTALL_GROUP, help='Install a group to a sysroot')
  group_parser.add_argument('name', type=str,
                            help='Name of group to install.')
  group_parser.add_argument('--gid', type=int, help='GID of the group')

  # Both group and user parsers need to understand the target sysroot.
  for sub_parser in (user_parser, group_parser):
    sub_parser.add_argument(
        'sysroot', type='path', help='The sysroot to install the user into')

  options = parser.parse_args(argv)
  options.Freeze()
  return options


def main(argv):
  cros_build_lib.AssertInsideChroot()
  options = GetOptions(argv)

  if options.action == ACTION_GET_ENTRY:
    db = user_db.UserDB(options.sysroot)
    if options.database == USER_DB:
      print(db.GetUserEntry(options.name, skip_lock=options.nolock))
    else:
      print(db.GetGroupEntry(options.name, skip_lock=options.nolock))
    return 0

  overlays = sysroot_lib.Sysroot(options.sysroot).GetStandardField(
      sysroot_lib.STANDARD_FIELD_PORTDIR_OVERLAY).split()

  # TODO(wiley) This process could be optimized to avoid reparsing these
  #             overlay databases each time.
  account_db = accounts_lib.AccountDatabase()
  for overlay_path in overlays:
    database_path = os.path.join(overlay_path, ACCOUNT_DB_FILENAME)
    if os.path.exists(database_path):
      account_db.AddAccountsFromDatabase(database_path)

  installed_users = user_db.UserDB(options.sysroot)

  if options.action == ACTION_INSTALL_USER:
    account_db.InstallUser(options.name, installed_users,
                           uid=options.uid, shell=options.shell,
                           homedir=options.home,
                           primary_group=options.primary_group)

    homedir = account_db.users[options.name].home
    homedir_path = os.path.join(options.sysroot, homedir)

    if homedir != '/dev/null' and not os.path.exists(homedir_path):
      osutils.SafeMakedirs(homedir_path, sudo=True)
      uid = account_db.users[options.name].uid
      cros_build_lib.SudoRunCommand(
          ['chown', '%d:%d' % (uid, uid), homedir_path], print_cmd=False)

  elif options.action == ACTION_INSTALL_GROUP:
    account_db.InstallGroup(options.name, installed_users, gid=options.gid)
  else:
    cros_build_lib.Die('Unsupported account type: %s' % options.account_type)
