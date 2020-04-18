// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Class to run and get details about user commands.
 *
 * @constructor
 * @param {SwitchAccessInterface} switchAccess
 */
function Commands(switchAccess) {
  /**
   * SwitchAccess reference.
   *
   * @private {SwitchAccessInterface}
   */
  this.switchAccess_ = switchAccess;

  /**
   * A map from command name to the default key code and function binding for
   * the command.
   *
   * @private {!Object}
   */
  this.commandMap_ = this.buildCommandMap_();
}

Commands.prototype = {

  /**
   * Return a list of the names of all user commands.
   *
   * @return {!Array<string>}
   */
  getCommands: function() {
    return Object.keys(this.commandMap_);
  },

  /**
   * Return the default key code for a command.
   *
   * @param {string} command
   * @return {number}
   */
  getDefaultKeyCodeFor: function(command) {
    return this.commandMap_[command]['defaultKeyCode'];
  },

  /**
   * Run the function binding for the specified command.
   *
   * @param {string} command
   */
  runCommand: function(command) {
    this.commandMap_[command]['binding']();
  },

  /**
   * Build the object that maps from command name to the default key code and
   * function binding for the command.
   *
   * @return {!Object}
   */
  buildCommandMap_: function() {
    return {
      'next': {
        'defaultKeyCode': 49, /* '1' key */
        'binding': this.switchAccess_.moveToNode.bind(this.switchAccess_, true)
      },
      'previous': {
        'defaultKeyCode': 50, /* '2' key */
        'binding': this.switchAccess_.moveToNode.bind(this.switchAccess_, false)
      },
      'select': {
        'defaultKeyCode': 51, /* '3' key */
        'binding': this.switchAccess_.selectCurrentNode.bind(this.switchAccess_)
      },
      'options': {
        'defaultKeyCode': 52, /* '4' key */
        'binding': this.switchAccess_.showOptionsPage.bind(this.switchAccess_)
      },
      'debugNext': {
        'defaultKeyCode': -1, /* unused key */
        'binding': this.switchAccess_.debugMoveToNext.bind(this.switchAccess_)
      },
      'debugPrevious': {
        'defaultKeyCode': -1, /* unused key */
        'binding':
            this.switchAccess_.debugMoveToPrevious.bind(this.switchAccess_)
      },
      'debugChild': {
        'defaultKeyCode': -1, /* unused key */
        'binding':
            this.switchAccess_.debugMoveToFirstChild.bind(this.switchAccess_)
      },
      'debugParent': {
        'defaultKeyCode': -1, /* unused key */
        'binding': this.switchAccess_.debugMoveToParent.bind(this.switchAccess_)
      }
    };
  }
};
