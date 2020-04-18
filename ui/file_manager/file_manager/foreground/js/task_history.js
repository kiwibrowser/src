// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * TaskHistory object keeps track of the histry of task executions.
 * This is responsible for keeping the history in persistent storage, too.
 * @extends {cr.EventTarget}
 * @constructor
 * @struct
 */
function TaskHistory() {
  cr.EventTarget.call(this);

  /**
   * The recent history of task executions. Key is task ID and value is time
   * stamp of the latest execution of the task.
   * @type {!Object<string, number>}
   */
  this.lastExecutedTime_ = {};

  chrome.storage.onChanged.addListener(this.onLocalStorageChanged_.bind(this));
  this.load_();
};

/**
 * @enum {string}
 */
TaskHistory.EventType = {
  UPDATE: 'update'
};

/**
 * This key is used to store the history in chrome's local storage.
 * @const {string}
 */
TaskHistory.STORAGE_KEY_LAST_EXECUTED_TIME = 'task-last-executed-time';

/**
 * @const {number}
 */
TaskHistory.LAST_EXECUTED_TIME_HISTORY_MAX = 100;


TaskHistory.prototype = {
  __proto__: cr.EventTarget.prototype,

  /**
   * Records the timing of task execution.
   * @param {string} taskId
   */
  recordTaskExecuted: function(taskId) {
    this.lastExecutedTime_[taskId] = Date.now();
    this.truncate_();
    this.save_();
  },

  /**
   * Gets the time stamp of last execution of given task. If the record is not
   * found, returns 0.
   * @param {string} taskId
   * @return {number}
   */
  getLastExecutedTime: function(taskId) {
    return this.lastExecutedTime_[taskId] ? this.lastExecutedTime_[taskId] : 0;
  },

  /**
   * Loads current history from local storage.
   * @private
   */
  load_: function() {
    chrome.storage.local.get(
        TaskHistory.STORAGE_KEY_LAST_EXECUTED_TIME, function(value) {
          this.lastExecutedTime_ =
              value[TaskHistory.STORAGE_KEY_LAST_EXECUTED_TIME] || {};
        }.bind(this));
  },

  /**
   * Saves current history to local storage.
   * @private
   */
  save_: function() {
    var objectToSave = {};
    objectToSave[TaskHistory.STORAGE_KEY_LAST_EXECUTED_TIME] =
        this.lastExecutedTime_;
    chrome.storage.local.set(objectToSave);
  },

  /**
   * Handles change event on storage to update current history.
   * @param {!Object<string, !StorageChange>} changes
   * @param {string} areaName
   * @private
   */
  onLocalStorageChanged_: function(changes, areaName) {
    if (areaName != 'local')
      return;

    for (var key in changes) {
      if (key == TaskHistory.STORAGE_KEY_LAST_EXECUTED_TIME) {
        this.lastExecutedTime_ = changes[key].newValue;
        cr.dispatchSimpleEvent(this, TaskHistory.EventType.UPDATE);
      }
    }
  },

  /**
   * Trancates current history so that the size of history does not exceed
   * STORAGE_KEY_LAST_EXECUTED_TIME.
   * @private
   */
  truncate_: function() {
    var keys = Object.keys(this.lastExecutedTime_);
    if (keys.length <= TaskHistory.LAST_EXECUTED_TIME_HISTORY_MAX)
      return;

    var items = [];
    for (var i = 0; i < keys.length; i++)
      items.push({id: keys[i], timestamp: this.lastExecutedTime_[keys[i]]});

    items.sort((a, b) => b.timestamp - a.timestamp);
    items = items.slice(0, TaskHistory.LAST_EXECUTED_TIME_HISTORY_MAX);

    var newObject = {};
    for (var i = 0; i < items.length; i++)
      newObject[items[i].id] = items[i].timestamp;

    this.lastExecutedTime_ = newObject;
  }
};
