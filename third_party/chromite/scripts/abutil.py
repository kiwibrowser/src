# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tool to fetch and manipulate ab:// URLs."""

from __future__ import print_function

from chromite.lib import androidbuild
from chromite.lib import commandline


def CmdFetch(ab_client, opts):
  """Implements the `abutil fetch ab://...` subcommand.

  Args:
    ab_client: The androidbuild API client.
    opts: The command line arguments, result of ArgumentParser's parse_args.

  Returns:
    The return status for the command. None or 0 for success.
  """
  branch, target, build_id, filepath = androidbuild.SplitAbUrl(opts.url)
  if not filepath:
    raise ValueError('Invalid URL [%s] must specify a filepath.' % opts.url)
  androidbuild.FetchArtifact(
      ab_client,
      branch,
      target,
      build_id,
      filepath,
      opts.output)


def CmdListBuilds(ab_client, opts):
  """Implements the `abutil list-builds ab://...` subcommand.

  Args:
    ab_client: The androidbuild API client.
    opts: The command line arguments, result of ArgumentParser's parse_args.

  Returns:
    The return status for the command. None or 0 for success.
  """
  branch, target, build_id, filepath = androidbuild.SplitAbUrl(opts.url)
  if build_id or filepath:
    raise ValueError('Invalid URL [%s] must only have branch and target.' %
                     opts.url)
  for build_id in androidbuild.FindRecentBuilds(ab_client, branch, target):
    print(build_id)


def CmdLatestGreen(ab_client, opts):
  """Implements the `abutil latest-green ab://...` subcommand.

  Args:
    ab_client: The androidbuild API client.
    opts: The command line arguments, result of ArgumentParser's parse_args.

  Returns:
    The return status for the command. None or 0 for success.
  """
  branch, target, build_id, filepath = androidbuild.SplitAbUrl(opts.url)
  if build_id or filepath:
    raise ValueError('Invalid URL [%s] must only have branch and target.' %
                     opts.url)
  build_id = androidbuild.FindLatestGreenBuildId(ab_client, branch, target)
  if build_id is None:
    return 1
  print(build_id)


def GetParser():
  """Creates the argparse parser for the `abutil` subcommands."""
  parser = commandline.ArgumentParser(
      description=__doc__,
      default_log_level='notice')

  # Global options.
  parser.add_argument(
      '--json-key-file', type='path',
      help='Apiary JSON file for authorization.')

  # Subcommands.
  subcommands = parser.add_subparsers(dest='command')

  subparser = subcommands.add_parser(
      'fetch',
      help='Fetch an image from androidbuild.')
  subparser.set_defaults(func=CmdFetch)
  subparser.add_argument(
      'url', type='ab_url',
      help='ab:// URL to fetch.')
  subparser.add_argument(
      'output', type='path', nargs='?',
      help='Path where to save the fetched file.')

  subparser = subcommands.add_parser(
      'list-builds',
      help='List available builds for a branch/target combination.')
  subparser.set_defaults(func=CmdListBuilds)
  subparser.add_argument(
      'url', type='ab_url',
      help='ab:// URL with branch and target to list.')

  subparser = subcommands.add_parser(
      'latest-green',
      help='List latest green build for a branch/target.')
  subparser.set_defaults(func=CmdLatestGreen)
  subparser.add_argument(
      'url', type='ab_url',
      help='ab:// URL with branch and target to list.')

  return parser


def main(argv):
  parser = GetParser()
  opts = parser.parse_args(argv)
  opts.Freeze()
  creds = androidbuild.LoadCredentials(opts.json_key_file)
  ab_client = androidbuild.GetApiClient(creds)
  return opts.func(ab_client, opts)
