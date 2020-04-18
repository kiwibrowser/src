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
goog.provide('i18n.input.chrome.inputview.ReadyState');



goog.scope(function() {



/**
 * The system ready state which mainains a state bit map.
 * Inputview controller uses this to determine whether the system is ready to
 * render the UI.
 *
 * @constructor
 */
i18n.input.chrome.inputview.ReadyState = function() {
};
var ReadyState = i18n.input.chrome.inputview.ReadyState;


/**
 * The state type.
 *
 * @enum {number}
 */
ReadyState.State = {
  IME_LIST_READY: 0x1,
  KEYBOARD_CONFIG_READY: 0x10,
  LAYOUT_READY: 0x100,
  LAYOUT_CONFIG_READY: 0x1000,
  M17N_LAYOUT_READY: 0x10000,
  DISPLAY_SIZE_READY: 0x100000
};


/**
 * The internal ready state bit map.
 *
 * @private {number}
 */
ReadyState.prototype.state_ = 0;


/**
 * Gets whether a specific state type is ready.
 *
 * @param {ReadyState.State} state .
 * @return {boolean} Whether is ready.
 */
ReadyState.prototype.isReady = function(state) {
  return !!(this.state_ & state);
};


/**
 * Sets state ready for the given state type.
 *
 * @param {ReadyState.State} state .
 */
ReadyState.prototype.markStateReady = function(state) {
  this.state_ |= state;
};
});  // goog.scope
