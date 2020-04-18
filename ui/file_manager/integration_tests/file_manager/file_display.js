// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Checks if the files initially added by the C++ side are displayed, and
 * that files subsequently added are also displayed.
 *
 * @param {string} path Directory path to be tested.
 */
function fileDisplay(path) {
  var appId;

  var expectedFilesBefore =
      TestEntryInfo.getExpectedRows(path == RootPath.DRIVE ?
          BASIC_DRIVE_ENTRY_SET : BASIC_LOCAL_ENTRY_SET).sort();

  var expectedFilesAfter =
      expectedFilesBefore.concat([ENTRIES.newlyAdded.getExpectedRow()]).sort();

  StepsRunner.run([
    function() {
      setupAndWaitUntilReady(null, path, this.next);
    },
    // Verify the file list.
    function(result) {
      appId = result.windowId;
      var filesBefore = result.fileList;
      chrome.test.assertEq(expectedFilesBefore, filesBefore);
      this.next();
    },
    // Add new file entries.
    function() {
      addEntries(['local', 'drive'], [ENTRIES.newlyAdded], this.next);
    },
    // Wait for the new file entries.
    function(result) {
      if (!result)
        chrome.test.assertTrue(false, 'Failed: adding new files');
      remoteCall.waitForFileListChange(appId, expectedFilesBefore.length).
          then(this.next);
    },
    // Verify the new file list.
    function(filesAfter) {
      chrome.test.assertEq(expectedFilesAfter, filesAfter);
      checkIfNoErrorsOccured(this.next);
    },
  ]);
}

/**
 * Tests files display in Downloads.
 */
testcase.fileDisplayDownloads = function() {
  fileDisplay(RootPath.DOWNLOADS);
};

/**
 * Tests files display in Google Drive.
 */
testcase.fileDisplayDrive = function() {
  fileDisplay(RootPath.DRIVE);
};

/**
 * Tests files display in an MTP volume.
 */
testcase.fileDisplayMtp = function() {
  var appId;
  var MTP_VOLUME_QUERY = '#directory-tree > .tree-item > .tree-row > ' +
    '.item-icon[volume-type-icon="mtp"]';

  StepsRunner.run([
    function() {
      setupAndWaitUntilReady(null, RootPath.DOWNLOADS, this.next);
    },
    // Mount MTP volume in the Downloads window.
    function(results) {
      appId = results.windowId;
      chrome.test.sendMessage(JSON.stringify({name: 'mountFakeMtp'}),
                              this.next);
    },
    // Wait for the MTP mount.
    function(result) {
      remoteCall.waitForElement(appId, MTP_VOLUME_QUERY).then(this.next);
    },
    // Click to open the MTP volume.
    function() {
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, [MTP_VOLUME_QUERY], this.next);
    },
    // Verify the MTP file list.
    function(appIds) {
      remoteCall
          .waitForFiles(
              appId, TestEntryInfo.getExpectedRows(BASIC_FAKE_ENTRY_SET),
              {ignoreLastModifiedTime: true})
          .then(this.next);
    },
    function() {
      checkIfNoErrorsOccured(this.next);
    }
  ]);
};

/**
 * Tests files display in a removable USB volume.
 */
testcase.fileDisplayUsb = function() {
  var appId;
  var USB_VOLUME_QUERY = '#directory-tree > .tree-item > .tree-row > ' +
      '.item-icon[volume-type-icon="removable"]';

  StepsRunner.run([
    function() {
      setupAndWaitUntilReady(null, RootPath.DOWNLOADS, this.next);
    },
    // Mount USB volume in the Downloads window.
    function(results) {
      appId = results.windowId;
      chrome.test.sendMessage(
          JSON.stringify({name: 'mountFakeUsb'}), this.next);
    },
    // Wait for the USB mount.
    function(result) {
      remoteCall.waitForElement(appId, USB_VOLUME_QUERY).then(this.next);
    },
    // Click to open the USB volume.
    function() {
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, [USB_VOLUME_QUERY], this.next);
    },
    // Verify the USB file list.
    function(appIds) {
      remoteCall.waitForFiles(
          appId,
          TestEntryInfo.getExpectedRows(BASIC_FAKE_ENTRY_SET),
          {ignoreLastModifiedTime: true}).then(this.next);
    },
    function() {
      checkIfNoErrorsOccured(this.next);
    }
  ]);
};

/**
 * Searches for a string in Downloads and checks that the correct results
 * are displayed.
 *
 * @param {string} searchTerm The string to search for.
 * @param {Array<Object>} expectedResults The results set.
 *
 */
function searchDownloads(searchTerm, expectedResults) {
  var appId;

  StepsRunner.run([
    function() {
      setupAndWaitUntilReady(null, RootPath.DOWNLOADS, this.next);
    },
    // Focus the search box.
    function(results) {
      appId = results.windowId;
      remoteCall.callRemoteTestUtil('fakeEvent',
                                    appId,
                                    ['#search-box input', 'focus'],
                                    this.next);
    },
    // Input a text.
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.callRemoteTestUtil('inputText',
                                    appId,
                                    ['#search-box input', searchTerm],
                                    this.next);
    },
    // Notify the element of the input.
    function() {
      remoteCall.callRemoteTestUtil('fakeEvent',
                                    appId,
                                    ['#search-box input', 'input'],
                                    this.next);
    },
    function(result) {
      remoteCall.waitForFileListChange(appId, BASIC_LOCAL_ENTRY_SET.length).
      then(this.next);
    },
    function(actualFilesAfter) {
      chrome.test.assertEq(
          TestEntryInfo.getExpectedRows(expectedResults).sort(),
          actualFilesAfter);

      checkIfNoErrorsOccured(this.next);
    }
  ]);
}

/**
 * Tests case-senstive search for an entry in Downloads.
 */
testcase.fileSearch = function() {
  searchDownloads('hello', [ENTRIES.hello]);
};

/**
 * Tests case-insenstive search for an entry in Downloads.
 */
testcase.fileSearchCaseInsensitive = function() {
  searchDownloads('HELLO', [ENTRIES.hello]);
};

/**
 * Tests searching for a string doesn't match anything in Downloads and that
 * there are no displayed items that match the search string.
 */
testcase.fileSearchNotFound = function() {
  var appId;
  var searchTerm = 'blahblah';

  StepsRunner.run([
    function() {
      setupAndWaitUntilReady(null, RootPath.DOWNLOADS, this.next);
    },
    // Focus the search box.
    function(results) {
      appId = results.windowId;
      remoteCall.callRemoteTestUtil('fakeEvent',
                                    appId,
                                    ['#search-box input', 'focus'],
                                    this.next);
    },
    // Input a text.
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.callRemoteTestUtil('inputText',
                                    appId,
                                    ['#search-box input', searchTerm],
                                    this.next);
    },
    // Notify the element of the input.
    function() {
      remoteCall.callRemoteTestUtil('fakeEvent',
                                    appId,
                                    ['#search-box input', 'input'],
                                    this.next);
    },
    function(result) {
      remoteCall.waitForElement(appId, ['#empty-folder-label b']).
          then(this.next);
    },
    function(element) {
      chrome.test.assertEq(element.text, '\"' + searchTerm + '\"');
      checkIfNoErrorsOccured(this.next);
    }
  ]);
};
