// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Class to manage SwitchAccess and interact with other controllers.
 *
 * @constructor
 * @implements {SwitchAccessInterface}
 */
function SwitchAccess() {
  console.log('Switch access is enabled');

  /**
   * User commands.
   *
   * @private {Commands}
   */
  this.commands_ = null;

  /**
   * User preferences.
   *
   * @private {SwitchAccessPrefs}
   */
  this.switchAccessPrefs_ = null;

  /**
   * Handles changes to auto-scan.
   *
   * @private {AutoScanManager}
   */
  this.autoScanManager_ = null;

  /**
   * Handles keyboard input.
   *
   * @private {KeyboardHandler}
   */
  this.keyboardHandler_ = null;

  /**
   * Handles interactions with the accessibility tree, including moving to and
   * selecting nodes.
   *
   * @private {AutomationManager}
   */
  this.automationManager_ = null;

  this.init_();
}

SwitchAccess.prototype = {
  /**
   * Set up preferences, controllers, and event listeners.
   *
   * @private
   */
  init_: function() {
    this.commands_ = new Commands(this);
    this.switchAccessPrefs_ = new SwitchAccessPrefs(this);
    this.autoScanManager_ = new AutoScanManager(this);
    this.keyboardHandler_ = new KeyboardHandler(this);

    chrome.automation.getDesktop(function(desktop) {
      this.automationManager_ = new AutomationManager(desktop);
    }.bind(this));

    document.addEventListener(
        'prefsUpdate', this.handlePrefsUpdate_.bind(this));
  },

  /**
   * Move to the next/previous interesting node. If |doNext| is true, move to
   * the next node. Otherwise, move to the previous node.
   *
   * @param {boolean} doNext
   * @override
   */
  moveToNode: function(doNext) {
    if (this.automationManager_)
      this.automationManager_.moveToNode(doNext);
  },

  /**
   * Perform the default action on the current node.
   *
   * @override
   */
  selectCurrentNode: function() {
    if (this.automationManager_)
      this.automationManager_.selectCurrentNode();
  },

  /**
   * Open the options page in a new tab.
   *
   * @override
   */
  showOptionsPage: function() {
    let optionsPage = {url: 'options.html'};
    chrome.tabs.create(optionsPage);
  },

  /**
   * Return a list of the names of all user commands.
   *
   * @override
   * @return {!Array<string>}
   */
  getCommands: function() {
    return this.commands_.getCommands();
  },

  /**
   * Return the default key code for a command.
   *
   * @override
   * @param {string} command
   * @return {number}
   */
  getDefaultKeyCodeFor: function(command) {
    return this.commands_.getDefaultKeyCodeFor(command);
  },

  /**
   * Run the function binding for the specified command.
   *
   * @override
   * @param {string} command
   */
  runCommand: function(command) {
    this.commands_.runCommand(command);
  },

  /**
   * Perform actions as the result of actions by the user. Currently, restarts
   * auto-scan if it is enabled.
   *
   * @override
   */
  performedUserAction: function() {
    this.autoScanManager_.restartIfRunning();
  },

  /**
   * Handle a change in user preferences.
   *
   * @param {!Event} event
   * @private
   */
  handlePrefsUpdate_: function(event) {
    let updatedPrefs = event.detail;
    for (let key of Object.keys(updatedPrefs)) {
      switch (key) {
        case 'enableAutoScan':
          this.autoScanManager_.setEnabled(updatedPrefs[key]);
          break;
        case 'autoScanTime':
          this.autoScanManager_.setScanTime(updatedPrefs[key]);
          break;
        default:
          if (this.commands_.getCommands().includes(key))
            this.keyboardHandler_.updateSwitchAccessKeys();
      }
    }
  },

  /**
   * Set the value of the preference |key| to |value| in chrome.storage.sync.
   * this.prefs_ is not set until handleStorageChange_.
   *
   * @override
   * @param {string} key
   * @param {boolean|string|number} value
   */
  setPref: function(key, value) {
    this.switchAccessPrefs_.setPref(key, value);
  },

  /**
   * Get the value of type 'boolean' of the preference |key|. Will throw a type
   * error if the value of |key| is not 'boolean'.
   *
   * @override
   * @param  {string} key
   * @return {boolean}
   */
  getBooleanPref: function(key) {
    return this.switchAccessPrefs_.getBooleanPref(key);
  },

  /**
   * Get the value of type 'number' of the preference |key|. Will throw a type
   * error if the value of |key| is not 'number'.
   *
   * @override
   * @param  {string} key
   * @return {number}
   */
  getNumberPref: function(key) {
    return this.switchAccessPrefs_.getNumberPref(key);
  },

  /**
   * Get the value of type 'string' of the preference |key|. Will throw a type
   * error if the value of |key| is not 'string'.
   *
   * @override
   * @param  {string} key
   * @return {string}
   */
  getStringPref: function(key) {
    return this.switchAccessPrefs_.getStringPref(key);
  },

  /**
   * Returns true if |keyCode| is already used to run a command from the
   * keyboard.
   *
   * @override
   * @param {number} keyCode
   * @return {boolean}
   */
  keyCodeIsUsed: function(keyCode) {
    return this.switchAccessPrefs_.keyCodeIsUsed(keyCode);
  },

  /**
   * Move to the next sibling of the current node if it has one.
   *
   * @override
   */
  debugMoveToNext: function() {
    if (this.automationManager_)
      this.automationManager_.debugMoveToNext();
  },

  /**
   * Move to the previous sibling of the current node if it has one.
   *
   * @override
   */
  debugMoveToPrevious: function() {
    if (this.automationManager_)
      this.automationManager_.debugMoveToPrevious();
  },

  /**
   * Move to the first child of the current node if it has one.
   *
   * @override
   */
  debugMoveToFirstChild: function() {
    if (this.automationManager_)
      this.automationManager_.debugMoveToFirstChild();
  },

  /**
   * Move to the parent of the current node if it has one.
   *
   * @override
   */
  debugMoveToParent: function() {
    if (this.automationManager_)
      this.automationManager_.debugMoveToParent();
  }
};
