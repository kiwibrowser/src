// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Include test fixture.
GEN_INCLUDE(['net_internals_test.js']);

// Anonymous namespace
(function() {

function DebugLogsStatusWatcher() {
  NetInternalsTest.Task.call(this);
  this.setCompleteAsync(true);
}

DebugLogsStatusWatcher.prototype = {
  __proto__: NetInternalsTest.Task.prototype,

  start: function() {
    g_browser.addStoreDebugLogsObserver(this);
    $('chromeos-view-store-debug-logs').click();
  },

  onStoreDebugLogs: function(status) {
    if (status.indexOf('Created') != -1) {
      this.onTaskDone(true);
      return;
    }
    if (status.indexOf('Failed') != -1) {
      this.onTaskDone(false);
      return;
    }
  }
};

function ResultChecker() {
  NetInternalsTest.Task.call(this);
}

ResultChecker.prototype = {
  __proto__: NetInternalsTest.Task.prototype,

  start: function(success) {
    assertTrue(success);
    this.onTaskDone();
  }
};

TEST_F(
    'NetInternalsTest', 'netInternalsChromeOSViewStoreDebugLogs', function() {
      if (!cr.isChromeOS) {
        testDone();
        return;
      }

      // #chromeos-view-import-onc fails accessibility check.
      this.runAccessibilityChecks = false;
      NetInternalsTest.switchToView('chromeos');

      var taskQueue = new NetInternalsTest.TaskQueue(true);
      taskQueue.addTask(new DebugLogsStatusWatcher());
      taskQueue.addTask(new ResultChecker());
      taskQueue.run();
    });

})();  // Anonymous namespace
