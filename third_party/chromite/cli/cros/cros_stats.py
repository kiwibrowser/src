# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""cros stats: Generate stats reports from CIDB."""

from __future__ import print_function

import datetime
import sys

from chromite.lib import build_time_stats
from chromite.cli import command
from chromite.lib import commandline
from chromite.cli.cros import cros_cidbcreds  # TODO: Move into lib???
from chromite.lib import cros_logging as logging


@command.CommandDecorator('stats')
class StatsCommand(command.CliCommand):
  """Script that shows build timing for a build, and it's stages."""

  def __init__(self, options):
    super(StatsCommand, self).__init__(options)

  @classmethod
  def AddParser(cls, parser):
    super(cls, StatsCommand).AddParser(parser)

    stats_args = parser.add_argument_group()

    stats_args.add_argument('--cred-dir', type='path',
                            metavar='CIDB_CREDENTIALS_DIR',
                            help='Database credentials directory with'
                                 ' certificates and other connection'
                                 ' information. Obtain your credentials'
                                 ' at go/cros-cidb-admin .')

    stats_args.add_argument('--start-date', action='store', type='date',
                            default=None,
                            help='Limit scope to a start date in the past.')
    stats_args.add_argument('--end-date', action='store', type='date',
                            default=None,
                            help='Limit scope to an end date in the past.')

    stats_args.add_argument('--csv', action='store_true', default=False,
                            help='Output in CSV format.')

    stats_args.add_argument('--stages', action='store_true', default=True,
                            help='Do not show stats for all stages run.')
    stats_args.add_argument('--nostages', dest='stages', action='store_false',
                            help='Do not show stats for all stages run.')
    stats_args.add_argument('--update-cidb-creds', dest='force_update',
                            action='store_true',
                            help='force updating the cidb credentials.')

    # Which builds are we generating a report for?
    ex_group = parser.add_mutually_exclusive_group(required=True)

    # Ask for pull out a single build id to compare against other values.
    ex_group.add_argument('--build-id', action='store', type=int, default=None,
                          help="Single build with comparison to 'normal'.")

    # Look at all builds for a build_config.
    ex_group.add_argument('--build-config', action='store', default=None,
                          help='Build config to gather stats for.')

    # Look at all builds for a master/slave group.
    ex_group.add_argument('--build-type', default=None,
                          choices=build_time_stats.BUILD_TYPE_MAP.keys(),
                          help='Build master/salves to gather stats for: cq.')

    # How many builds are included, for builder/build-types.
    start_group = parser.add_mutually_exclusive_group()
    start_group.add_argument('--month', action='store_true', default=False,
                             help='Limit scope to the past 30 days.')
    start_group.add_argument('--week', action='store_true', default=False,
                             help='Limit scope to the past week.')
    start_group.add_argument('--day', action='store_true', default=False,
                             help='Limit scope to the past day.')
    start_group.add_argument('--trending', action='store_true', default=False,
                             help='Show stats for all builds by month.')

    # What kind of report do we generate?
    report_group = parser.add_mutually_exclusive_group()
    report_group.add_argument('--report', default='standard',
                              choices=['standard', 'stability', 'success',
                                       'stage-stability'],
                              help='What type of report to generate?')


  @staticmethod
  def OptionsToStartEndDates(options):
    if options.start_date:
      # If a start date is provided, --week, etc extend forward if provided.
      start_date = options.start_date
      if options.end_date:
        end_date = options.end_date
      elif options.month:
        end_date = start_date + datetime.timedelta(days=30)
      elif options.day:
        end_date = start_date + datetime.timedelta(days=1)
      else:  # Default to week.
        end_date = start_date + datetime.timedelta(days=7)
    else:
      # Otherwise --week, etc extend backwards from the end date or now().
      end_date = options.end_date or datetime.datetime.now().date()
      if options.month:
        start_date = end_date - datetime.timedelta(days=30)
      elif options.day:
        start_date = end_date - datetime.timedelta(days=1)
      elif options.trending:
        start_date, end_date = None, None
      else:  # Default of past_week.
        start_date = end_date - datetime.timedelta(days=7)

    return start_date, end_date


  def Run(self):
    """Run cros build."""
    self.options.Freeze()

    commandline.RunInsideChroot(self)

    credentials = self.options.cred_dir
    if not credentials:
      credentials = cros_cidbcreds.CheckAndGetCIDBCreds(
          force_update=self.options.force_update)

    # Delay import so sqlalchemy isn't pulled in until we need it.
    from chromite.lib import cidb

    db = cidb.CIDBConnection(credentials)

    # Timeframe for discovering builds, if options.build_id not used.
    start_date, end_date = self.OptionsToStartEndDates(self.options)

    # Trending is sufficiently different to be handled on it's own.
    if not self.options.trending and self.options.report != 'success':
      assert not self.options.csv, (
          '--csv can only be used with --trending or --report success.')

    # Data about a single build (optional).
    focus_build = None

    if self.options.build_id:
      logging.info('Gathering data for %s', self.options.build_id)
      focus_status = build_time_stats.BuildIdToBuildStatus(
          db, self.options.build_id)
      focus_build = build_time_stats.GetBuildTimings(focus_status)

      build_config = focus_status['build_config']
      builds_statuses = build_time_stats.BuildConfigToStatuses(
          db, build_config, start_date, end_date)
      description = 'Focus %d - %s' % (self.options.build_id, build_config)

    elif self.options.build_config:
      builds_statuses = build_time_stats.BuildConfigToStatuses(
          db, self.options.build_config, start_date, end_date)
      description = 'Config %s' % self.options.build_config

    elif self.options.build_type:
      builds_statuses = build_time_stats.MasterConfigToStatuses(
          db, build_time_stats.BUILD_TYPE_MAP[self.options.build_type],
          start_date, end_date)
      description = 'Type %s' % self.options.build_type

    if not builds_statuses:
      logging.critical('No Builds Found For: %s', description)
      return 1

    if self.options.report == 'success':
      # Calculate per-build success rates and per-stage success rates.
      build_success_rates = build_time_stats.GetBuildSuccessRates(
          builds_statuses)
      stage_success_rates = (
          build_time_stats.GetStageSuccessRates(builds_statuses) if
          self.options.stages else {})

      # Include the number of distinct Chrome uprevs if build_type is set.
      waterfall = build_time_stats.BUILD_TYPE_MAP[self.options.build_type]
      chrome_uprevs = (build_time_stats.GetNumDistinctChromeVersions(
          db, waterfall, start_date, end_date)
                       if self.options.build_type else None)

      if self.options.csv:
        build_time_stats.SuccessReportCsv(
            sys.stdout, build_success_rates, stage_success_rates, chrome_uprevs)
      else:
        build_time_stats.SuccessReport(
            sys.stdout, description, build_success_rates, stage_success_rates,
            chrome_uprevs)
      return 0

    # Compute per-build timing.
    builds_timings = [build_time_stats.GetBuildTimings(status)
                      for status in builds_statuses]

    if not builds_timings:
      logging.critical('No timing results For: %s', description)
      return 1

    # Report results.
    if self.options.report == 'standard':
      build_time_stats.Report(
          sys.stdout,
          description,
          focus_build,
          builds_timings,
          self.options.stages,
          self.options.trending,
          self.options.csv)
    elif self.options.report == 'stability':
      build_time_stats.StabilityReport(
          sys.stdout,
          description,
          builds_timings)
    elif self.options.report == 'stage-stability':
      pbss = build_time_stats.GetPerBuildStageStats(builds_statuses)
      build_time_stats.PerBuildStageStabilityReport(sys.stdout, description,
                                                    pbss)
