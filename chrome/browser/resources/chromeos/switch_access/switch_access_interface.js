// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Interface for controllers to interact with main SwitchAccess class.
 *
 * @interface
 */
function SwitchAccessInterface() {}

SwitchAccessInterface.prototype = {
  /**
   * Move to the next/previous interesting node. If |doNext| is true, move to
   * the next node. Otherwise, move to the previous node.
   *
   * @param {boolean} doNext
   */
  moveToNode: function(doNext) {},

  /**
   * Perform the default action on the current node.
   */
  selectCurrentNode: function() {},

  /**
   * Open the options page in a new tab.
   */
  showOptionsPage: function() {},

  /**
   * Return a list of the names of all user commands.
   *
   * @return {!Array<string>}
   */
  getCommands: function() {},

  /**
   * Return the default key code for a command.
   *
   * @param {string} command
   * @return {number}
   */
  getDefaultKeyCodeFor: function(command) {},

  /**
   * Run the function binding for the specified command.
   *
   * @param {string} command
   */
  runCommand: function(command) {},

  /**
   * Perform actions as the result of actions by the user. Currently, restarts
   * auto-scan if it is enabled.
   */
  performedUserAction: function() {},

  /**
   * Set the value of the preference |key| to |value| in chrome.storage.sync.
   * this.prefs_ is not set until handleStorageChange_.
   *
   * @param {string} key
   * @param {boolean|string|number} value
   */
  setPref: function(key, value) {},

  /**
   * Get the value of type 'boolean' of the preference |key|. Will throw a type
   * error if the value of |key| is not 'boolean'.
   *
   * @param  {string} key
   * @return {boolean}
   */
  getBooleanPref: function(key) {},

  /**
   * Get the value of type 'number' of the preference |key|. Will throw a type
   * error if the value of |key| is not 'number'.
   *
   * @param  {string} key
   * @return {number}
   */
  getNumberPref: function(key) {},

  /**
   * Get the value of type 'string' of the preference |key|. Will throw a type
   * error if the value of |key| is not 'string'.
   *
   * @param  {string} key
   * @return {string}
   */
  getStringPref: function(key) {},

  /**
   * Returns true if |keyCode| is already used to run a command from the
   * keyboard.
   *
   * @param {number} keyCode
   * @return {boolean}
   */
  keyCodeIsUsed: function(keyCode) {},

  /**
   * Move to the next sibling of the current node if it has one.
   */
  debugMoveToNext: function() {},

  /**
   * Move to the previous sibling of the current node if it has one.
   */
  debugMoveToPrevious: function() {},

  /**
   * Move to the first child of the current node if it has one.
   */
  debugMoveToFirstChild: function() {},

  /**
   * Move to the parent of the current node if it has one.
   */
  debugMoveToParent: function() {}
};
