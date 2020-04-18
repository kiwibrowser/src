// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Test suite for the Media Engagement WebUI.
 */
var ROOT_PATH = '../../../../../';
var EXAMPLE_URL_1 = 'http://example.com/';
var EXAMPLE_URL_2 = 'http://shmlexample.com/';

GEN('#include "chrome/browser/media/media_engagement_service.h"');
GEN('#include "chrome/browser/media/media_engagement_service_factory.h"');
GEN('#include "chrome/browser/ui/browser.h"');
GEN('#include "media/base/media_switches.h"');

function MediaEngagementWebUIBrowserTest() {}

MediaEngagementWebUIBrowserTest.prototype = {
  __proto__: testing.Test.prototype,

  browsePreload: 'chrome://media-engagement',

  featureList: ['media::kRecordMediaEngagementScores', ''],

  runAccessibilityChecks: false,

  isAsync: true,

  testGenPreamble: function() {
    GEN('MediaEngagementService* service =');
    GEN('  MediaEngagementServiceFactory::GetForProfile(');
    GEN('    browser()->profile());');
    GEN('service->RecordVisit(GURL("' + EXAMPLE_URL_1 + '"));');
    GEN('service->RecordVisit(GURL("' + EXAMPLE_URL_2 + '"));');
    GEN('service->RecordPlayback(GURL("' + EXAMPLE_URL_1 + '"));');
    GEN('service->RecordPlayback(GURL("' + EXAMPLE_URL_2 + '"));');
  },

  extraLibraries: [
    ROOT_PATH + 'third_party/mocha/mocha.js',
    ROOT_PATH + 'chrome/test/data/webui/mocha_adapter.js',
  ],
};

TEST_F('MediaEngagementWebUIBrowserTest', 'All', function() {
  suiteSetup(function() {
    return whenPageIsPopulatedForTest();
  });

  test('check engagement values are loaded', function() {
    var originCells =
        Array.from(document.getElementsByClassName('origin-cell'));
    assertDeepEquals(
        [EXAMPLE_URL_1, EXAMPLE_URL_2], originCells.map(x => x.textContent));
  });

  mocha.run();
});
