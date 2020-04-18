// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Class that manages the addition and removal of WebUI
 * listeners.
 */

/**
 * Create a WebUIListenerTracker to track a set of Web UI listeners.
 * @constructor
 */
function WebUIListenerTracker() {
  /**
   * The Web UI listeners being tracked.
   * @private {!Array<!WebUIListener>}
   */
  this.listeners_ = [];
}

WebUIListenerTracker.prototype = {
  /**
   * Adds a WebUI listener to the array of listeners being tracked, which
   * will be removed when removeAll() is called.
   * Do not use this method if the listener will be removed manually.
   * @param {string} eventName The event to listen to.
   * @param {!Function} callback The callback to run when the event is fired.
   */
  add: function(eventName, callback) {
    this.listeners_.push(cr.addWebUIListener(eventName, callback));
  },

  /**
   * Removes all Web UI listeners that are currently being tracked.
   */
  removeAll: function() {
    while (this.listeners_.length > 0) {
      cr.removeWebUIListener(this.listeners_.pop());
    }
  },
};
