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
goog.provide('i18n.input.chrome.inputview.handler.SwipeState');



/**
 * The state for the swipe action.
 *
 * @constructor
 */
i18n.input.chrome.inputview.handler.SwipeState = function() {
  /**
   * The offset in x-axis.
   *
   * @type {number}
   */
  this.offsetX = 0;

  /**
   * The offset in y-axis.
   *
   * @type {number}
   */
  this.offsetY = 0;

  /**
   * The previous x coordinate.
   *
   * @type {number}
   */
  this.previousX = 0;

  /**
   * The previous y coordinate.
   *
   * @type {number}
   */
  this.previousY = 0;
};


/**
 * Resets the state.
 */
i18n.input.chrome.inputview.handler.SwipeState.prototype.reset =
    function() {
  this.offsetX = this.offsetY = this.previousX = this.previousY = 0;
};

