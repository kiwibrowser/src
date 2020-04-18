// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * Logging info for benchmarking purposes. Should be the first js file included.
 */

/* Stack of events that has been logged. */
var eventLog = [];

/**
 * Logs an event.
 * @param {string} name The name of the event (can be any string).
 * @param {boolean=} opt_shouldLogTime If true, the event is used for
 *     benchmarking and the time is logged. Otherwise, just push the event on
 *     the event stack.
 */
function logEvent(name, opt_shouldLogTime) {
  if (opt_shouldLogTime)
    chrome.send('metricsHandler:logEventTime', [name]);
  eventLog.push([name, Date.now()]);
}

logEvent('Tab.NewTabScriptStart', true);
window.addEventListener('load', function(e) {
  logEvent('Tab.NewTabOnload', true);
});
document.addEventListener('DOMContentLoaded', function(e) {
  logEvent('Tab.NewTabDOMContentLoaded', true);
});
