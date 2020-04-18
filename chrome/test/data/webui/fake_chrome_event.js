// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Fake implementations of ChromeEvent.
 */

/**
 * @constructor
 * @extends {ChromeEvent}
 */
function FakeChromeEvent() {
  /** @type {!Set<!Function>} */
  this.listeners_ = new Set();
}

FakeChromeEvent.prototype = {
  /** @param {Function} listener */
  addListener: function(listener) {
    assertFalse(
        this.listeners_.has(listener),
        'FakeChromeEvent.addListened: Listener already added');
    this.listeners_.add(listener);
  },

  /** @param {Function} listener */
  removeListener: function(listener) {
    assertTrue(
        this.listeners_.has(listener),
        'FakeChromeEvent.removeListener: Listener does not exist');
    this.listeners_.delete(listener);
  },

  /** @param {...} args */
  callListeners: function(...var_args) {
    this.listeners_.forEach(function(l) {
      l.apply(null, var_args);
    });
  }
};
