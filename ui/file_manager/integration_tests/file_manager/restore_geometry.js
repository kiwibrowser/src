// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Tests restoring window geometry of the Files app.
 */
testcase.restoreGeometry = function() {
  var appId;
  var appId2;
  StepsRunner.run([
    // Set up File Manager.
    function() {
      setupAndWaitUntilReady(null, RootPath.DOWNLOADS, this.next);
    },
    // Resize the window to minimal dimensions.
    function(results) {
      appId = results.windowId;
      remoteCall.callRemoteTestUtil(
          'resizeWindow', appId, [640, 480], this.next);
    },
    // Check the current window's size.
    function() {
      remoteCall.waitForWindowGeometry(appId, 640, 480).then(this.next);
    },
    // Enlarge the window by 10 pixels.
    function(result) {
      remoteCall.callRemoteTestUtil(
          'resizeWindow', appId, [650, 490], this.next);
    },
    // Check the current window's size.
    function() {
      remoteCall.waitForWindowGeometry(appId, 650, 490).then(this.next);
    },
    // Open another window, where the current view is restored.
    function() {
      setupAndWaitUntilReady(null, RootPath.DOWNLOADS, this.next);
    },
    // Check the next window's size.
    function(results) {
      appId2 = results.windowId;
      remoteCall.waitForWindowGeometry(appId2, 650, 490).then(this.next);
    },
    // Check for errors.
    function() {
      checkIfNoErrorsOccured(this.next);
    }
  ]);
};

/**
 * Tests restoring a maximized Files app window.
 */
testcase.restoreGeometryMaximized = function() {
  var appId;
  var appId2;
  var caller = getCaller();
  StepsRunner.run([
    // Set up File Manager.
    function() {
      setupAndWaitUntilReady(null, RootPath.DOWNLOADS, this.next);
    },
    // Maximize the window
    function(results) {
      appId = results.windowId;
      remoteCall.callRemoteTestUtil('maximizeWindow', appId, [], this.next);
    },
    // Check that the first window is maximized.
    function() {
      return repeatUntil(function() {
        return remoteCall.callRemoteTestUtil('isWindowMaximized', appId, [])
            .then(function(isMaximized) {
              if (isMaximized)
                return true;
              else
                return pending(caller, 'Waiting window maximized...');
            });
      }).then(this.next);
    },
    // Close the window.
    function() {
      remoteCall.closeWindowAndWait(appId).then(this.next);
    },
    // Open a Files app window again.
    function() {
      setupAndWaitUntilReady(null, RootPath.DOWNLOADS, this.next);
    },
    // Check that the newly opened window is maximized initially.
    function(results) {
      appId2 = results.windowId;
      return repeatUntil(function() {
        return remoteCall.callRemoteTestUtil('isWindowMaximized', appId2, [])
            .then(function(isMaximized) {
              if (isMaximized)
                return true;
              else
                return pending(caller, 'Waiting window maximized...');
            });
      }).then(this.next);
    },
    function() {
      checkIfNoErrorsOccured(this.next);
    }
  ]);
};
