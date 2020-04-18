// Copyright 2016 The ChromeOS IME Authors. All Rights Reserved.
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
/**
 * @fileoverview A single point in a gesture typing stroke.
 */
goog.provide('i18n.input.chrome.inputview.elements.content.Point');

goog.scope(function() {



/**
 * This class is used for two purposes: (1) Be rendered as a gesture trail in
 * the UI, and (2) be transmitted to the backend decoder where they are
 * processed into candidate words.
 *
 * @param {number} x The x coordinate.
 * @param {number} y The y coordinate.
 * @param {number} identifier The pointer event identifier.
 * @constructor
 */
i18n.input.chrome.inputview.elements.content.Point =
    function(x, y, identifier) {
  /**
   * The left offset relative to the canvas.
   *
   * @type {number}
   */
  this.x = x;

  /**
   * The top offset relative to the canvas.
   *
   * @type {number}
   */
  this.y = y;

  /**
   * The pointer ID.
   *
   * @type {number}
   */
  this.pointer = 0;

  /**
   * The time-to-live value of the point, used to render the trail fading
   * effect.
   *
   * @type {number}
   */
  this.ttl = Point.STARTING_TTL;

  /**
   * The time this point was created, in ms since epoch.
   *
   * @type {number}
   */
  this.time = Date.now();

  /**
   * The action type of the point.
   *
   * @type {i18n.input.chrome.inputview.elements.content.Point.Action}
   */
  this.action = i18n.input.chrome.inputview.elements.content.Point.Action.MOVE;

  /**
   * The pointer event identifier associated with this point.
   *
   * @type {number}
   */
  this.identifier = identifier;
};
var Point =
    i18n.input.chrome.inputview.elements.content.Point;


/**
 * Starting time-to-live value.
 *
 * @const {number}
 */
Point.STARTING_TTL = 255;


/**
 * Enum describing the type of action for a given point.
 *
 * @enum {number}
 */
Point.Action = {
  DOWN: 0,
  UP: 1,
  MOVE: 2
};
});  // goog.scope
