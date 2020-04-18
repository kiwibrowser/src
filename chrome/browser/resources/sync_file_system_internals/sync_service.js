// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * WebUI to monitor the Sync File System Service.
 */
var SyncService = (function() {
  'use strict';

  var SyncService = {};

  /**
   * Request Sync Service Status.
   */
  function getServiceStatus() {
    chrome.send('getServiceStatus');
  }

  /**
   * Called when service status is initially retrieved or updated via events.
   * @param {string} Service status enum as a string.
   */
  SyncService.onGetServiceStatus = function(statusString) {
    $('service-status').textContent = statusString;
  };

  /**
   * Request Google Drive Notification Source. e.g. XMPP or polling.
   */
  function getNotificationSource() {
    chrome.send('getNotificationSource');
  }

  /**
   * Handles callback from getNotificationSource.
   * @param {string} Notification source as a string.
   */
  SyncService.onGetNotificationSource = function(sourceString) {
    $('notification-source').textContent = sourceString;
  };

  // Keeps track of the last log event seen so it's not reprinted.
  var lastLogEventId = -1;

  /**
   * Request debug log.
   */
  function getLog() {
    chrome.send('getLog', [lastLogEventId]);
  }

  /**
   * Clear old logs.
   */
  function clearLogs() {
    chrome.send('clearLogs');
    $('log-entries').innerHTML = '';
  }

  /**
   * Handles callback from getUpdateLog.
   * @param {Array} list List of dictionaries containing 'id', 'time', 'logEvent'.
   */
  SyncService.onGetLog = function(logEntries) {
    var itemContainer = $('log-entries');
    for (var i = 0; i < logEntries.length; i++) {
      var logEntry = logEntries[i];
      var tr = document.createElement('tr');
      var error = /ERROR/.test(logEntry.logEvent) ? ' error' : '';
      tr.appendChild(
          createElementFromText('td', logEntry.time, {'class': 'log-time'}));
      tr.appendChild(createElementFromText(
          'td', logEntry.logEvent, {'class': 'log-event' + error}));
      itemContainer.appendChild(tr);

      lastLogEventId = logEntry.id;
    }
  };

  /**
   * Get initial sync service values and set listeners to get updated values.
   */
  function main() {
    cr.ui.decorate('tabbox', cr.ui.TabBox);
    $('clear-log-button').addEventListener('click', clearLogs);
    getServiceStatus();
    getNotificationSource();

    // TODO: Look for a way to push entries to the page when necessary.
    window.setInterval(getLog, 1000);
  }

  document.addEventListener('DOMContentLoaded', main);
  return SyncService;
})();
