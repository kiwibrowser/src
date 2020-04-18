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
goog.provide('i18n.input.chrome.inputview.PointerConfig');


/**
 * The configuration of the pointer.
 *
 * @param {boolean} dblClick .
 * @param {boolean} longPressWithPointerUp .
 * @param {boolean} longPressWithoutPointerUp .
 * @constructor
 */
i18n.input.chrome.inputview.PointerConfig = function(dblClick,
    longPressWithPointerUp, longPressWithoutPointerUp) {
  /**
   * True to enable double click.
   *
   * @type {boolean}
   */
  this.dblClick = dblClick;

  /**
   * The delay of the double click. If not set or is 0, the default delay(500ms)
   * is used.
   *
   * @type {number}
   */
  this.dblClickDelay = 0;

  /**
   * True to enable long press and not cancel the next pointer up.
   *
   * @type {boolean}
   */
  this.longPressWithPointerUp = longPressWithPointerUp;

  /**
   * True to enable long press and cancel the next pointer up.
   *
   * @type {boolean}
   */
  this.longPressWithoutPointerUp = longPressWithoutPointerUp;

  /**
   * The flicker direction.
   *
   * @type {number}
   */
  this.flickerDirection = 0;

  /**
   * The delay of the long press.
   *
   * @type {number}
   */
  this.longPressDelay = 0;

  /** @type {boolean} */
  this.stopEventPropagation = true;

  /** @type {boolean} */
  this.preventDefault = true;
};

