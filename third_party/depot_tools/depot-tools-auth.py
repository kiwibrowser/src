#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Manages cached OAuth2 tokens used by other depot_tools scripts.

Usage:
  depot-tools-auth login codereview.chromium.org
  depot-tools-auth info codereview.chromium.org
  depot-tools-auth logout codereview.chromium.org
"""

import logging
import optparse
import sys
import os

import auth
import setup_color
import subcommand

__version__ = '1.0'


@subcommand.usage('<hostname>')
def CMDlogin(parser, args):
  """Performs interactive login and caches authentication token."""
  # Forcefully relogin, revoking previous token.
  hostname, authenticator = parser.parse_args(args)
  authenticator.logout()
  authenticator.login()
  print_token_info(hostname, authenticator)
  return 0


@subcommand.usage('<hostname>')
def CMDlogout(parser, args):
  """Revokes cached authentication token and removes it from disk."""
  _, authenticator = parser.parse_args(args)
  done = authenticator.logout()
  print 'Done.' if done else 'Already logged out.'
  return 0


@subcommand.usage('<hostname>')
def CMDinfo(parser, args):
  """Shows email associated with a cached authentication token."""
  # If no token is cached, AuthenticationError will be caught in 'main'.
  hostname, authenticator = parser.parse_args(args)
  print_token_info(hostname, authenticator)
  return 0


def print_token_info(hostname, authenticator):
  token_info = authenticator.get_token_info()
  print 'Logged in to %s as %s.' % (hostname, token_info['email'])
  print ''
  print 'To login with a different email run:'
  print '  depot-tools-auth login %s' % hostname
  print 'To logout and purge the authentication token run:'
  print '  depot-tools-auth logout %s' % hostname


class OptionParser(optparse.OptionParser):
  def __init__(self, *args, **kwargs):
    optparse.OptionParser.__init__(
        self, *args, prog='depot-tools-auth', version=__version__, **kwargs)
    self.add_option(
        '-v', '--verbose', action='count', default=0,
        help='Use 2 times for more debugging info')
    auth.add_auth_options(self, auth.make_auth_config(use_oauth2=True))

  def parse_args(self, args=None, values=None):
    """Parses options and returns (hostname, auth.Authenticator object)."""
    options, args = optparse.OptionParser.parse_args(self, args, values)
    levels = [logging.WARNING, logging.INFO, logging.DEBUG]
    logging.basicConfig(level=levels[min(options.verbose, len(levels) - 1)])
    auth_config = auth.extract_auth_config_from_options(options)
    if len(args) != 1:
      self.error('Expecting single argument (hostname).')
    if not auth_config.use_oauth2:
      self.error('This command is only usable with OAuth2 authentication')
    return args[0], auth.get_authenticator_for_host(args[0], auth_config)


def main(argv):
  dispatcher = subcommand.CommandDispatcher(__name__)
  try:
    return dispatcher.execute(OptionParser(), argv)
  except auth.AuthenticationError as e:
    print >> sys.stderr, e
    return 1


if __name__ == '__main__':
  setup_color.init()
  try:
    sys.exit(main(sys.argv[1:]))
  except KeyboardInterrupt:
    sys.stderr.write('interrupted\n')
    sys.exit(1)
