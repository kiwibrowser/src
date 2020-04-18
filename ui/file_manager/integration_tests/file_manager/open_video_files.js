// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

(function() {

/**
 * Waits until a window having the given filename appears.
 * @param {string} filename Filename of the requested window.
 * @param {Promise} promise Promise to be fulfilled with a found window's ID.
 */
function waitForPlaying(filename) {
  var caller = getCaller();
  return repeatUntil(function() {
    return videoPlayerApp.callRemoteTestUtil('isPlaying',
                                             null,
                                             [filename]).
        then(function(result) {
          if (result)
            return true;
          return pending(
              caller, 'Window with the prefix %s is not found.', filename);
        });
  });
}

/**
 * Tests if the video player shows up for the selected movie and that it is
 * loaded successfully.
 *
 * @param {string} path Directory path to be tested.
 */
function videoOpen(path) {
  var appId;
  StepsRunner.run([
    function() {
      setupAndWaitUntilReady(null, path, this.next);
    },
    function(results) {
      appId = results.windowId;
      // Select the song.
      remoteCall.callRemoteTestUtil(
          'openFile', appId, ['world.ogv'], this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      // Wait for the video player.
      waitForPlaying('world.ogv').then(this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.callRemoteTestUtil(
          'getErrorCount', null, [], this.next);
    },
    function(errorCount) {
      chrome.test.assertEq(errorCount, 0);
      videoPlayerApp.callRemoteTestUtil(
          'getErrorCount', null, [], this.next);
    },
    function(errorCount) {
      chrome.test.assertEq(errorCount, 0);
      checkIfNoErrorsOccured(this.next);
    }
  ]);
}

// Exports test functions.
testcase.videoOpenDrive = function() {
  videoOpen(RootPath.DRIVE);
};

testcase.videoOpenDownloads = function() {
  videoOpen(RootPath.DOWNLOADS);
};

})();
