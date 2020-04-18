// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Fake task.
 *
 * @param {boolean} isDefault Whether the task is default or not.
 * @param {string} taskId Task ID.
 * @param {string} title Title of the task.
 * @param {boolean=} opt_isGenericFileHandler Whether the task is a generic
 *     file handler.
 * @constructor
 */
function FakeTask(isDefault, taskId, title, opt_isGenericFileHandler) {
  this.driveApp = false;
  this.iconUrl = 'chrome://theme/IDR_DEFAULT_FAVICON';  // Dummy icon
  this.isDefault = isDefault;
  this.taskId = taskId;
  this.title = title;
  this.isGenericFileHandler = opt_isGenericFileHandler || false;
  Object.freeze(this);
}

/**
 * Fake tasks for a local volume.
 *
 * @type {Array<FakeTask>}
 * @const
 */
var DOWNLOADS_FAKE_TASKS = [
  new FakeTask(true, 'dummytaskid|open-with', 'DummyTask1'),
  new FakeTask(false, 'dummytaskid-2|open-with', 'DummyTask2')
];

/**
 * Fake tasks for a drive volume.
 *
 * @type {Array<FakeTask>}
 * @const
 */
var DRIVE_FAKE_TASKS = [
  new FakeTask(true, 'dummytaskid|drive|open-with', 'DummyTask1'),
  new FakeTask(false, 'dummytaskid-2|drive|open-with', 'DummyTask2')
];

/**
 * Sets up task tests.
 *
 * @param {string} rootPath Root path.
 * @param {Array<FakeTask>} fakeTasks Fake tasks.
 */
function setupTaskTest(rootPath, fakeTasks) {
  return setupAndWaitUntilReady(null, rootPath).then(function(results) {
    return remoteCall.callRemoteTestUtil(
        'overrideTasks',
        results.windowId,
        [fakeTasks]).then(function() {
      return results.windowId;
    });
  });
}

/**
 * Tests executing the default task when there is only one task.
 *
 * @param {string} expectedTaskId Task ID expected to execute.
 * @param {string} windowId Window ID.
 */
function executeDefaultTask(expectedTaskId, windowId) {
  // Select file.
  var selectFilePromise =
      remoteCall.callRemoteTestUtil('selectFile', windowId, ['hello.txt']);

  // Double-click the file.
  var doubleClickPromise = selectFilePromise.then(function(result) {
    chrome.test.assertTrue(result);
    return remoteCall.callRemoteTestUtil(
        'fakeMouseDoubleClick',
        windowId,
        ['#file-list li.table-row[selected] .filename-label span']);
  });

  // Wait until the task is executed.
  return doubleClickPromise.then(function(result) {
    chrome.test.assertTrue(!!result);
    return remoteCall.waitUntilTaskExecutes(windowId, expectedTaskId);
  });
}

/**
 * Tests to specify default task via the default task dialog.
 *
 * @param {string} expectedTaskId Task ID to be expected to newly specify as
 *     default.
 * @param {string} windowId Window ID.
 * @return {Promise} Promise to be fulfilled/rejected depends on the test
 *     result.
 */
function defaultTaskDialog(expectedTaskId, windowId) {
  // Prepare expected labels.
  var expectedLabels = [
    'DummyTask1 (default)',
    'DummyTask2'
  ];

  // Select file.
  var selectFilePromise =
      remoteCall.callRemoteTestUtil('selectFile', windowId, ['hello.txt']);

  // Click the change default menu.
  var menuClickedPromise = selectFilePromise.
      then(function() {
        return remoteCall.waitForElement(windowId, '#tasks[multiple]');
      }).
      then(function() {
        return remoteCall.waitForElement(
            windowId, '#tasks-menu .change-default');
      }).
      then(function() {
        return remoteCall.callRemoteTestUtil(
            'fakeEvent', windowId, ['#tasks', 'select', {
              item: {type: 'ChangeDefaultTask'}
            }]);
      }).
      then(function(result) {
        chrome.test.assertTrue(result);
      });

  var caller = getCaller();
  // Wait for the list of menu item is added as expected.
  var menuPreparedPromise = menuClickedPromise.then(function() {
    return repeatUntil(function() {
      // Obtains menu items.
      var menuItemsPromise = remoteCall.callRemoteTestUtil(
          'queryAllElements',
          windowId,
          ['#default-task-dialog #default-tasks-list li']);

      // Compare the contents of items.
      return menuItemsPromise.then(function(items) {
        var actualLabels = items.map(function(item) { return item.text; });
        if (chrome.test.checkDeepEq(expectedLabels, actualLabels)) {
          return true;
        } else {
          return pending(
              caller, 'Tasks do not match, expected: %j, actual: %j.',
              expectedLabels, actualLabels);
        }
      });
    });
  });

  // Click the non default item.
  var itemClickedPromise = menuPreparedPromise.
      then(function() {
        return remoteCall.callRemoteTestUtil(
            'fakeEvent',
            windowId,
            [
              '#default-task-dialog #default-tasks-list li:nth-of-type(2)',
              'mousedown',
              {bubbles: true, button: 0}
            ]);
      }).
      then(function() {
        return remoteCall.callRemoteTestUtil(
            'fakeEvent',
            windowId,
            [
              '#default-task-dialog #default-tasks-list li:nth-of-type(2)',
              'click',
              {bubbles: true}
            ]);
      }).
      then(function(result) {
        chrome.test.assertTrue(result);
      });

  // Wait for the dialog hidden, and the task is executed.
  var dialogHiddenPromise = itemClickedPromise.then(function() {
    return remoteCall.waitForElementLost(
        windowId, '#default-task-dialog', null);
  });

  // Execute the new default task.
  var taskButtonClicked =
      dialogHiddenPromise
          .then(function() {
            // Click on "Open ▼" button.
            remoteCall.callRemoteTestUtil(
                'fakeMouseClick', windowId, ['#tasks']);
            // Wait for dropdown menu to show.
            return remoteCall.waitForElement(
                windowId, '#tasks-menu cr-menu-item');
          })
          .then(function(result) {
            // Click on first menu item.
            remoteCall.callRemoteTestUtil(
                'fakeMouseClick', windowId,
                ['#tasks-menu cr-menu-item:nth-child(1)']);
            // Wait dropdown menu to hide.
            return remoteCall.waitForElement(windowId, '#tasks-menu[hidden]');
          })
          .then(function(result) {
            chrome.test.assertTrue(!!result);
          });

  // Check the executed tasks.
  return taskButtonClicked.then(function() {
    return remoteCall.waitUntilTaskExecutes(windowId, expectedTaskId);
  });
}

testcase.executeDefaultTaskDrive = function() {
  testPromise(setupTaskTest(RootPath.DRIVE, DRIVE_FAKE_TASKS).then(
      executeDefaultTask.bind(null, 'dummytaskid|drive|open-with')));
};

testcase.executeDefaultTaskDownloads = function() {
  testPromise(setupTaskTest(RootPath.DOWNLOADS, DOWNLOADS_FAKE_TASKS).then(
      executeDefaultTask.bind(null, 'dummytaskid|open-with')));
};

testcase.defaultTaskDialogDrive = function() {
  testPromise(setupTaskTest(RootPath.DRIVE, DRIVE_FAKE_TASKS).then(
      defaultTaskDialog.bind(null, 'dummytaskid-2|drive|open-with')));
};

testcase.defaultTaskDialogDownloads = function() {
  testPromise(setupTaskTest(RootPath.DOWNLOADS, DOWNLOADS_FAKE_TASKS).then(
      defaultTaskDialog.bind(null, 'dummytaskid-2|open-with')));
};

testcase.genericTaskIsNotExecuted = function() {
  var tasks = [
    new FakeTask(false, 'dummytaskid|open-with', 'DummyTask1',
        true /* isGenericFileHandler */)
  ];

  // When default task is not set, executeDefaultInternal_ in file_tasks.js
  // tries to show it in a browser tab. By checking the view-in-browser task is
  // executed, we check that default task is not set in this situation.
  //
  // See: src/ui/file_manager/file_manager/foreground/js/file_tasks.js&l=404
  testPromise(setupTaskTest(RootPath.DOWNLOADS, tasks)
    .then(function(windowId) {
      return executeDefaultTask(
          FILE_MANAGER_EXTENSIONS_ID + '|file|view-in-browser',
          windowId);
    }));
};

testcase.genericTaskAndNonGenericTask = function() {
  var tasks = [
    new FakeTask(false, 'dummytaskid|open-with', 'DummyTask1',
        true /* isGenericFileHandler */),
    new FakeTask(false, 'dummytaskid-2|open-with', 'DummyTask2',
        false /* isGenericFileHandler */),
    new FakeTask(false, 'dummytaskid-3|open-with', 'DummyTask3',
        true /* isGenericFileHandler */)
  ];

  testPromise(setupTaskTest(RootPath.DOWNLOADS, tasks).then(
    executeDefaultTask.bind(null, 'dummytaskid-2|open-with')));
};
