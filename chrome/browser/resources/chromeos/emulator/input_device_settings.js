// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'input-device-settings',

  behaviors: [Polymer.NeonAnimatableBehavior],

  /**
   * @param {!Event} e
   * Callback when the user toggles the touchpad.
   */
  onTouchpadChange: function(e) {
    chrome.send('setHasTouchpad', [e.target.checked]);
    this.$.changeDescription.opened = true;
  },

  /**
   * @param {!Event} e
   * Callback when the user toggles the mouse.
   */
  onMouseChange: function(e) {
    chrome.send('setHasMouse', [e.target.checked]);
    this.$.changeDescription.opened = true;
  },

  /**
   * Callback when the existence of a fake mouse changes.
   * @param {boolean} exists
   */
  setMouseExists: function(exists) {
    this.$.mouse.checked = exists;
  },

  /**
   * Callback when the existence of a fake touchpad changes.
   * @param {boolean} exists
   */
  setTouchpadExists: function(exists) {
    this.$.touchpad.checked = exists;
  },
});
