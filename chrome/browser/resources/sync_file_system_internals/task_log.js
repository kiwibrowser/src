// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var TaskLog = (function() {
  'use strict';

  var nextTaskLogSeq = 1;
  var TaskLog = {};

  function observeTaskLog() {
    chrome.send('observeTaskLog');
  }

  /**
   * Handles per-task log event.
   * @param {Object} taskLog a dictionary containing 'duration',
   * 'task_description', 'result_description' and 'details'.
   */
  TaskLog.onTaskLogRecorded = function(taskLog) {
    var details = document.createElement('td');
    details.classList.add('task-log-details');

    var label = document.createElement('label');
    details.appendChild(label);

    var collapseCheck = document.createElement('input');
    collapseCheck.setAttribute('type', 'checkbox');
    collapseCheck.classList.add('task-log-collapse-check');
    label.appendChild(collapseCheck);

    var ul = document.createElement('ul');
    for (var i = 0; i < taskLog.details.length; ++i)
      ul.appendChild(createElementFromText('li', taskLog.details[i]));
    label.appendChild(ul);

    var tr = document.createElement('tr');
    tr.appendChild(createElementFromText(
        'td', taskLog.duration, {'class': 'task-log-duration'}));
    tr.appendChild(createElementFromText(
        'td', taskLog.task_description, {'class': 'task-log-description'}));
    tr.appendChild(createElementFromText(
        'td', taskLog.result_description, {'class': 'task-log-result'}));
    tr.appendChild(details);

    $('task-log-entries').appendChild(tr);
  };

  /**
   * Get initial sync service values and set listeners to get updated values.
   */
  function main() {
    observeTaskLog();
  }

  document.addEventListener('DOMContentLoaded', main);
  return TaskLog;
})();
