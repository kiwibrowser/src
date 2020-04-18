// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @type {!TaskQueue} */
var queue;

/** @type {!Object<importer.TaskQueue.UpdateType, number>} */
var updates = {};

function setUp() {
  queue = new importer.TaskQueue();

  // Set up a callback to log updates from running tasks.
  for (var updateType in importer.TaskQueue.UpdateType) {
    // Reset counts for all update types.
    updates[importer.TaskQueue.UpdateType[updateType]] = 0;
  }

  // Counts the number of updates of each type that have been received.
  var updateCallback = function(type, updatedTask) {
    updates[type]++;
  };
  queue.addUpdateCallback(updateCallback);
}

/**
 * A Task subclass for testing.
 * @constructor
 * @extends {importer.TaskQueue.BaseTask}
 *
 * @param {string} taskId
 */
var TestTask = function(taskId) {
  importer.TaskQueue.BaseTask.call(this, taskId);

  /** @type {boolean} */
  this.wasRun = false;

  /**
   * @private {Function}
   */
  this.runResolver_ = null;

  this.runPromise_ = new Promise(function(resolve) {
    this.runResolver_ = resolve;
  }.bind(this));
};
TestTask.prototype.__proto__ = importer.TaskQueue.BaseTask.prototype;

/** @override */
TestTask.prototype.run = function() {
  this.wasRun = true;
  this.runResolver_(this);
};

/** Sends a quick error notification. */
TestTask.prototype.notifyError = function() {
  this.notify(importer.TaskQueue.UpdateType.ERROR);
};

/** Sends a quick completion notification. */
TestTask.prototype.notifyComplete = function() {
  this.notify(importer.TaskQueue.UpdateType.COMPLETE);
};

/** Sends a quick cancelled notification. */
TestTask.prototype.notifyCanceled = function() {
  this.notify(importer.TaskQueue.UpdateType.CANCELED);
};

/** Sends a quick progress notification. */
TestTask.prototype.notifyProgress = function() {
  this.notify(importer.TaskQueue.UpdateType.PROGRESS);
};

/** @return {!Promise} A promise that settles once #run is called. */
TestTask.prototype.whenRun = function() {
  return this.runPromise_;
};

// Verifies that a queued task gets run.
function testRunsTask(callback) {
  var task = new TestTask('task0');
  queue.queueTask(task);
  reportPromise(task.whenRun(), callback);
}

// Verifies that multiple queued tasks get run.
function testRunsTasks(callback) {
  var task0 = new TestTask('task0');
  var task1 = new TestTask('task1');

  // Make the tasks call Task#notifyComplete when they are run.
  task0.whenRun().then(function(task) { task.notifyComplete(); });
  task1.whenRun().then(function(task) { task.notifyComplete(); });

  // Enqueue both tasks, and then verify that they were run.
  queue.queueTask(task0);
  queue.queueTask(task1);
  reportPromise(
      Promise.all([task0.whenRun(), task1.whenRun()]),
      callback);
}

// Verifies that the active callback triggers when the queue starts doing work
function testOnActiveCalled(callback) {
  var task = new TestTask('task0');

  // Make a promise that resolves when the active callback is triggered.
  var whenActive = new Promise(function(resolve) {
    queue.setActiveCallback(
        function() {
          // Verify that the active callback is called before the task runs.
          assertFalse(task.wasRun);
          resolve();
        });
  });

  // Queue a task, and then check that the active callback was triggered.
  queue.queueTask(task);
  reportPromise(whenActive, callback);
}

// Verifies that the idle callback triggers when the queue is empty.
function testOnIdleCalled(callback) {
  var task = new TestTask('task0');

  task.whenRun().then(function(task) { task.notifyComplete(); });

  // Make a promise that resolves when the idle callback is triggered
  // (i.e. after all queued tasks have finished running).
  var whenDone = new Promise(function(resolve) {
    queue.setIdleCallback(
        function() {
          // Verify that the idle callback is called after the task runs.
          assertTrue(task.wasRun);
          resolve();
        });
  });

  // Queue a task, then check that the idle callback was triggered.
  queue.queueTask(task);
  reportPromise(whenDone, callback);
}

// Verifies that the update callback is called when a task reports progress.
function testProgressUpdate(callback) {
  var task = new TestTask('task0');

  // Get the task to report some progress, then success, when it's run.
  task.whenRun()
      .then(
          function(task) {
            task.notifyProgress();
            return task;
          })
      .then(
          function(task) {
            task.notifyComplete();
            return task;
          });

  // Make a promise that resolves after the task runs.
  var whenDone = new Promise(function(resolve) {
    queue.setIdleCallback(
        function() {
          // Verify that progress was recorded.
          assertEquals(1, updates[importer.TaskQueue.UpdateType.PROGRESS]);
          resolve();
        });
  });

  queue.queueTask(task);
  reportPromise(whenDone, callback);
}

// Verifies that the update callback is called to report successful task
// completion.
function testSuccessUpdate(callback) {
  var task = new TestTask('task0');

  // Get the task to report success when it's run.
  task.whenRun().then(function(task) { task.notifyComplete(); });

  queue.queueTask(task);

  var whenDone = new Promise(function(resolve) {
    queue.setIdleCallback(
        function() {
          // Verify that the done callback was called.
          assertEquals(1, updates[importer.TaskQueue.UpdateType.COMPLETE]);
          resolve();
        });
  });

  reportPromise(whenDone, callback);
}

// Verifies that the update callback is called to report task errors.
function testErrorUpdate(callback) {
  var task = new TestTask('task0');

  // Get the task to report an error when it's run.
  task.whenRun().then(
      function(task) {
        task.notifyError();
        // Errors are not terminal; still need to signal task completion
        // otherwise the test hangs.
        task.notifyComplete();
      });

  queue.queueTask(task);

  var whenDone = new Promise(function(resolve) {
    queue.setIdleCallback(
        function() {
          // Verify that the done callback was called.
          assertEquals(1, updates[importer.TaskQueue.UpdateType.ERROR]);
          resolve();
        });
  });

  reportPromise(whenDone, callback);
}

function testOnTaskCancelled(callback) {
  var task0 = new TestTask('task0');
  var task1 = new TestTask('task1');

  // Make the tasks call Task#notifyComplete when they are run.
  task0.whenRun().then(function(task) {task.notifyCanceled(); });
  task1.whenRun().then(function(task) {task.notifyComplete(); });

  // Enqueue both tasks, and then verify that they were run.
  queue.queueTask(task0);
  queue.queueTask(task1);
  reportPromise(
      Promise.all([task0.whenRun(), task1.whenRun()]),
      callback);
}
