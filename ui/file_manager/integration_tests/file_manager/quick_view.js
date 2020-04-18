// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

function openQuickViewSteps(appId, filename) {
  var caller = getCaller();
  return [
    function(results) {
      remoteCall.callRemoteTestUtil('selectFile', appId, [filename], this.next);
    },
    function(results) {
      chrome.test.assertTrue(results);
      // Press Space key.
      remoteCall.callRemoteTestUtil(
          'fakeKeyDown', appId,
          ['#file-list', ' ', ' ', false, false, false], this.next);
    },
    function(results) {
      chrome.test.assertTrue(results);

      // Wait until Quick View is displayed.
      repeatUntil(function() {
        return remoteCall
            .callRemoteTestUtil(
                'deepQueryAllElements', appId,
                [['#quick-view', '#dialog'], null, ['display']])
            .then(function(results) {
              if (results.length === 0 ||
                  results[0].styles.display === 'none') {
                return pending(caller, 'Quick View is not opened yet.');
              }
              return results;
            });
      }).then(this.next);
    },
    function(results) {
      chrome.test.assertEq(1, results.length);
      // Check Quick View dialog is displayed.
      chrome.test.assertEq('block', results[0].styles.display);
      checkIfNoErrorsOccured(this.next);
    },
  ];
}

function closeQuickViewSteps(appId) {
  return [
    function() {
      return remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, [['#quick-view', '#contentPanel']],
          this.next);
    },
    function(result) {
      chrome.test.assertEq(true, result);
      // Wait until Quick View is displayed.
      repeatUntil(function() {
        return remoteCall
            .callRemoteTestUtil(
                'deepQueryAllElements', appId,
                [['#quick-view', '#dialog'], null, ['display']])
            .then(function(results) {
              if (results.length > 0 && results[0].styles.display !== 'none') {
                return pending(caller, 'Quick View is not closed yet.');
              }
              return;
            });
      }).then(this.next);
    }
  ];
}

/**
 * Tests opening Quick View.
 */
testcase.openQuickView = function() {
  setupAndWaitUntilReady(null, RootPath.DOWNLOADS).then(function(results) {
    StepsRunner.run(
        openQuickViewSteps(results.windowId, 'My Desktop Background.png'));
  });
};

/**
 * Tests closing Quick View.
 */
testcase.closeQuickView = function() {
  setupAndWaitUntilReady(null, RootPath.DOWNLOADS).then(function(results) {
    StepsRunner.run(
        openQuickViewSteps(results.windowId, 'My Desktop Background.png')
            .concat(closeQuickViewSteps(results.windowId)));
  });
};
