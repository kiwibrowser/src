# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest
import sys

from core import system_health_csv_generator
from core import path_util
sys.path.insert(1, path_util.GetTelemetryDir())  # To resolve telemetry imports


class GenerateSystemHealthCSVTest(unittest.TestCase):
  def testPopulateExpectations(self):
    expected_result = {
        'browse:media:tumblr': 'Mac 10.11',
        'browse:news:cnn': 'Mac Platforms',
        'browse:news:hackernews': 'Win Platforms, Mac Platforms',
        'browse:search:google': 'Win Platforms',
        'browse:tools:earth': 'All Platforms',
        'browse:tools:maps': 'All Platforms',
        'play:media:google_play_music': 'All Platforms',
        'play:media:pandora': 'All Platforms',
        'play:media:soundcloud': 'Win Platforms'}
    all_expects = [{
        'browse:news:cnn': [(
            ['Mac Platforms'], 'crbug.com/728576')],
        'browse:tools:earth': [(
            ['All Platforms'], 'crbug.com/760966')],
        'browse:news:hackernews': [(
            ['Win Platforms', 'Mac Platforms'],
            'crbug.com/712694')],
        'play:media:soundcloud': [(
            ['Win Platforms'], 'crbug.com/649392')],
        'play:media:google_play_music': [(
            ['All Platforms'], 'crbug.com/649392')],
        'browse:tools:maps': [(
            ['All Platforms'], 'crbug.com/712694')],
        'browse:search:google': [(
            ['Win Platforms'], 'win:crbug.com/673775, mac:crbug.com/756027')],
        'browse:media:tumblr': [(
            ['Mac 10.11'], 'crbug.com/760966')],
        'play:media:pandora': [(
            ['All Platforms'], 'crbug.com/649392')],
    }]
    self.assertEquals(
        expected_result,
        system_health_csv_generator.PopulateExpectations(all_expects))
