// Copyright 2014 The ChromeOS IME Authors. All Rights Reserved.
// limitations under the License.
// See the License for the specific language governing permissions and
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// distributed under the License is distributed on an "AS-IS" BASIS,
// Unless required by applicable law or agreed to in writing, software
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// You may obtain a copy of the License at
// you may not use this file except in compliance with the License.
// Licensed under the Apache License, Version 2.0 (the "License");
//
goog.provide('i18n.input.chrome.inputview.StateManager');

goog.require('i18n.input.chrome.inputview.Covariance');

goog.scope(function() {



/**
 * The state for the input view keyboard.
 *
 * @constructor
 */
i18n.input.chrome.inputview.StateManager = function() {
  /** @type {!i18n.input.chrome.inputview.Covariance} */
  this.covariance = new i18n.input.chrome.inputview.Covariance();
};
var StateManager = i18n.input.chrome.inputview.StateManager;


/** @type {string} */
StateManager.prototype.contextType = '';


/**
 * The state of the keyboard.
 *
 * @type {number}
 * @private
 */
StateManager.prototype.state_ = 0;


/**
 * The sticky state.
 *
 * @type {number}
 * @private
 */
StateManager.prototype.sticky_ = 0;


/**
 * Bits to indicate which state key is down.
 *
 * @type {number}
 * @private
 */
StateManager.prototype.stateKeyDown_ = 0;


/**
 * Bits to track which state is in chording.
 *
 * @type {number}
 * @private
 */
StateManager.prototype.chording_ = 0;


/**
 * A flag to temporary compatible with the current modifier key state
 * managerment, this bit indicates the sticky is set by double_click or long
 * press which won't be canceled by keyup or text commit.
 *
 * @private {number}
 */
StateManager.prototype.finalSticky_ = 0;


/**
 * Whether the current keyset is in English mode.
 *
 * @type {boolean}
 */
StateManager.prototype.isEnMode = false;


/**
 * Sets a state to keydown.
 *
 * @param {!i18n.input.chrome.inputview.StateType} stateType The state type.
 * @param {boolean} isKeyDown True if the state key is down.
 */
i18n.input.chrome.inputview.StateManager.prototype.setKeyDown = function(
    stateType, isKeyDown) {
  if (isKeyDown) {
    this.stateKeyDown_ |= stateType;
  } else {
    this.stateKeyDown_ &= ~stateType;
    this.chording_ &= ~stateType;
  }
};


/**
 * True if the key is down.
 *
 * @param {!i18n.input.chrome.inputview.StateType} stateType .
 * @return {boolean} .
 */
i18n.input.chrome.inputview.StateManager.prototype.isKeyDown = function(
    stateType) {
  return (this.stateKeyDown_ & stateType) != 0;
};


/**
 * Triggers chording and record it for each key-downed state.
 */
i18n.input.chrome.inputview.StateManager.prototype.triggerChording =
    function() {
  this.chording_ |= this.stateKeyDown_;
};


/**
 * True if the state is chording now.
 *
 * @param {!i18n.input.chrome.inputview.StateType} stateType The state type.
 * @return {boolean} True if the state is chording.
 */
i18n.input.chrome.inputview.StateManager.prototype.isChording = function(
    stateType) {
  return (this.chording_ & stateType) != 0;
};


/**
 * Is a state in final sticky.
 *
 * @param {i18n.input.chrome.inputview.StateType} stateType .
 * @return {boolean} .
 */
i18n.input.chrome.inputview.StateManager.prototype.isFinalSticky = function(
    stateType) {
  return (this.finalSticky_ & stateType) != 0;
};


/**
 * Sets a specific state to be final sticky.
 *
 * @param {i18n.input.chrome.inputview.StateType} stateType .
 * @param {boolean} isFinalSticky .
 */
i18n.input.chrome.inputview.StateManager.prototype.setFinalSticky = function(
    stateType, isFinalSticky) {
  if (isFinalSticky) {
    this.finalSticky_ |= stateType;
  } else {
    this.finalSticky_ &= ~stateType;
  }
};


/**
 * Sets a state to be sticky.
 *
 * @param {!i18n.input.chrome.inputview.StateType} stateType The state type.
 * @param {boolean} isSticky True to set it sticky.
 */
i18n.input.chrome.inputview.StateManager.prototype.setSticky = function(
    stateType, isSticky) {
  if (isSticky) {
    this.sticky_ |= stateType;
  } else {
    this.sticky_ &= ~stateType;
  }
};


/**
 * Is a state sticky.
 *
 * @param {!i18n.input.chrome.inputview.StateType} stateType The state
 *     type.
 * @return {boolean} True if it is sticky.
 */
i18n.input.chrome.inputview.StateManager.prototype.isSticky = function(
    stateType) {
  return (this.sticky_ & stateType) != 0;
};


/**
 * Sets a state.
 *
 * @param {!i18n.input.chrome.inputview.StateType} stateType The state
 *     type.
 * @param {boolean} enable True to enable the state.
 */
i18n.input.chrome.inputview.StateManager.prototype.setState = function(
    stateType, enable) {
  if (enable) {
    this.state_ = this.state_ | stateType;
  } else {
    this.state_ = this.state_ & ~stateType;
  }
};


/**
 * Toggle the state.
 *
 * @param {!i18n.input.chrome.inputview.StateType} stateType The state
 *     type.
 * @param {boolean} isSticky True to set it sticky.
 */
i18n.input.chrome.inputview.StateManager.prototype.toggleState = function(
    stateType, isSticky) {
  var enable = !this.hasState(stateType);
  this.setState(stateType, enable);
  isSticky = enable ? isSticky : false;
  this.setSticky(stateType, isSticky);
};


/**
 * Is the state on.
 *
 * @param {!i18n.input.chrome.inputview.StateType} stateType The state
 *     type.
 * @return {boolean} True if the state is on.
 */
i18n.input.chrome.inputview.StateManager.prototype.hasState = function(
    stateType) {
  return (this.state_ & stateType) != 0;
};


/**
 * Gets the state.
 *
 * @return {number} The state.
 */
i18n.input.chrome.inputview.StateManager.prototype.getState =
    function() {
  return this.state_;
};


/**
 * Clears unsticky state.
 *
 */
i18n.input.chrome.inputview.StateManager.prototype.clearUnstickyState =
    function() {
  this.state_ = this.state_ & this.sticky_;
};


/**
 * True if there is unsticky state.
 *
 * @return {boolean} True if there is unsticky state.
 */
i18n.input.chrome.inputview.StateManager.prototype.hasUnStickyState =
    function() {
  return this.state_ != this.sticky_;
};


/**
 * Resets the state.
 *
 */
i18n.input.chrome.inputview.StateManager.prototype.reset = function() {
  this.state_ = 0;
  this.sticky_ = 0;
};

});  // goog.scope
