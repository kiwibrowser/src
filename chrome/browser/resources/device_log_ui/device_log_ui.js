// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var DeviceLogUI = (function() {
  'use strict';

  /**
   * Creates a tag for the log level.
   *
   * @param {string} level A string that represents log level.
   * @return {HTMLSpanElement} The created span element.
   */
  var createLevelTag = function(level) {
    var levelClassName = 'log-level-' + level.toLowerCase();
    var tag = document.createElement('span');
    tag.textContent = level;
    tag.className = 'level-tag ' + levelClassName;
    return tag;
  };

  /**
   * Creates a tag for the log type.
   *
   * @param {string} level A string that represents log type.
   * @return {HTMLSpanElement} The created span element.
   */
  var createTypeTag = function(type) {
    var typeClassName = 'log-type-' + type.toLowerCase();
    var tag = document.createElement('span');
    tag.textContent = type;
    tag.className = 'type-tag ' + typeClassName;
    return tag;
  };

  /**
   * Creates an element that contains the time, the event, the level and
   * the description of the given log entry.
   *
   * @param {Object} logEntry An object that represents a single line of log.
   * @return {?HTMLParagraphElement} The created p element that represents
   *     the log entry, or null if the entry should be skipped.
   */
  var createLogEntryText = function(logEntry) {
    var level = logEntry['level'];
    var levelCheckbox = 'log-level-' + level.toLowerCase();
    if ($(levelCheckbox) && !$(levelCheckbox).checked)
      return null;

    var type = logEntry['type'];
    var typeCheckbox = 'log-type-' + type.toLowerCase();
    if ($(typeCheckbox) && !$(typeCheckbox).checked)
      return null;

    var res = document.createElement('p');
    var textWrapper = document.createElement('span');
    var fileinfo = '';
    if ($('log-fileinfo').checked)
      fileinfo = logEntry['file'];
    var timestamp = '';
    if ($('log-timedetail').checked)
      timestamp = logEntry['timestamp'];
    else
      timestamp = logEntry['timestampshort'];
    textWrapper.textContent = loadTimeData.getStringF(
        'logEntryFormat', timestamp, fileinfo, logEntry['event']);
    res.appendChild(createTypeTag(type));
    res.appendChild(createLevelTag(level));
    res.appendChild(textWrapper);
    return res;
  };

  /**
   * Creates event log entries.
   *
   * @param {Array<string>} logEntries An array of strings that represent log
   *     log events in JSON format.
   */
  var createEventLog = function(logEntries) {
    var container = $('log-container');
    container.textContent = '';
    for (var i = 0; i < logEntries.length; ++i) {
      var entry = createLogEntryText(JSON.parse(logEntries[i]));
      if (entry)
        container.appendChild(entry);
    }
  };

  /**
   * Callback function, triggered when the log is received.
   *
   * @param {Object} data A JSON structure of event log entries.
   */
  var getLogCallback = function(data) {
    try {
      createEventLog(JSON.parse(data));
    } catch (e) {
      var container = $('log-container');
      container.textContent = 'No log entries';
    }
  };

  /**
   * Requests a log update.
   */
  var requestLog = function() {
    chrome.send('DeviceLog.getLog');
  };

  /**
   * Sets refresh rate if the interval is found in the url.
   */
  var setRefresh = function() {
    var interval = parseQueryParams(window.location)['refresh'];
    if (interval && interval != '')
      setInterval(requestLog, parseInt(interval) * 1000);
  };

  /**
   * Gets log information from WebUI.
   */
  document.addEventListener('DOMContentLoaded', function() {
    // Show all levels except 'debug' by default.
    $('log-level-error').checked = true;
    $('log-level-user').checked = true;
    $('log-level-event').checked = true;
    $('log-level-debug').checked = false;

    // Show all types by default.
    var checkboxes = document.querySelectorAll(
        '#log-checkbox-container input[type="checkbox"][id*="log-type"]');
    for (var i = 0; i < checkboxes.length; ++i)
      checkboxes[i].checked = true;

    $('log-fileinfo').checked = false;
    $('log-timedetail').checked = false;

    $('log-refresh').onclick = requestLog;
    checkboxes = document.querySelectorAll(
        '#log-checkbox-container input[type="checkbox"]');
    for (var i = 0; i < checkboxes.length; ++i)
      checkboxes[i].onclick = requestLog;

    setRefresh();
    requestLog();
  });

  return {getLogCallback: getLogCallback};
})();
