// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Constants for interacting with the directory tree on the LHS of Files.
 * When we are not in guest mode, we fill Google Drive with the basic entry set
 * which causes an extra tree-item to be added.
 */
var TREEITEM_DRIVE = '#directory-tree > div:nth-child(1) ';
var TREEITEM_DOWNLOADS = '#directory-tree > div:nth-child(2) ';
var EXPAND_ICON = '> .tree-row > .expand-icon';
var EXPANDED_SUBTREE = '> .tree-children[expanded]';
var NEWFOLDER = '#tree-item-autogen-id-9';
var TESTFOLDER  = '#tree-item-autogen-id-10';
var NEWFOLDER_GUEST = '#tree-item-autogen-id-7';
var TESTFOLDER_GUEST  = '#tree-item-autogen-id-8';

/**
 * Selects the first item in the file list.
 * @param {string} windowId ID of the target window.
 * @return {Promise} Promise to be fulfilled on success.
 */
function selectFirstListItem(windowId) {
  return Promise.resolve().then(function() {
    // Ensure no selected item.
    return remoteCall.waitForElementLost(
        windowId,
        'div.detail-table > list > li[selected]');
  }).then(function() {
    // Push Down.
    return remoteCall.callRemoteTestUtil(
        'fakeKeyDown', windowId,
        // Down
        ['#file-list', 'ArrowDown', 'Down', true, false, false]);
  }).then(function() {
    // Wait for selection.
    return remoteCall.waitForElement(windowId,
                                     'div.detail-table > list > li[selected]');
  }).then(function() {
    // Ensure that only the first item is selected.
    return remoteCall.callRemoteTestUtil(
        'queryAllElements',
        windowId,
        ['div.detail-table > list > li[selected]']);
  }).then(function(elements) {
    chrome.test.assertEq(1, elements.length);
    chrome.test.assertEq('listitem-1', elements[0].attributes['id']);
  });
}

/**
 * Creates new folder.
 * @param {string} windowId ID of the target window.
 * @param {string} path Initial path.
 * @param {Array<TestEntryInfo>} initialEntrySet Initial set of entries.
 * @return {Promise} Promise to be fulfilled on success.
 */
function createNewFolder(windowId, path, initialEntrySet) {
  var caller = getCaller();
  return Promise.resolve(
  ).then(function() {
    // Push Ctrl + E.
    return remoteCall.callRemoteTestUtil(
        'fakeKeyDown', windowId,
        // Ctrl + E
        ['#file-list', 'e', 'U+0045', true, false, false]);
  }).then(function() {
    // Wait for rename text field.
    return remoteCall.waitForElement(windowId, 'li[renaming] input.rename');
  }).then(function() {
    return remoteCall.callRemoteTestUtil(
        'queryAllElements',
        windowId,
        ['div.detail-table > list > li[selected]']);
  }).then(function(elements) {
    // Ensure that only the new directory is selected and being renamed.
    chrome.test.assertEq(1, elements.length);
    chrome.test.assertTrue('renaming' in elements[0].attributes);
  }).then(function() {
    // Check directory tree for new folder.
    if (chrome.extension.inIncognitoContext)
      return remoteCall.waitForElement(windowId, NEWFOLDER_GUEST);
    else
      return remoteCall.waitForElement(windowId, NEWFOLDER);
  }).then(function() {
    // Type new folder name.
    return remoteCall.callRemoteTestUtil(
        'inputText', windowId, ['input.rename', 'Test Folder Name']);
  }).then(function() {
    // Push Enter.
    return remoteCall.callRemoteTestUtil(
        'fakeKeyDown',
        windowId,
        ['input.rename', 'Enter', 'Enter', false, false, false]);
  }).then(function() {
    // Wait until rename completes.
    return remoteCall.waitForElementLost(windowId, 'input.rename');
  }).then(function() {
     // Once it is renamed, the original 'New Folder' item is removed.
     if (chrome.extension.inIncognitoContext)
       return remoteCall.waitForElementLost(windowId, NEWFOLDER_GUEST);
     else
       return remoteCall.waitForElementLost(windowId, NEWFOLDER);
  }).then(function() {
    // A newer entry is then added for the renamed folder.
    if (chrome.extension.inIncognitoContext)
      return remoteCall.waitForElement(windowId, TESTFOLDER_GUEST);
    else
      return remoteCall.waitForElement(windowId, TESTFOLDER);
  }).then(function() {
    var expectedEntryRows = TestEntryInfo.getExpectedRows(initialEntrySet);
    expectedEntryRows.push(['Test Folder Name', '--', 'Folder', '']);
    // Wait for the new folder.
    return remoteCall.waitForFiles(windowId,
                                   expectedEntryRows,
                                   {ignoreLastModifiedTime: true});
  }).then(function() {
    // Wait until the new created folder is selected.
    var nameSpanQuery = 'div.detail-table > list > ' +
                        'li[selected]:not([renaming]) span.entry-name';

    return repeatUntil(function() {
      var selectedNameRetrievePromise = remoteCall.callRemoteTestUtil(
            'queryAllElements',
            windowId,
            ['div.detail-table > list > li[selected] span.entry-name']);

      return selectedNameRetrievePromise.then(function(elements) {
        if (elements.length !== 1) {
          return pending(
              caller, 'Selection is not ready (elements: %j)', elements);
        } else if (elements[0].text !== 'Test Folder Name') {
          return pending(
              caller, 'Selected item is wrong. (actual: %s)', elements[0].text);
        } else {
          return true;
        }
      });
    });
  });
}

/**
 * This is used to expand the tree item for Downloads or Drive.
 * @param {string} windowId The Files app window.
 * @param {string} selector The Downloads or Drive tree item selector.
 * @return {Promise} Promise fulfilled on success.
 */
function expandRoot(windowId, selector) {
  return remoteCall.waitForElement(
      windowId, selector + EXPAND_ICON).then(function() {
    return remoteCall.callRemoteTestUtil(
        'fakeMouseClick', windowId, [selector + EXPAND_ICON]);
  }).then(function(result) {
    chrome.test.assertTrue(result);
    return remoteCall.waitForElement(windowId,
        selector + EXPANDED_SUBTREE);
  });
}

testcase.selectCreateFolderDownloads = function() {
  var PATH = RootPath.DOWNLOADS;
  var windowId = null;
  var promise = new Promise(function(callback) {
    setupAndWaitUntilReady(null, PATH, callback);
  }).then(function(results) {
    windowId = results.windowId;
    return selectFirstListItem(windowId);
  }).then(function() {
    return expandRoot(windowId, TREEITEM_DOWNLOADS);
  }).then(function() {
    return remoteCall.waitForElement(windowId, '#detail-table');
  }).then(function() {
    return createNewFolder(windowId, PATH, BASIC_LOCAL_ENTRY_SET);
  });

  testPromise(promise);
};

testcase.createFolderDownloads = function() {
  var PATH = RootPath.DOWNLOADS;
  var windowId = null;
  var promise = new Promise(function(callback) {
    setupAndWaitUntilReady(null, PATH, callback);
  }).then(function(results) {
    windowId = results.windowId;
    return expandRoot(windowId, TREEITEM_DOWNLOADS);
  }).then(function() {
    return remoteCall.waitForElement(windowId, '#detail-table');
  }).then(function() {
    return createNewFolder(windowId, PATH, BASIC_LOCAL_ENTRY_SET);
  });

  testPromise(promise);
};

testcase.createFolderDrive = function() {
  var PATH = RootPath.DRIVE;
  var windowId = null;
  var promise = new Promise(function(callback) {
    setupAndWaitUntilReady(null, PATH, callback);
  }).then(function(results) {
    windowId = results.windowId;
    return expandRoot(windowId, TREEITEM_DRIVE);
  }).then(function() {
    return remoteCall.waitForElement(windowId, '#detail-table');
  }).then(function() {
    return createNewFolder(windowId, PATH, BASIC_DRIVE_ENTRY_SET);
  });

  testPromise(promise);
};
