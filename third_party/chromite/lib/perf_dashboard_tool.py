# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Developer tool for exploring perf website interaction.

The performance dashboards can be found here (login w/your @google.com):
  https://chromeperf.appspot.com/  (production)
  https://chrome-perf.googleplex.com/  (staging)

By default though, this tool will post data to a local instance running on your
system.  See this page for details on running that:
  https://sites.google.com/a/google.com/chromeperf/

This guide should help familiarize yourself with the perf data format:
  http://dev.chromium.org/developers/testing/sending-data-to-the-performance-dashboard
This tool currently uses the version 0 data format.

Some notes:
 - --revision or --cros-version/--chrome-version should be independent
 - do not mix rev/cros-ver/chrome-ver in the same data series/graph

Examples:
  # Create a data point at (20110701024650,361077106).  The test name is
  # "sdk.size" and is in the "base" series in the "combined" graph.
  $ ./perf_dashboard_tool -u bytes -t sdk.size -g combined -d base \\
      --revision 20110701024650 361077106

  # Create a data point at (6689.0.0,2000400100).  The test name is
  # "disk.size" and is in the "data" series.
  $ ./perf_dashboard_tool -u bytes -t disk.size --cros-version 6689.0.0 \\
      --chrome-version 41.6689.0.0 2000400100
"""

from __future__ import print_function

import getpass
import os
import tempfile
import urllib

from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import perf_uploader


MASTER_NAME = 'ChromeOSPerfTest'
TEST_NAME = 'perf_uploader_tool.%s' % getpass.getuser()


def GetParser():
  """Return a command line parser"""
  parser = commandline.ArgumentParser(description=__doc__)
  parser.add_argument('-n', '--dry-run', default=False, action='store_true',
                      help='Show what would be uploaded')
  parser.add_argument('--url', default=perf_uploader.LOCAL_DASHBOARD_URL,
                      help='Dashboard to send results to')

  group = parser.add_argument_group('Bot Details')
  group.add_argument('-m', '--master', default=MASTER_NAME,
                     help='The buildbot master field')
  group.add_argument('-b', '--bot',
                     default=cros_build_lib.GetHostName(fully_qualified=True),
                     help='The bot name (e.g. buildbot config)')

  group = parser.add_argument_group('Version Options (X-axis)')
  group.add_argument('--revision', default=None,
                     help='Revision number')
  group.add_argument('--cros-version', default=None,
                     help='Chrome OS version (X.Y.Z)')
  group.add_argument('--chrome-version', default=None,
                     help='Chrome version (M.X.Y.Z)')

  group = parser.add_argument_group('Data Options')
  group.add_argument('-t', '--test', default=TEST_NAME,
                     help='The test name field')
  group.add_argument('--higher-is-better', default=False, action='store_true',
                     help='Whether higher values are better than lower')
  group.add_argument('-u', '--units', default='',
                     help='Units for the perf data (e.g. percent, bytes)')
  group.add_argument('-g', '--graph',
                     help='Graph name (to group multiple tests)')
  group.add_argument('-d', '--description', default='data',
                     help='Name for this data series')
  group.add_argument('--stdio-uri',
                     help='Custom log page to link data point to')
  group.add_argument('data',
                     help='Data point (int or float)')

  return parser


def main(argv):
  parser = GetParser()
  opts = parser.parse_args(argv)
  opts.Freeze()

  logging.info('Uploading results to %s', opts.url)
  logging.info('Master name: %s', opts.master)
  logging.info('Test name: %s', opts.test)

  with tempfile.NamedTemporaryFile() as output:
    perf_uploader.OutputPerfValue(
        output.name,
        opts.description,
        float(opts.data),
        opts.units,
        graph=opts.graph,
        stdio_uri=opts.stdio_uri)
    perf_values = perf_uploader.LoadPerfValues(output.name)

  logging.debug('Uploading:')
  for value in perf_values:
    logging.debug('  %s', value)

  perf_uploader.UploadPerfValues(
      perf_values,
      opts.bot,
      opts.test,
      revision=opts.revision,
      cros_version=opts.cros_version,
      chrome_version=opts.chrome_version,
      dashboard=opts.url,
      master_name=opts.master,
      test_prefix='',
      platform_prefix='',
      dry_run=opts.dry_run)

  data_name = opts.graph if opts.graph else opts.description
  args = {
      'masters': opts.master,
      'tests': '%s/%s' % (opts.test, data_name),
      'bots': opts.bot,
  }
  view_url = os.path.join(opts.url, 'report?%s' % urllib.urlencode(args))
  logging.info('View results at %s', view_url)
  logging.info('Note: To make tests public, visit %s',
               os.path.join(opts.url, 'change_internal_only'))
  logging.info('Note: To update the test list, visit %s',
               os.path.join(opts.url, 'update_test_suites'))
