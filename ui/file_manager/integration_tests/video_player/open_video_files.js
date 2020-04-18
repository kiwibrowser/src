// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * The openSingleImage test for Downloads.
 * @return {Promise} Promise to be fulfilled with on success.
 */
testcase.openSingleVideoOnDownloads = function() {
    var test = openSingleVideo('local', 'downloads', ENTRIES.world);
    return test.then(function(args) {
      var videoPlayer = args[1];
      chrome.test.assertFalse('cast-available' in videoPlayer.attributes);
    });
};

/**
 * The openSingleImage test for Drive.
 * @return {Promise} Promise to be fulfilled with on success.
 */
testcase.openSingleVideoOnDrive = function() {
    var test = openSingleVideo('drive', 'drive', ENTRIES.world);
    return test.then(function(args) {
      var appWindow = args[0];
      var videoPlayer = args[1];
      chrome.test.assertFalse('cast-available' in videoPlayer.attributes);

      return remoteCallVideoPlayer.callRemoteTestUtil(
          'loadMockCastExtension', appWindow, []).then(function() {
        // Loads cast extension and wait for available cast.
        return remoteCallVideoPlayer.waitForElement(
            appWindow, '#video-player[cast-available]');
      });
    });
};
