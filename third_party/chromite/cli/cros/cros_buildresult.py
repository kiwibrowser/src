# -*- coding: utf-8 -*-
# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""cros buildresult: Look up results for a single build."""

from __future__ import print_function

import datetime
import json
import os

from chromite.cli import command
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.cli.cros import cros_cidbcreds
from chromite.lib import cros_logging as logging


_FINISHED_STATUSES = (
    'fail', 'pass', 'missing', 'aborted', 'skipped', 'forgiven')


def FetchBuildStatuses(db, options):
  """Fetch the requested build statuses.

  The results are NOT filtered or fixed up.

  Args:
    db: CIDBConnection.
    options: Parsed command line options set.

  Returns:
    List of build_status dicts from CIDB, or None.
  """
  if options.buildbucket_id:
    build_status = db.GetBuildStatusWithBuildbucketId(
        options.buildbucket_id)
    if build_status:
      return [build_status]
  elif options.cidb_id:
    build_status = db.GetBuildStatus(options.cidb_id)
    if build_status:
      return [build_status]
  elif options.build_config:
    start_date = options.start_date or options.date
    end_date = options.end_date or options.date
    return db.GetBuildHistory(
        options.build_config, db.NUM_RESULTS_NO_LIMIT,
        start_date=start_date, end_date=end_date)
  else:
    cros_build_lib.Die('You must specify which builds.')


def IsBuildStatusFinished(build_status):
  """Populates the 'artifacts_url' and 'stages' build_status fields.

  Args:
    db: CIDBConnection.
    build_status: Single build_status dict returned by any Fetch method.

  Returns:
    build_status dict with additional fields populated.
  """
  return build_status['status'] in _FINISHED_STATUSES


def FixUpBuildStatus(db, build_status):
  """Add 'extra' build_status values we need.

  Populates the 'artifacts_url' and 'stages' build_status fields.

  Args:
    db: CIDBConnection.
    build_status: Single build_status dict returned by any Fetch method.

  Returns:
    build_status dict with additional fields populated.
  """
  # We don't actually store the artifacts_url, but we store a URL for a specific
  # artifact we can use to derive it.
  build_status['artifacts_url'] = None
  if build_status['metadata_url']:
    build_status['artifacts_url'] = os.path.dirname(
        build_status['metadata_url'])

  # Find stage information.
  build_status['stages'] = db.GetBuildStages(build_status['id'])

  return build_status


def Report(build_statuses):
  """Generate the stdout description of a given build.

  Args:
    build_statuses: List of build_status dict's from FetchBuildStatus.

  Returns:
    str to display as the final report.
  """
  result = ''

  for build_status in build_statuses:
    result += '\n'.join([
        'cidb_id: %s' % build_status['id'],
        'buildbucket_id: %s' % build_status['buildbucket_id'],
        'status: %s' % build_status['status'],
        'artifacts_url: %s' % build_status['artifacts_url'],
        'toolchain_url: %s' % build_status['toolchain_url'],
        'stages:\n'
    ])
    for stage in build_status['stages']:
      result += '  %s: %s\n' % (stage['name'], stage['status'])
    result += '\n'  # Blank line between builds.

  return result


def ReportJson(build_statuses):
  """Generate the json description of a given build.

  Args:
    build_statuses: List of build_status dict's from FetchBuildStatus.

  Returns:
    str to display as the final report.
  """
  report = {}

  for build_status in build_statuses:
    report[build_status['buildbucket_id']] = {
        'cidb_id': build_status['id'],
        'buildbucket_id': build_status['buildbucket_id'],
        'status': build_status['status'],
        'stages': {s['name']: s['status'] for s in build_status['stages']},
        'artifacts_url': build_status['artifacts_url'],
        'toolchain_url': build_status['toolchain_url'],
    }

  return json.dumps(report)


@command.CommandDecorator('buildresult')
class BuildResultCommand(command.CliCommand):
  """Script that looks up results of finished builds."""

  EPILOG = """
Look up a single build result:
  cros buildresult --buildbucket-id 1234567890123
  cros buildresult --cidb-id 1234

Look up results by build config name:
  cros buildresult --build-config samus-pre-cq
  cros buildresult --build-config samus-pre-cq --date 2018-1-2
  cros buildresult --build-config samus-pre-cq \
      --start-date 2018-1-2 --end-date 2018-1-7

Output can be json formatted with:
  cros buildresult --buildbucket-id 1234567890123 --report json

Note:
  This tool does NOT work for master-*-tryjob, precq-launcher-try, or
  builds on branches older than CL:942097.

Note:
  Exit code 1: A script error or bad options combination.
  Exit code 2: No matching finished builds were found.
"""

  def __init__(self, options):
    super(BuildResultCommand, self).__init__(options)

  @classmethod
  def AddParser(cls, parser):
    super(cls, BuildResultCommand).AddParser(parser)

    # CIDB access credentials.
    creds_args = parser.add_argument_group()

    creds_args.add_argument('--cred-dir', type='path',
                            metavar='CIDB_CREDENTIALS_DIR',
                            help='Database credentials directory with'
                                 ' certificates and other connection'
                                 ' information. Obtain your credentials'
                                 ' at go/cros-cidb-admin .')

    creds_args.add_argument('--update-cidb-creds', dest='force_update',
                            action='store_true',
                            help='force updating the cidb credentials.')

    # What build do we report on?
    request_group = parser.add_mutually_exclusive_group()

    request_group.add_argument(
        '--buildbucket-id', help='Buildbucket ID of build to look up.')
    request_group.add_argument(
        '--cidb-id', help='CIDB ID of the build to look up.')
    request_group.add_argument(
        '--build-config', help='')

    #
    date_group = parser.add_argument_group()

    date_group.add_argument(
        '--date', action='store', type='date', default=datetime.date.today(),
        help='Request all finished builds on a given day. Default today.')

    date_group.add_argument(
        '--start-date', action='store', type='date', default=None,
        help='Request all builds between (inclusive) start and end dates.')

    date_group.add_argument(
        '--end-date', action='store', type='date', default=None,
        help='End of date range (inclusive) specified by --start-date.')

    # What kind of report do we generate?
    parser.add_argument('--report', default='standard',
                        choices=['standard', 'json'],
                        help='What format is the output in?')

  def Run(self):
    """Run cros buildresult."""
    self.options.Freeze()

    commandline.RunInsideChroot(self)

    credentials = self.options.cred_dir
    if not credentials:
      credentials = cros_cidbcreds.CheckAndGetCIDBCreds(
          force_update=self.options.force_update)

    # Delay import so sqlalchemy isn't pulled in until we need it.
    from chromite.lib import cidb

    db = cidb.CIDBConnection(credentials)

    build_statuses = FetchBuildStatuses(db, self.options)

    if build_statuses:
      # Filter out builds that don't exist in CIDB, or which aren't finished.
      build_statuses = [b for b in build_statuses if IsBuildStatusFinished(b)]

    # If we found no builds at all, return a different exit code to help
    # automated scripts know they should try waiting longer.
    if not build_statuses:
      logging.error('No build found. Perhaps not started?')
      return 2

    # Fixup all of the builds we have.
    build_statuses = [FixUpBuildStatus(db, b) for b in build_statuses]

    # Produce our final result.
    if self.options.report == 'json':
      report = ReportJson(build_statuses)
    else:
      report = Report(build_statuses)

    print(report)
