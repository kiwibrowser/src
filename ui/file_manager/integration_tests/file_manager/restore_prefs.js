// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Tests restoring the sorting order.
 */
testcase.restoreSortColumn = function() {
  var appId;
  var EXPECTED_FILES = TestEntryInfo.getExpectedRows([
    ENTRIES.photos,     // 'photos' (directory)
    ENTRIES.world,      // 'world.ogv', 59943 bytes
    ENTRIES.beautiful,  // 'Beautiful Song.ogg', 13410 bytes
    ENTRIES.desktop,    // 'My Desktop Background.png', 272 bytes
    ENTRIES.hello,      // 'hello.txt', 51 bytes
  ]);
  StepsRunner.run([
    // Set up File Manager.
    function() {
      setupAndWaitUntilReady(null, RootPath.DOWNLOADS, this.next);
    },
    // Sort by name.
    function(results) {
      appId = results.windowId;
      remoteCall.callRemoteTestUtil('fakeMouseClick',
                                    appId,
                                    ['.table-header-cell:nth-of-type(1)'],
                                    this.next);
    },
    // Check the sorted style of the header.
    function() {
      remoteCall.waitForElement(appId, '.table-header-sort-image-asc').
          then(this.next);
    },
    // Sort by size (in descending order).
    function() {
      remoteCall.callRemoteTestUtil('fakeMouseClick',
                                    appId,
                                    ['.table-header-cell:nth-of-type(2)'],
                                    this.next);
    },
    // Check the sorted style of the header.
    function() {
      remoteCall.waitForElement(appId, '.table-header-sort-image-desc').
          then(this.next);
    },
    // Check the sorted files.
    function() {
      remoteCall.waitForFiles(appId, EXPECTED_FILES, {orderCheck: true}).
          then(this.next);
    },
    // Open another window, where the sorted column should be restored.
    function() {
      setupAndWaitUntilReady(null, RootPath.DOWNLOADS, this.next);
    },
    // Check the sorted style of the header.
    function(results) {
      appId = results.windowId;
      remoteCall.waitForElement(appId, '.table-header-sort-image-desc').
          then(this.next);
    },
    // Check the sorted files.
    function() {
      remoteCall.waitForFiles(appId, EXPECTED_FILES, {orderCheck: true}).
          then(this.next);
    },
    // Check the error.
    function() {
      checkIfNoErrorsOccured(this.next);
    }
  ]);
};

/**
 * Tests restoring the current view (the file list or the thumbnail grid).
 */
testcase.restoreCurrentView = function() {
  var appId;
  StepsRunner.run([
    // Set up File Manager.
    function() {
      setupAndWaitUntilReady(null, RootPath.DOWNLOADS, this.next);
    },
    // Check the initial view.
    function(results) {
      appId = results.windowId;
      remoteCall.waitForElement(appId, '.thumbnail-grid[hidden]').
          then(this.next);
    },
    // Change the current view.
    function() {
      remoteCall.callRemoteTestUtil('fakeMouseClick',
                                    appId,
                                    ['#view-button'],
                                    this.next);
    },
    // Check the new current view.
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.waitForElement(appId, '.detail-table[hidden]').
          then(this.next);
    },
    // Open another window, where the current view is restored.
    function() {
      openNewWindow(null, RootPath.DOWNLOADS, this.next);
    },
    // Check the current view.
    function(inAppId) {
      appId = inAppId;
      remoteCall.waitForElement(appId, '.detail-table[hidden]').then(this.next);
    },
    // Check the error.
    function() {
      checkIfNoErrorsOccured(this.next);
    }
  ]);
};
