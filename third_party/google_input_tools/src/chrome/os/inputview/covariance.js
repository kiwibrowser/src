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
goog.provide('i18n.input.chrome.inputview.Covariance');

goog.require('goog.object');
goog.require('i18n.input.chrome.ElementType');


goog.scope(function() {
var ElementType = i18n.input.chrome.ElementType;



/**
 * The covariance used for gaussian model.
 *
 * @constructor
 */
i18n.input.chrome.inputview.Covariance = function() {
  /** @private {number} */
  this.breakDown_ = 0;
};
var Covariance = i18n.input.chrome.inputview.Covariance;


/**
 * The break-down for covariance.
 *
 * @enum {number}
 */
Covariance.BreakDown = {
  A11Y: 1,
  HORIZONTAL: 2,
  WIDE_SCREEN: 4
};


/**
 * The key type.
 *
 * @type {!Object.<ElementType, number>}
 */
Covariance.ElementTypeMap = goog.object.create(
    ElementType.CHARACTER_KEY, 0,
    ElementType.COMPACT_KEY, 1
    );


/**
 * The value.
 * Key: the break down value.
 * Value: A list - first is the covariance for full keyboard, second is for
 * compact.
 *
 * @private {!Object.<!Array.<number>>}
 */
Covariance.VALUE_ = {
  0: [120, 160],
  1: [130, 0],
  2: [235, 342],
  3: [162, 0],
  4: [160, 213],
  5: [142, 0],
  6: [230, 332],
  7: [162, 0]
};


/**
 * Updates the covariance.
 *
 * @param {boolean} isWideScreen .
 * @param {boolean} isHorizontal .
 * @param {boolean} isA11y .
 */
Covariance.prototype.update = function(isWideScreen, isHorizontal, isA11y) {
  this.breakDown_ = 0;
  if (isWideScreen) {
    this.breakDown_ |= Covariance.BreakDown.WIDE_SCREEN;
  }
  if (isHorizontal) {
    this.breakDown_ |= Covariance.BreakDown.HORIZONTAL;
  }
  if (isA11y) {
    this.breakDown_ |= Covariance.BreakDown.A11Y;
  }
};


/**
 * Gets the covariance value.
 *
 * @param {ElementType} type .
 * @return {number} The value.
 */
Covariance.prototype.getValue = function(type) {
  var index = Covariance.ElementTypeMap[type];
  return Covariance.VALUE_[this.breakDown_][index];
};

});  // goog.scope

