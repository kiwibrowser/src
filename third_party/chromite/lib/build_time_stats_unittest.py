# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for boolparse_lib methods."""

from __future__ import print_function

import datetime
import StringIO

import build_time_stats
import cros_test_lib


class BuildTimeStatsTest(cros_test_lib.TestCase):
  """Unittest build_time_stats code that doesn't use the database."""

  def testFilterBuildStatuses(self):
    test_data = [
        {'status': 'pass', 'waterfall': 'chromeos'},
        {'status': 'fail', 'waterfall': 'chromeos'},
        {'status': 'pass', 'waterfall': 'chromiumos'},
        {'status': 'fail', 'waterfall': 'chromiumos'},
        {'status': 'pass', 'waterfall': 'chromiumos.release'},
        {'status': 'fail', 'waterfall': 'chromiumos.release'},
        {'status': 'pass', 'waterfall': 'bogus'},
        {'status': 'fail', 'waterfall': 'bogus'},
    ]

    expected = [
        {'status': 'pass', 'waterfall': 'chromeos'},
        {'status': 'fail', 'waterfall': 'chromeos'},
        {'status': 'pass', 'waterfall': 'chromiumos'},
        {'status': 'fail', 'waterfall': 'chromiumos'},
    ]

    result = build_time_stats.FilterBuildStatuses(test_data)
    self.assertEqual(result, expected)

  def testGetBuildTimings(self):

    startTime = datetime.datetime(2014, 11, 3, 16, 22)

    good_data = {
        'id': 1,
        'build_config': 'test_config',
        'status': 'pass',
        'start_time': startTime,
        'finish_time': startTime + datetime.timedelta(hours=6),
        'stages': [
            {
                'name': 'start',
                'start_time': startTime,
                'finish_time': startTime + datetime.timedelta(hours=2),
            },
            {
                'name': 'build',
                'start_time': startTime + datetime.timedelta(hours=2),
                'finish_time': startTime + datetime.timedelta(hours=4),
            },
        ],
    }

    expected_good = build_time_stats.BuildTiming(
        id=1,
        build_config='test_config',
        success=True,
        start=startTime,
        finish=startTime + datetime.timedelta(hours=6),
        duration=datetime.timedelta(hours=6),
        stages=[
            build_time_stats.StageTiming(
                name='start',
                start=datetime.timedelta(0),
                finish=datetime.timedelta(hours=2),
                duration=datetime.timedelta(hours=2)),
            build_time_stats.StageTiming(
                name='build',
                start=datetime.timedelta(hours=2),
                finish=datetime.timedelta(hours=4),
                duration=datetime.timedelta(hours=2))
        ]
    )

    bad_data = {
        'id': 2,
        'build_config': 'test_config',
        'status': 'bogus',
        'start_time': startTime,
        'finish_time': None,
        'stages': [
            {
                'name': 'start',
                'start_time': startTime,
                'finish_time': None,
            },
            {
                'name': 'build',
                'start_time': None,
                'finish_time': startTime + datetime.timedelta(hours=4),
            },
        ],
    }

    expected_bad = build_time_stats.BuildTiming(
        id=2,
        build_config='test_config',
        success=False,
        start=startTime,
        finish=None,
        duration=None,
        stages=[
            build_time_stats.StageTiming(
                name='start',
                start=datetime.timedelta(0),
                finish=None,
                duration=None),
            build_time_stats.StageTiming(
                name='build',
                start=None,
                finish=datetime.timedelta(hours=4),
                duration=None),
        ]
    )

    result = build_time_stats.GetBuildTimings(good_data)
    self.assertEqual(result, expected_good)

    result = build_time_stats.GetBuildTimings(bad_data)
    self.assertEqual(result, expected_bad)

  def testCalculateTimeStats(self):
    test_data = [
        None,
        datetime.timedelta(hours=0),
        None,
        datetime.timedelta(hours=2),
        datetime.timedelta(hours=7),
        None,
    ]

    expected = build_time_stats.TimeDeltaStats(
        median=datetime.timedelta(hours=2),
        mean=datetime.timedelta(hours=3),
        min=datetime.timedelta(0),
        max=datetime.timedelta(hours=7))

    result = build_time_stats.CalculateTimeStats(test_data)
    self.assertEqual(result, expected)

  def testCalculateTimeStatsNoInput(self):
    self.assertIsNone(build_time_stats.CalculateTimeStats([]))
    self.assertIsNone(build_time_stats.CalculateTimeStats([None, None]))


class BuildTimeStatsReportTest(cros_test_lib.TestCase):
  """Test the build_time_stats.Report method."""

  def setUp(self):
    # We compare a lot of large strings.
    self.maxDiff = None

    start0 = datetime.datetime(2014, 10, 3, 16, 22)
    start1 = datetime.datetime(2014, 11, 3, 16, 22)
    start2 = datetime.datetime(2014, 11, 4, 16, 22)
    start3 = datetime.datetime(2014, 12, 1, 16, 22)

    self.focus_build = build_time_stats.BuildTiming(
        id=0,
        build_config='test_config',
        success=True,
        start=start0,
        finish=start0 + datetime.timedelta(hours=12),
        duration=datetime.timedelta(hours=12),
        stages=[
            build_time_stats.StageTiming(
                name='start',
                start=datetime.timedelta(0),
                finish=datetime.timedelta(hours=8),
                duration=datetime.timedelta(hours=8)),
            build_time_stats.StageTiming(
                name='build',
                start=datetime.timedelta(hours=6),
                finish=datetime.timedelta(hours=12),
                duration=datetime.timedelta(hours=6))
        ]
    )

    self.builds_timings = [
        build_time_stats.BuildTiming(
            id=1,
            build_config='test_config',
            success=True,
            start=start1,
            finish=start1 + datetime.timedelta(hours=6),
            duration=datetime.timedelta(hours=6),
            stages=[
                build_time_stats.StageTiming(
                    name='start',
                    start=datetime.timedelta(0),
                    finish=datetime.timedelta(hours=2),
                    duration=datetime.timedelta(hours=2)),
                build_time_stats.StageTiming(
                    name='build',
                    start=datetime.timedelta(hours=2),
                    finish=datetime.timedelta(hours=4),
                    duration=datetime.timedelta(hours=2))
            ]
        ),
        build_time_stats.BuildTiming(
            id=2,
            build_config='test_config',
            success=True,
            start=start2,
            finish=start2 + datetime.timedelta(hours=8),
            duration=datetime.timedelta(hours=8),
            stages=[
                build_time_stats.StageTiming(
                    name='start',
                    start=datetime.timedelta(0),
                    finish=datetime.timedelta(hours=4),
                    duration=datetime.timedelta(hours=4)),
                build_time_stats.StageTiming(
                    name='build',
                    start=datetime.timedelta(hours=4),
                    finish=datetime.timedelta(hours=8),
                    duration=datetime.timedelta(hours=4))
            ]
        ),
        build_time_stats.BuildTiming(
            id=3,
            build_config='test_config',
            success=True,
            start=start3,
            finish=start2 + datetime.timedelta(hours=6),
            duration=datetime.timedelta(hours=6),
            stages=[
                build_time_stats.StageTiming(
                    name='start',
                    start=datetime.timedelta(0),
                    finish=datetime.timedelta(hours=2),
                    duration=datetime.timedelta(hours=2)),
                build_time_stats.StageTiming(
                    name='build',
                    start=datetime.timedelta(hours=2),
                    finish=datetime.timedelta(hours=4),
                    duration=datetime.timedelta(hours=2))
            ]
        ),
        build_time_stats.BuildTiming(
            id=4,
            build_config='test_config',
            success=False,
            start=start3,
            finish=start2 + datetime.timedelta(hours=6),
            duration=datetime.timedelta(hours=6),
            stages=[
                build_time_stats.StageTiming(
                    name='start',
                    start=datetime.timedelta(0),
                    finish=datetime.timedelta(hours=2),
                    duration=datetime.timedelta(hours=2)),
                build_time_stats.StageTiming(
                    name='build',
                    start=datetime.timedelta(hours=2),
                    finish=datetime.timedelta(hours=4),
                    duration=datetime.timedelta(hours=2))
            ]
        ),
        build_time_stats.BuildTiming(
            id=5,
            build_config='timeout_config',
            success=False,
            start=start3,
            finish=None,
            duration=datetime.timedelta(hours=6),
            stages=[
                build_time_stats.StageTiming(
                    name='start',
                    start=datetime.timedelta(0),
                    finish=datetime.timedelta(hours=2),
                    duration=datetime.timedelta(hours=2)),
                build_time_stats.StageTiming(
                    name='build',
                    start=datetime.timedelta(hours=2),
                    finish=datetime.timedelta(hours=4),
                    duration=datetime.timedelta(hours=2))
            ]
        ),
    ]

  def testReportNoFocusNoStagesNoTrend(self):
    result = StringIO.StringIO()
    build_time_stats.Report(
        result,
        'description',
        None,
        self.builds_timings,
        stages=False,
        trending=False)

    expected = '''description
Averages for 5 Builds: 1 - 5
  success  60% timeouts  20% median 6:00:00 mean 6:40:00 min 6:00:00 max 8:00:00
'''
    self.assertMultiLineEqual(expected, result.getvalue())

  def testReportFocusNoStagesNoTrend(self):
    result = StringIO.StringIO()
    build_time_stats.Report(
        result,
        'description',
        self.focus_build,
        self.builds_timings,
        stages=False,
        trending=False)

    expected = '''description
Averages for 5 Builds: 1 - 5
 12:00:00 success  60% timeouts  20% median 6:00:00 mean 6:40:00 min 6:00:00 max 8:00:00
'''
    self.assertMultiLineEqual(expected, result.getvalue())

  def testReportNoFocusStagesNoTrend(self):
    result = StringIO.StringIO()
    build_time_stats.Report(
        result,
        'description',
        None,
        self.builds_timings,
        stages=True,
        trending=False)

    expected = '''description
Averages for 5 Builds: 1 - 5
  success  60% timeouts  20% median 6:00:00 mean 6:40:00 min 6:00:00 max 8:00:00

start:
  start:     median 0:00:00 mean 0:00:00 min 0:00:00 max 0:00:00
  duration:  median 2:00:00 mean 2:40:00 min 2:00:00 max 4:00:00
  finish:    median 2:00:00 mean 2:40:00 min 2:00:00 max 4:00:00
build:
  start:     median 2:00:00 mean 2:40:00 min 2:00:00 max 4:00:00
  duration:  median 2:00:00 mean 2:40:00 min 2:00:00 max 4:00:00
  finish:    median 4:00:00 mean 5:20:00 min 4:00:00 max 8:00:00
'''
    self.assertMultiLineEqual(expected, result.getvalue())

  def testReportFocusStagesNoTrend(self):
    result = StringIO.StringIO()
    build_time_stats.Report(
        result,
        'description',
        self.focus_build,
        self.builds_timings,
        stages=True,
        trending=False)

    expected = '''description
Averages for 5 Builds: 1 - 5
 12:00:00 success  60% timeouts  20% median 6:00:00 mean 6:40:00 min 6:00:00 max 8:00:00

start:
  start:    0:00:00 median 0:00:00 mean 0:00:00 min 0:00:00 max 0:00:00
  duration: 8:00:00 median 2:00:00 mean 2:40:00 min 2:00:00 max 4:00:00
  finish:   8:00:00 median 2:00:00 mean 2:40:00 min 2:00:00 max 4:00:00
build:
  start:    6:00:00 median 2:00:00 mean 2:40:00 min 2:00:00 max 4:00:00
  duration: 6:00:00 median 2:00:00 mean 2:40:00 min 2:00:00 max 4:00:00
  finish:   12:00:00 median 4:00:00 mean 5:20:00 min 4:00:00 max 8:00:00
'''
    self.assertMultiLineEqual(expected, result.getvalue())

  def testReportNoFocusNoStagesTrend(self):
    result = StringIO.StringIO()
    build_time_stats.Report(
        result,
        'description',
        None,
        self.builds_timings,
        stages=False,
        trending=True,)

    expected = '''description
Averages for 5 Builds: 1 - 5
  success  60% timeouts  20% median 6:00:00 mean 6:40:00 min 6:00:00 max 8:00:00

2014-11:  success 100% timeouts   0% median 8:00:00 mean 7:00:00 min 6:00:00 max 8:00:00
2014-12:  success  33% timeouts  33% median 6:00:00 mean 6:00:00 min 6:00:00 max 6:00:00
'''
    self.assertMultiLineEqual(expected, result.getvalue())

  def testReportFocusStagesTrend(self):
    result = StringIO.StringIO()
    build_time_stats.Report(
        result,
        'description',
        self.focus_build,
        self.builds_timings,
        stages=True,
        trending=True,)

    expected = '''description
Averages for 5 Builds: 1 - 5
 12:00:00 success  60% timeouts  20% median 6:00:00 mean 6:40:00 min 6:00:00 max 8:00:00

start:
  start:    0:00:00 median 0:00:00 mean 0:00:00 min 0:00:00 max 0:00:00
  duration: 8:00:00 median 2:00:00 mean 2:40:00 min 2:00:00 max 4:00:00
  finish:   8:00:00 median 2:00:00 mean 2:40:00 min 2:00:00 max 4:00:00
build:
  start:    6:00:00 median 2:00:00 mean 2:40:00 min 2:00:00 max 4:00:00
  duration: 6:00:00 median 2:00:00 mean 2:40:00 min 2:00:00 max 4:00:00
  finish:   12:00:00 median 4:00:00 mean 5:20:00 min 4:00:00 max 8:00:00

2014-11:  success 100% timeouts   0% median 8:00:00 mean 7:00:00 min 6:00:00 max 8:00:00
  start:
    start:    median 0:00:00 mean 0:00:00 min 0:00:00 max 0:00:00
    duration: median 4:00:00 mean 3:00:00 min 2:00:00 max 4:00:00
    finish:   median 4:00:00 mean 3:00:00 min 2:00:00 max 4:00:00
  build:
    start:    median 4:00:00 mean 3:00:00 min 2:00:00 max 4:00:00
    duration: median 4:00:00 mean 3:00:00 min 2:00:00 max 4:00:00
    finish:   median 8:00:00 mean 6:00:00 min 4:00:00 max 8:00:00
2014-12:  success  33% timeouts  33% median 6:00:00 mean 6:00:00 min 6:00:00 max 6:00:00
  start:
    start:    median 0:00:00 mean 0:00:00 min 0:00:00 max 0:00:00
    duration: median 2:00:00 mean 2:00:00 min 2:00:00 max 2:00:00
    finish:   median 2:00:00 mean 2:00:00 min 2:00:00 max 2:00:00
  build:
    start:    median 2:00:00 mean 2:00:00 min 2:00:00 max 2:00:00
    duration: median 2:00:00 mean 2:00:00 min 2:00:00 max 2:00:00
    finish:   median 4:00:00 mean 4:00:00 min 4:00:00 max 4:00:00
'''
    self.assertMultiLineEqual(expected, result.getvalue())

  def testReportNoFocusNoStagesCsv(self):
    result = StringIO.StringIO()
    build_time_stats.Report(
        result,
        'description',
        None,
        self.builds_timings,
        stages=False,
        trending=True,
        csv=True)

    expected = '''"description", "Averages for 5 Builds: 1 - 5"
"", "Build", "", "", "", ""
"", "success", "median", "mean", "min", "max"
"ALL", " 60%", "6:00:00", "6:40:00", "6:00:00", "8:00:00"
"2014-11", "100%", "8:00:00", "7:00:00", "6:00:00", "8:00:00"
"2014-12", " 33%", "6:00:00", "6:00:00", "6:00:00", "6:00:00"
'''
    self.assertMultiLineEqual(expected, result.getvalue())

  def testReportNoFocusStagesCsv(self):
    result = StringIO.StringIO()
    build_time_stats.Report(
        result,
        'description',
        None,
        self.builds_timings,
        stages=True,
        trending=True,
        csv=True)

    # pylint: disable=line-too-long
    expected = '''"description", "Averages for 5 Builds: 1 - 5"
"", "Build", "", "", "", "", "start", "", "", "", "", "build", "", "", "", ""
"", "success", "median", "mean", "min", "max", "success", "median", "mean", "min", "max", "success", "median", "mean", "min", "max"
"ALL", " 60%", "6:00:00", "6:40:00", "6:00:00", "8:00:00", "", "2:00:00", "2:40:00", "2:00:00", "4:00:00", "", "2:00:00", "2:40:00", "2:00:00", "4:00:00"
"2014-11", "100%", "8:00:00", "7:00:00", "6:00:00", "8:00:00", "", "4:00:00", "3:00:00", "2:00:00", "4:00:00", "", "4:00:00", "3:00:00", "2:00:00", "4:00:00"
"2014-12", " 33%", "6:00:00", "6:00:00", "6:00:00", "6:00:00", "", "2:00:00", "2:00:00", "2:00:00", "2:00:00", "", "2:00:00", "2:00:00", "2:00:00", "2:00:00"
'''
    self.assertMultiLineEqual(expected, result.getvalue())

  def testReportFocusNoStagesCsv(self):
    result = StringIO.StringIO()
    build_time_stats.Report(
        result,
        'description',
        self.focus_build,
        self.builds_timings,
        stages=False,
        trending=True,
        csv=True)

    expected = '''"description", "Averages for 5 Builds: 1 - 5"
"", "Build", "", "", "", ""
"", "success", "median", "mean", "min", "max"
"Focus", "True", "12:00:00", "", "", ""
"ALL", " 60%", "6:00:00", "6:40:00", "6:00:00", "8:00:00"
"2014-11", "100%", "8:00:00", "7:00:00", "6:00:00", "8:00:00"
"2014-12", " 33%", "6:00:00", "6:00:00", "6:00:00", "6:00:00"
'''
    self.assertMultiLineEqual(expected, result.getvalue())

  def testReportFocusStagesCsv(self):
    result = StringIO.StringIO()

    build_time_stats.Report(
        result,
        'description',
        self.focus_build,
        self.builds_timings,
        stages=True,
        trending=True,
        csv=True)

    # pylint: disable=line-too-long
    expected = '''"description", "Averages for 5 Builds: 1 - 5"
"", "Build", "", "", "", "", "start", "", "", "", "", "build", "", "", "", ""
"", "success", "median", "mean", "min", "max", "success", "median", "mean", "min", "max", "success", "median", "mean", "min", "max"
"Focus", "True", "12:00:00", "", "", "", "", "8:00:00", "", "", "", "", "6:00:00", "", "", ""
"ALL", " 60%", "6:00:00", "6:40:00", "6:00:00", "8:00:00", "", "2:00:00", "2:40:00", "2:00:00", "4:00:00", "", "2:00:00", "2:40:00", "2:00:00", "4:00:00"
"2014-11", "100%", "8:00:00", "7:00:00", "6:00:00", "8:00:00", "", "4:00:00", "3:00:00", "2:00:00", "4:00:00", "", "4:00:00", "3:00:00", "2:00:00", "4:00:00"
"2014-12", " 33%", "6:00:00", "6:00:00", "6:00:00", "6:00:00", "", "2:00:00", "2:00:00", "2:00:00", "2:00:00", "", "2:00:00", "2:00:00", "2:00:00", "2:00:00"
'''
    self.assertMultiLineEqual(expected, result.getvalue())

  def testStabilityReport(self):
    result = StringIO.StringIO()
    build_time_stats.StabilityReport(
        result,
        'description',
        self.builds_timings)

    expected = '''description
timeout_config:   0% successes 100% timeouts 1 builds.
test_config:     75% successes   0% timeouts 4 builds.
'''
    self.assertMultiLineEqual(expected, result.getvalue())
