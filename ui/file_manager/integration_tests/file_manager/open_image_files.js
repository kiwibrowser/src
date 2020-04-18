// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

(function() {

/**
 * Tests if the gallery shows up for the selected image and is loaded
 * successfully.
 *
 * @param {string} path Directory path to be tested.
 */
function imageOpen(path) {
  var appId;
  var galleryAppId;

  var expectedFilesBefore =
      TestEntryInfo.getExpectedRows(path == RootPath.DRIVE ?
          BASIC_DRIVE_ENTRY_SET : BASIC_LOCAL_ENTRY_SET).sort();
  var expectedFilesAfter =
      expectedFilesBefore.concat([ENTRIES.image3.getExpectedRow()]).sort();

  StepsRunner.run([
    function() {
      setupAndWaitUntilReady(null, path, this.next);
    },
    // Select the song.
    function(results) {
      appId = results.windowId;

      // Add an additional image file.
      addEntries(['local', 'drive'], [ENTRIES.image3], this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.waitForFileListChange(appId, expectedFilesBefore.length).
          then(this.next);
    },
    function(actualFilesAfter) {
      chrome.test.assertEq(expectedFilesAfter, actualFilesAfter);
      // Open a file in the Files app.
      remoteCall.callRemoteTestUtil(
          'openFile', appId, ['My Desktop Background.png'], this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      // Wait for the window.
      galleryApp.waitForWindow('gallery.html').then(this.next);
    },
    function(inAppId) {
      galleryAppId = inAppId;
      // Wait for the file opened.
      galleryApp.waitForSlideImage(
          galleryAppId, 800, 600, 'My Desktop Background').then(this.next);
    },
    function() {
      // Open another file in the Files app.
      remoteCall.callRemoteTestUtil(
          'openFile', appId, ['image3.jpg'], this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      // Wait for the file opened.
      galleryApp.waitForSlideImage(
          galleryAppId, 640, 480, 'image3').then(this.next);
    },
    function() {
      // Close window
      galleryApp.closeWindowAndWait(galleryAppId).then(this.next);
    },
    // Wait for closing.
    function(result) {
      chrome.test.assertTrue(result, 'Fail to close the window');
      checkIfNoErrorsOccured(this.next);
    }
  ]);
}

testcase.imageOpenDownloads = function() {
  imageOpen(RootPath.DOWNLOADS);
};

testcase.imageOpenDrive = function() {
  imageOpen(RootPath.DRIVE);
};

})();
