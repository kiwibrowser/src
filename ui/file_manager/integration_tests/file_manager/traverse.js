// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Test utility for traverse tests.
 * @param {string} path Root path to be traversed.
 */
function traverseDirectories(path) {
  var appId;
  StepsRunner.run([
    // Set up File Manager. Do not add initial files.
    function() {
      openNewWindow(null, path, this.next);
    },
    // Check the initial view.
    function(inAppId) {
      appId = inAppId;
      remoteCall.waitForElement(appId, '#detail-table').then(this.next);
    },
    function() {
      addEntries(['local', 'drive'], NESTED_ENTRY_SET, this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.waitForFiles(appId, [ENTRIES.directoryA.getExpectedRow()]).
          then(this.next);
    },
    // Open the directory
    function() {
      remoteCall.callRemoteTestUtil('openFile', appId, ['A'], this.next);
    },
    // Check the contents of current directory.
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.waitForFiles(appId, [ENTRIES.directoryB.getExpectedRow()]).
          then(this.next);
    },
    // Open the directory
    function() {
      remoteCall.callRemoteTestUtil('openFile', appId, ['B'], this.next);
    },
    // Check the contents of current directory.
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.waitForFiles(appId, [ENTRIES.directoryC.getExpectedRow()]).
          then(this.next);
    },
    // Check the error.
    function() {
      checkIfNoErrorsOccured(this.next);
    }
  ]);
}

/**
 * Tests to traverse local directories.
 */
testcase.traverseDownloads = function() {
  traverseDirectories(RootPath.DOWNLOADS);
};

/**
 * Tests to traverse drive directories.
 */
testcase.traverseDrive = function() {
  traverseDirectories(RootPath.DRIVE);
};
