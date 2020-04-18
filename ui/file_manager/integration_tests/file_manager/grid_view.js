// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Shows the grid view and checks the label texts of entries.
 *
 * @param {string} rootPath Root path to be used as a default current directory
 *     during initialization. Can be null, for no default path.
 * @param {Array<TestEntryInfo>} expectedSet Set of entries that are expected
 *     to appear in the grid view.
 * @return {Promise} Promise to be fulfilled or rejected depending on the test
 *     result.
 */
function showGridView(rootPath, expectedSet) {
  var caller = getCaller();
  var expectedLabels = expectedSet.map(function(entryInfo) {
    return entryInfo.nameText;
  }).sort();
  var setupPromise = setupAndWaitUntilReady(null, rootPath);
  return setupPromise.then(function(results) {
    var windowId = results.windowId;
    // Click the grid view button.
    var clickedPromise = remoteCall.waitForElement(windowId,
                                                   '#view-button').
        then(function() {
          return remoteCall.callRemoteTestUtil(
              'fakeEvent', windowId, ['#view-button', 'click']);
        });

    // Compare the grid labels of the entries.
    return clickedPromise.then(function() {
      return repeatUntil(function() {
        var labelsPromise = remoteCall.callRemoteTestUtil(
            'queryAllElements',
            windowId,
            ['grid:not([hidden]) .thumbnail-item .entry-name']);
        return labelsPromise.then(function(labels) {
          var actualLabels = labels.map(function(label) {
            return label.text;
          }).sort();
          if (chrome.test.checkDeepEq(expectedLabels, actualLabels))
            return true;
          return pending(
              caller,
              'Failed to compare the grid lables, expected: %j, actual %j.',
              expectedLabels, actualLabels);
        });
      });
    });
  });
}

/**
 * Tests to show grid view on a local directory.
 */
testcase.showGridViewDownloads = function() {
  testPromise(showGridView(
      RootPath.DOWNLOADS, BASIC_LOCAL_ENTRY_SET));
};

/**
 * Tests to show grid view on a drive directory.
 */
testcase.showGridViewDrive = function() {
  testPromise(showGridView(
      RootPath.DRIVE, BASIC_DRIVE_ENTRY_SET));
};
