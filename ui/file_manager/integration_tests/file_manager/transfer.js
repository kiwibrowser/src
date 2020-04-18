// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Test function to copy from the specified source to the specified destination.
 * @param {TestEntryInfo} targetFile TestEntryInfo of target file to be copied.
 * @param {string} srcName Type of source volume. e.g. downloads, drive,
 *     drive_recent, drive_shared_with_me, drive_offline.
 * @param {Array<TestEntryInfo>} srcEntries Expected initial contents in the
 *     source volume.
 * @param {string} dstName Type of destination volume.
 * @param {Array<TestEntryInfo>} dstEntries Expected initial contents in the
 *     destination volume.
 */
function copyBetweenVolumes(targetFile,
                            srcName,
                            srcEntries,
                            dstName,
                            dstEntries) {
  var srcContents = TestEntryInfo.getExpectedRows(srcEntries).sort();
  var dstContents = TestEntryInfo.getExpectedRows(dstEntries).sort();

  var appId;
  StepsRunner.run([
    // Set up File Manager.
    function() {
      setupAndWaitUntilReady(null, RootPath.DOWNLOADS, this.next);
    },
    // Select the source volume.
    function(results) {
      appId = results.windowId;
      remoteCall.callRemoteTestUtil(
          'selectVolume', appId, [srcName], this.next);
    },
    // Wait for the expected files to appear in the file list.
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.waitForFiles(appId, srcContents).then(this.next);
    },
    // Set focus on the file list.
    function() {
      remoteCall.callRemoteTestUtil(
          'focus', appId, ['#file-list:not([hidden])'], this.next);
    },
    // Select the source file.
    function() {
      remoteCall.callRemoteTestUtil(
          'selectFile', appId, [targetFile.nameText], this.next);
    },
    // Copy the file.
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.callRemoteTestUtil('execCommand', appId, ['copy'], this.next);
    },
    // Select the destination volume.
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.callRemoteTestUtil(
          'selectVolume', appId, [dstName], this.next);
    },
    // Wait for the expected files to appear in the file list.
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.waitForFiles(appId, dstContents).then(this.next);
    },
    // Paste the file.
    function() {
      remoteCall.callRemoteTestUtil('execCommand', appId, ['paste'], this.next);
    },
    // Wait for the file list to change.
    function(result) {
      chrome.test.assertTrue(result);
      var ignoreFileSize =
          srcName == 'drive_shared_with_me' ||
          srcName == 'drive_offline' ||
          dstName == 'drive_shared_with_me' ||
          dstName == 'drive_offline';
      var dstContentsAfterPaste = dstContents.slice();
      var pasteFile = targetFile.getExpectedRow();
      for (var i = 0; i < dstContentsAfterPaste.length; i++) {
        if (dstContentsAfterPaste[i][0] === pasteFile[0]) {
          // Replace the last '.' in filename with ' (1).'.
          // e.g. 'my.note.txt' -> 'my.note (1).txt'
          pasteFile[0] = pasteFile[0].replace(/\.(?=[^\.]+$)/, ' (1).');
          break;
        }
      }
      dstContentsAfterPaste.push(pasteFile);
      remoteCall.waitForFiles(appId, dstContentsAfterPaste, {
        ignoreFileSize: ignoreFileSize,
        ignoreLastModifiedTime: true
      }).then(this.next);
    },
    // Check the last contents of file list.
    function() {
      checkIfNoErrorsOccured(this.next);
    }
  ]);
}

/**
 * Tests copying from Drive to Downloads.
 */
testcase.transferFromDriveToDownloads = function() {
  copyBetweenVolumes(
      ENTRIES.hello,
      'drive',
      BASIC_DRIVE_ENTRY_SET,
      'downloads',
      BASIC_LOCAL_ENTRY_SET);
};

/**
 * Tests copying from Downloads to Drive.
 */
testcase.transferFromDownloadsToDrive = function() {
  copyBetweenVolumes(
      ENTRIES.hello,
      'downloads',
      BASIC_LOCAL_ENTRY_SET,
      'drive',
      BASIC_DRIVE_ENTRY_SET);
};

/**
 * Tests copying from Drive shared with me to Downloads.
 */
testcase.transferFromSharedToDownloads = function() {
  copyBetweenVolumes(
      ENTRIES.testSharedDocument,
      'drive_shared_with_me',
      SHARED_WITH_ME_ENTRY_SET,
      'downloads',
      BASIC_LOCAL_ENTRY_SET);
};

/**
 * Tests copying from Drive shared with me to Drive.
 */
testcase.transferFromSharedToDrive = function() {
  copyBetweenVolumes(
      ENTRIES.testSharedDocument,
      'drive_shared_with_me',
      SHARED_WITH_ME_ENTRY_SET,
      'drive',
      BASIC_DRIVE_ENTRY_SET);
};

/**
 * Tests copying from Drive offline to Downloads.
 */
testcase.transferFromOfflineToDownloads = function() {
  copyBetweenVolumes(
      ENTRIES.testDocument,
      'drive_offline',
      OFFLINE_ENTRY_SET,
      'downloads',
      BASIC_LOCAL_ENTRY_SET);
};

/**
 * Tests copying from Drive offline to Drive.
 */
testcase.transferFromOfflineToDrive = function() {
  copyBetweenVolumes(
      ENTRIES.testDocument,
      'drive_offline',
      OFFLINE_ENTRY_SET,
      'drive',
      BASIC_DRIVE_ENTRY_SET);
};
