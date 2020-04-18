# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Find suite ids corresponding to builds.."""

from __future__ import print_function

import json
import re
import sys

from chromite.cbuildbot import topology
from chromite.cli.cros import cros_cidbcreds
from chromite.lib import cidb
from chromite.lib import commandline
from chromite.lib import cros_logging as logging
from chromite.lib import milo


SUITE_RE = re.compile(
    r'http://cautotest.corp.google.com/afe/#tab_id=view_job&object_id=(\d+)')


def GetParser():
  """Creates the argparse parser."""
  parser = commandline.ArgumentParser(description=__doc__)
  parser.add_argument('--service_acct_json', type=str, action='store',
                      help='Path to service account credentials JSON file.')
  parser.add_argument('build_ids', type=str, nargs='*', action='store',
                      help='Build ids to report on.')
  parser.add_argument('--cred_dir', type=str, action='store',
                      metavar='CIDB_CREDENTIALS_DIR',
                      help='Database credentials directory with certificates '
                           'and other connection information. Obtain your '
                           'credentials at go/cros-cidb-admin .')
  parser.add_argument('--build_config', type=str, action='store',
                      help='Build config to report on.')
  parser.add_argument('--num_builds', type=int, action='store', default=1,
                      help='Number of master builds to gather.')
  parser.add_argument('--milo_host', type=str, action='store',
                      help='URL of MILO host.')
  parser.add_argument('--output', '-o', type=str, action='store',
                      help='Filename to write to.')
  parser.add_argument('--json', action='store_true',
                      help='Output as JSON.')
  parser.add_argument('--allow_empty', action='store_true',
                      help='Include builds with no suites.')
  parser.add_argument('--no_suites', action='store_true',
                      help='Do not include list of suites.')
  return parser


def GetSuites(milo_client, waterfall, builder_name, build_number):
  """Gets a list of suites ids for a given build from Milo.

  Args:
    milo_client: MiloClient object.
    waterfall: Buildbot waterfall.
    builder_name: Buildbot builder name.
    build_number: Buidlbot build number.

  Returns:
    A set of suite ids.
  """
  buildinfo = milo_client.BuildInfoGetBuildbot(waterfall, builder_name,
                                               build_number)

  suite_ids = set()
  for step in buildinfo['steps']:
    for link in buildinfo['steps'][step].get('otherLinks', []):
      if link.get('label') == 'Link to suite':
        url = link.get('url')
        m = SUITE_RE.search(url)
        if m:
          suite_ids.add(m.group(1))
        else:
          logging.error('Unable to parse suite link for %s: %s' %
                        (buildinfo['steps'][step]['name'], url))

  return suite_ids


def MakeBuildEntry(db, milo_client, build_id,
                   build_status=None, no_suites=False):
  """Generates an entry for a single build.

  Args:
    db: CIDB DB connection.
    milo_client: MiloClient object.
    build_id: The CIDB build ID.
    build_status: CIDB build status dictionary.
    no_suites: Boolean indiciating if suites do not need to be included.

  Returns:
    Dictionary for the build.
  """
  if build_status is None:
    build_status = db.GetBuildStatus(build_id)

  waterfall = build_status['waterfall']
  builder_name = build_status['builder_name']
  build_number = build_status['build_number']

  build_entry = {
      'build_id': build_status['id'],
      'waterfall': waterfall,
      'builder_name': builder_name,
      'build_number': build_number,
  }
  if not no_suites:
    build_entry['suite_ids'] = list(GetSuites(milo_client, waterfall,
                                              builder_name, build_number))

  logging.debug(StringifyBuildEntry(build_entry))
  return build_entry


def StringifyBuildEntry(entry):
  """Pretty print a build entry.

  Args:
    entry: a build entry from MakeBuildEntry.

  Returns:
    A printable string.
  """
  return '%s %s %d: %s' % (entry['build_id'], entry['builder_name'],
                           entry['build_number'],
                           ' '.join(entry.get('suite_ids', [])))


def main(argv):
  # Parse command line arguments.
  parser = GetParser()
  options = parser.parse_args(argv)

  # Set up clients.
  credentials = options.cred_dir or cros_cidbcreds.CheckAndGetCIDBCreds()
  db = cidb.CIDBConnection(credentials)
  topology.FetchTopologyFromCIDB(db)
  milo_client = milo.MiloClient(options.service_acct_json,
                                host=options.milo_host)

  builds = []

  # Add explicitly requested builds.
  if options.build_ids:
    for build_id in options.build_ids:
      builds.append(MakeBuildEntry(db, milo_client, build_id,
                                   no_suites=options.no_suites))

  # Search for builds by build config.
  if options.build_config:
    masters = db.GetBuildHistory(options.build_config, options.num_builds,
                                 final=True)
    for master in masters:
      builds.append(MakeBuildEntry(db, milo_client, master['id'], master,
                                   no_suites=options.no_suites))
      statuses = db.GetSlaveStatuses(master['id'])
      for slave in statuses:
        builds.append(MakeBuildEntry(db, milo_client, slave['id'], slave,
                                     no_suites=options.no_suites))

  if not options.allow_empty and not options.no_suites:
    builds = [b for b in builds if len(b.get('suite_ids', []))]

  # Output results.
  with open(options.output, 'w') if options.output else sys.stdout as f:
    if options.json:
      output = {
          'builds': builds,
      }
      json.dump(output, f)
    else:
      for b in builds:
        f.write(StringifyBuildEntry(b))
        f.write('\n')
