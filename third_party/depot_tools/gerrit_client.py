#!/usr/bin/python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Simple client for the Gerrit REST API.

Example usage:
  ./gerrit_client.py [command] [args]""
"""

from __future__ import print_function

import json
import logging
import optparse
import subcommand
import sys
import urllib
import urlparse

from third_party import colorama
import fix_encoding
import gerrit_util
import setup_color

__version__ = '0.1'
# Shortcut since it quickly becomes redundant.
Fore = colorama.Fore


def write_result(result, opt):
  if opt.json_file:
    with open(opt.json_file, 'w') as json_file:
      json_file.write(json.dumps(result))


@subcommand.usage('[args ...]')
def CMDbranchinfo(parser, args):
  parser.add_option('--branch', dest='branch', help='branch name')

  (opt, args) = parser.parse_args(args)
  host = urlparse.urlparse(opt.host).netloc
  project = urllib.quote_plus(opt.project)
  branch = urllib.quote_plus(opt.branch)
  result = gerrit_util.GetGerritBranch(host, project, branch)
  logging.info(result)
  write_result(result, opt)

@subcommand.usage('[args ...]')
def CMDbranch(parser, args):
  parser.add_option('--branch', dest='branch', help='branch name')
  parser.add_option('--commit', dest='commit', help='commit hash')

  (opt, args) = parser.parse_args(args)

  project = urllib.quote_plus(opt.project)
  host = urlparse.urlparse(opt.host).netloc
  branch = urllib.quote_plus(opt.branch)
  commit = urllib.quote_plus(opt.commit)
  result = gerrit_util.CreateGerritBranch(host, project, branch, commit)
  logging.info(result)
  write_result(result, opt)


@subcommand.usage('[args ...]')
def CMDchanges(parser, args):
  parser.add_option('-p', '--param', dest='params', action='append',
                    help='repeatable query parameter, format: -p key=value')
  parser.add_option('-o', '--o-param', dest='o_params', action='append',
                    help='gerrit output parameters, e.g. ALL_REVISIONS')
  parser.add_option('--limit', dest='limit', type=int,
                    help='maximum number of results to return')
  parser.add_option('--start', dest='start', type=int,
                    help='how many changes to skip '
                         '(starting with the most recent)')

  (opt, args) = parser.parse_args(args)

  result = gerrit_util.QueryChanges(
      urlparse.urlparse(opt.host).netloc,
      list(tuple(p.split('=', 1)) for p in opt.params),
      start=opt.start,        # Default: None
      limit=opt.limit,        # Default: None
      o_params=opt.o_params,  # Default: None
  )
  logging.info('Change query returned %d changes.', len(result))
  write_result(result, opt)


class OptionParser(optparse.OptionParser):
  """Creates the option parse and add --verbose support."""
  def __init__(self, *args, **kwargs):
    optparse.OptionParser.__init__(
        self, *args, prog='git cl', version=__version__, **kwargs)
    self.add_option(
        '--verbose', action='count', default=0,
        help='Use 2 times for more debugging info')
    self.add_option('--host', dest='host', help='Url of host.')
    self.add_option('--project', dest='project', help='project name')
    self.add_option(
        '--json_file', dest='json_file', help='output json filepath')

  def parse_args(self, args=None, values=None):
    options, args = optparse.OptionParser.parse_args(self, args, values)
    levels = [logging.WARNING, logging.INFO, logging.DEBUG]
    logging.basicConfig(level=levels[min(options.verbose, len(levels) - 1)])
    return options, args


def main(argv):
  if sys.hexversion < 0x02060000:
    print('\nYour python version %s is unsupported, please upgrade.\n'
          %(sys.version.split(' ', 1)[0],),
          file=sys.stderr)
    return 2
  dispatcher = subcommand.CommandDispatcher(__name__)
  return dispatcher.execute(OptionParser(), argv)


if __name__ == '__main__':
  # These affect sys.stdout so do it outside of main() to simplify mocks in
  # unit testing.
  fix_encoding.fix_encoding()
  setup_color.init()
  try:
    sys.exit(main(sys.argv[1:]))
  except KeyboardInterrupt:
    sys.stderr.write('interrupted\n')
    sys.exit(1)
