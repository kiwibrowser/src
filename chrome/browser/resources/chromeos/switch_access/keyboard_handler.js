// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Class to handle keyboard input.
 *
 * @constructor
 * @param {SwitchAccessInterface} switchAccess
 */
function KeyboardHandler(switchAccess) {
  /**
   * SwitchAccess reference.
   *
   * @private {SwitchAccessInterface}
   */
  this.switchAccess_ = switchAccess;

  this.init_();
}

KeyboardHandler.prototype = {
  /**
   * Set up key listener.
   *
   * @private
   */
  init_: function() {
    this.updateSwitchAccessKeys();
    document.addEventListener('keyup', this.handleKeyEvent_.bind(this));
  },

  /**
   * Update the keyboard keys captured by Switch Access to those stored in
   * prefs.
   */
  updateSwitchAccessKeys: function() {
    let keyCodes = [];
    for (let command of this.switchAccess_.getCommands()) {
      let keyCode = this.keyCodeFor_(command);
      if ((keyCode >= '0'.charCodeAt(0) && keyCode <= '9'.charCodeAt(0)) ||
          (keyCode >= 'A'.charCodeAt(0) && keyCode <= 'Z'.charCodeAt(0)))
        keyCodes.push(keyCode);
    }
    chrome.accessibilityPrivate.setSwitchAccessKeys(keyCodes);
  },

  /**
   * Return the key code that |command| maps to.
   *
   * @param {string} command
   * @return {number}
   */
  keyCodeFor_: function(command) {
    return this.switchAccess_.getNumberPref(command);
  },

  /**
   * Run the command associated with the passed keyboard event.
   *
   * @param {!Event} event
   * @private
   */
  handleKeyEvent_: function(event) {
    for (let command of this.switchAccess_.getCommands()) {
      if (this.keyCodeFor_(command) === event.keyCode) {
        let key = event.key.toUpperCase();
        console.log('\'' + key + '\' pressed for command: ' + command);
        this.switchAccess_.runCommand(command);
        this.switchAccess_.performedUserAction();
        return;
      }
    }
  },
};
