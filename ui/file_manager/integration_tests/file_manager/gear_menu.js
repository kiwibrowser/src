// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Expected files shown in Downloads with hidden enabled
 *
 * @type {!Array<!TestEntryInfo>}
 * @const
 */
var BASIC_LOCAL_ENTRY_SET_WITH_HIDDEN = [
  ENTRIES.hello,
  ENTRIES.world,
  ENTRIES.desktop,
  ENTRIES.beautiful,
  ENTRIES.photos,
  ENTRIES.hiddenFile
];

/**
 * Expected files shown in Drive with hidden enabled
 *
 * @type {!Array<!TestEntryInfo>}
 * @const
 */
var BASIC_DRIVE_ENTRY_SET_WITH_HIDDEN = [
  ENTRIES.hello,
  ENTRIES.world,
  ENTRIES.desktop,
  ENTRIES.beautiful,
  ENTRIES.photos,
  ENTRIES.unsupported,
  ENTRIES.testDocument,
  ENTRIES.testSharedDocument,
  ENTRIES.hiddenFile
];

/**
 * Expected files shown in Drive with Google Docs disabled
 *
 * @type {!Array<!TestEntryInfo>}
 * @const
 */
var BASIC_DRIVE_ENTRY_SET_WITHOUT_GDOCS = [
  ENTRIES.hello,
  ENTRIES.world,
  ENTRIES.desktop,
  ENTRIES.beautiful,
  ENTRIES.photos,
  ENTRIES.unsupported,
];

/**
 * Gets the common steps to toggle hidden files in the Files app
 * @param {!Array<!TestEntryInfo>} basicSet Files expected before showing hidden
 * @param {!Array<!TestEntryInfo>} hiddenEntrySet Files expected after showing
 * hidden
 * @return {!Array} The test steps to toggle hidden files
 */
function getTestCaseStepsForHiddenFiles(basicSet, hiddenEntrySet) {
  var appId;
  return [
    function(id) {
      appId = id;
      remoteCall.waitForElement(appId, '#gear-button').then(this.next);
    },
    // Open the gear menu by clicking the gear button.
    function() {
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, ['#gear-button'], this.next);
    },
    // Wait for menu to not be hidden.
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.waitForElement(appId, '#gear-menu:not([hidden])')
      .then(this.next);
    },
    // Wait for menu item to appear.
    function(result) {
      remoteCall.waitForElement(appId,
          '#gear-menu-toggle-hidden-files:not([disabled])').then(this.next);
    },
    // Wait for menu item to appear.
    function(result) {
      remoteCall.waitForElement(appId,
          '#gear-menu-toggle-hidden-files:not([checked])').then(this.next);
    },
    // Click the menu item.
    function(results) {
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, ['#gear-menu-toggle-hidden-files'],
              this.next);
    },
    // Wait for item to be checked.
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.waitForElement(appId,
          '#gear-menu-toggle-hidden-files[checked]').then(this.next);
    },
    // Check the hidden files are displayed.
    function(result) {
      remoteCall.waitForFiles(appId, TestEntryInfo.getExpectedRows(
          hiddenEntrySet), {ignoreFileSize: true, ignoreLastModifiedTime: true})
          .then(this.next);
    },
    // Repeat steps to toggle again.
    function(inAppId) {
      remoteCall.waitForElement(appId, '#gear-button').then(this.next);
    },
    function() {
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, ['#gear-button'], this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.waitForElement(appId, '#gear-menu:not([hidden])').then(
          this.next);
    },
    function(result) {
      remoteCall.waitForElement(appId,
          '#gear-menu-toggle-hidden-files:not([disabled])').then(this.next);
    },
    function(result) {
      remoteCall.waitForElement(appId,
          '#gear-menu-toggle-hidden-files[checked]').then(this.next);
    },
    function(results) {
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, ['#gear-menu-toggle-hidden-files'],
              this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.waitForElement(appId,
          '#gear-menu-toggle-hidden-files:not([checked])').then(this.next);
    },
    function(result) {
      remoteCall.waitForFiles(appId, TestEntryInfo.getExpectedRows(basicSet),
          {ignoreFileSize: true, ignoreLastModifiedTime: true}).then(this.next);
    },
    function() {
      checkIfNoErrorsOccured(this.next);
    },
  ];
}

/**
 * Test to toggle the show hidden files option on Downloads
 */
testcase.showHiddenFilesDownloads = function() {
  var appId;
  var steps = [
    function() {
      addEntries(['local'], BASIC_LOCAL_ENTRY_SET_WITH_HIDDEN, this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      openNewWindow(null, RootPath.DOWNLOADS).then(this.next);
    },
    function(inAppId) {
      appId = inAppId;
      remoteCall.waitForElement(appId, '#detail-table').then(this.next);
    },
    // Wait for volume list to be initialized.
    function() {
      remoteCall.waitForFileListChange(appId, 0).then(this.next);
    },
    function() {
      this.next(appId);
    },
  ];

  steps = steps.concat(getTestCaseStepsForHiddenFiles(BASIC_LOCAL_ENTRY_SET,
    BASIC_LOCAL_ENTRY_SET_WITH_HIDDEN));
  StepsRunner.run(steps);
};


/**
 * Test to toggle the show hidden files option on Drive
 */
testcase.showHiddenFilesDrive = function() {
  var appId;
  var steps = [
    function() {
      addEntries(['drive'], BASIC_DRIVE_ENTRY_SET_WITH_HIDDEN, this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      openNewWindow(null, RootPath.DRIVE).then(this.next);
    },
    function(inAppId) {
      appId = inAppId;
      remoteCall.waitForElement(appId, '#detail-table').then(this.next);
    },
    // Wait for volume list to be initialized.
    function() {
      remoteCall.waitForFileListChange(appId, 0).then(this.next);
    },
    function() {
      this.next(appId);
    },
  ];

  steps = steps.concat(getTestCaseStepsForHiddenFiles(BASIC_DRIVE_ENTRY_SET,
    BASIC_DRIVE_ENTRY_SET_WITH_HIDDEN));
  StepsRunner.run(steps);
};

/**
 * Test to toggle the Show Google Docs option on Drive
 */
testcase.toogleGoogleDocsDrive = function() {
  var appId;
  StepsRunner.run([
    function() {
      setupAndWaitUntilReady(null, RootPath.DRIVE).then(this.next);
    },
    function(results) {
      appId = results.windowId;
      remoteCall.waitForElement(appId, '#gear-button').then(this.next);
    },
    // Open the gear meny by a shortcut (Alt-E).
    function() {
      remoteCall.fakeKeyDown(appId, 'body', 'e', 'U+0045', false, false, true)
          .then(this.next);
    },
    // Wait for menu to appear.
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.waitForElement(appId, '#gear-menu').then(this.next);
    },
    // Wait for menu to appear.
    function(result) {
      remoteCall.waitForElement(appId,
          '#gear-menu-drive-hosted-settings:not([disabled])').then(this.next);
    },
    function(results) {
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, ['#gear-menu-drive-hosted-settings'],
              this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.waitForFiles(appId, TestEntryInfo.getExpectedRows(
          BASIC_DRIVE_ENTRY_SET_WITHOUT_GDOCS), {ignoreFileSize: true,
          ignoreLastModifiedTime: true}).then(this.next);
    },
    function(inAppId) {
      remoteCall.waitForElement(appId, '#gear-button').then(this.next);
    },
    function() {
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, ['#gear-button'], this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.waitForElement(appId, '#gear-menu').then(this.next);
    },
    function(result) {
      remoteCall.waitForElement(appId,
          '#gear-menu-drive-hosted-settings:not([disabled])').then(this.next);
    },
    function(results) {
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, ['#gear-menu-drive-hosted-settings'],
              this.next);
    },
    function(result) {
      remoteCall.waitForFiles(appId, TestEntryInfo.getExpectedRows(
          BASIC_DRIVE_ENTRY_SET), {ignoreFileSize: true,
          ignoreLastModifiedTime: true}).then(this.next);
    },
    function() {
      checkIfNoErrorsOccured(this.next);
    },
  ]);
};

/**
 * Test for the "paste-into-current-folder" menu item
 */
testcase.showPasteIntoCurrentFolder = function() {
  const entrySet = [ENTRIES.hello, ENTRIES.world];
  var appId;
  StepsRunner.run([
    function() {
      addEntries(['local'], entrySet, this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      openNewWindow(null, RootPath.DOWNLOADS).then(this.next);
    },
    function(inAppId) {
      appId = inAppId;
      remoteCall.waitForElement(appId, '#detail-table').then(this.next);
    },
    // Wait for the expected files to appear in the file list.
    function() {
      remoteCall.waitForFiles(appId, TestEntryInfo.getExpectedRows(entrySet))
          .then(this.next);
    },
    function() {
      remoteCall.waitForElement(appId, '#gear-button').then(this.next);
    },

    // 1. Before selecting entries
    function() {
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, ['#gear-button'], this.next);
    },
    // Wait for menu to appear.
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.waitForElement(appId, '#gear-menu:not([hidden])')
          .then(this.next);
    },
    // #paste-into-current-folder command is shown. It should be disabled
    // because no file has been copied to clipboard.
    function(result) {
      remoteCall
          .waitForElement(
              appId,
              '#gear-menu cr-menu-item' +
                  '[command=\'#paste-into-current-folder\']' +
                  '[disabled]:not([hidden])')
          .then(this.next);
    },
    function() {
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, ['#file-list'], this.next);
    },
    function() {
      remoteCall.waitForElement(appId, '#gear-menu[hidden]').then(this.next);
    },

    // 2. Selecting a single regular file
    function(result) {
      remoteCall.callRemoteTestUtil(
          'selectFile', appId, [ENTRIES.hello.nameText], this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, ['#gear-button'], this.next);
    },
    // Wait for menu to appear.
    // The command is still shown.
    function() {
      remoteCall
          .waitForElement(
              appId,
              '#gear-menu:not([hidden]) cr-menu-item' +
                  '[command=\'#paste-into-current-folder\']' +
                  '[disabled]:not([hidden])')
          .then(this.next);
    },
    function() {
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, ['#file-list'], this.next);
    },
    function() {
      remoteCall.waitForElement(appId, '#gear-menu[hidden]').then(this.next);
    },

    // 3. When ready to paste a file
    function(result) {
      remoteCall.callRemoteTestUtil(
          'selectFile', appId, [ENTRIES.hello.nameText], this.next);
    },
    // Ctrl-C to copy the selected file
    function() {
      remoteCall
          .fakeKeyDown(appId, '#file-list', 'c', 'U+0043', true, false, false)
          .then(this.next);
    },
    function() {
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, ['#gear-button'], this.next);
    },
    // The command appears enabled.
    function() {
      remoteCall
          .waitForElement(
              appId,
              '#gear-menu:not([hidden])' +
                  ' cr-menu-item[command=\'#paste-into-current-folder\']' +
                  ':not([disabled]):not([hidden])')
          .then(this.next);
    },
    function() {
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, ['#file-list'], this.next);
    },
    function() {
      remoteCall.waitForElement(appId, '#gear-menu[hidden]').then(this.next);
    },

    function() {
      checkIfNoErrorsOccured(this.next);
    },
  ]);
};

/**
 * Test for the "select-all" menu item
 */
testcase.showSelectAllInCurrentFolder = function() {
  const entrySet = [ENTRIES.newlyAdded];
  var appId;
  StepsRunner.run([
    function() {
      openNewWindow(null, RootPath.DOWNLOADS).then(this.next);
    },
    function(inAppId) {
      appId = inAppId;
      remoteCall.waitForElement(appId, '#detail-table').then(this.next);
    },
    function() {
      remoteCall.waitForElement(appId, '#gear-button').then(this.next);
    },

    // 1. Before selecting entries
    function() {
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, ['#gear-button'], this.next);
    },
    // Wait for menu to appear.
    function() {
      remoteCall.waitForElement(appId, '#gear-menu:not([hidden])')
          .then(this.next);
    },
    // #select-all command is shown. It should be disabled
    // because no files yet
    function(result) {
      remoteCall
          .waitForElement(
              appId,
              '#gear-menu cr-menu-item' +
                  '[command=\'#select-all\']' +
                  '[disabled]:not([hidden])')
          .then(this.next);
    },
    // Hide gear menu
    function() {
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, ['#file-list'], this.next);
    },
    function() {
      remoteCall.waitForElement(appId, '#gear-menu[hidden]').then(this.next);
    },
    // Add new file
    function() {
      addEntries(['local'], entrySet, this.next);
    },
    // Wait for the new files to appear in the file list.
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.waitForFiles(appId, TestEntryInfo.getExpectedRows(entrySet))
          .then(this.next);
    },
    // Re-show gear menu again
    function(result) {
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, ['#gear-button'], this.next);
    },
    // Wait for menu to appear.
    // The 'Select all' command appears enabled.
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall
          .waitForElement(
              appId,
              '#gear-menu:not([hidden]) cr-menu-item' +
                  '[command=\'#select-all\']' +
                  ':not([disabled]):not([hidden])')
          .then(this.next);
    },
    // Click 'Select all' menu item.
    function(results) {
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, ['#gear-menu-select-all'], this.next);
    },
    // Wait for selection.
    function() {
      remoteCall.waitForElement(appId, '#file-list li[selected]')
          .then(this.next);
    },
    // Hide gear menu
    function() {
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, ['#file-list'], this.next);
    },
    function() {
      remoteCall.waitForElement(appId, '#gear-menu[hidden]').then(this.next);
    },
    function() {
      checkIfNoErrorsOccured(this.next);
    },
  ]);
};
