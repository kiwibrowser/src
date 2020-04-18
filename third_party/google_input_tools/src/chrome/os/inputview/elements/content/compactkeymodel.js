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
goog.provide('i18n.input.chrome.inputview.elements.content.CompactKeyModel');

goog.require('i18n.input.chrome.inputview.MoreKeysShiftOperation');
goog.require('i18n.input.chrome.inputview.elements.content.FunctionalKey');


goog.scope(function() {
var MoreKeysShiftOperation = i18n.input.chrome.inputview.MoreKeysShiftOperation;


/**
 * The model of compact key.
 *
 * @param {number} marginLeftPercent The left margin.
 * @param {number} marginRightPercent The right margin.
 * @param {boolean} isGrey Whether it is grey.
 * @param {!Array.<string>} moreKeys The more keys.
 * @param {MoreKeysShiftOperation} moreKeysShiftOperation
 *     The type of opearation when the shift key is down.
 * @param {string} textOnShift The text to display on shift.
 * @param {Object.<string, !Object>} textOnContext Map for changing the key
 *     based on the current input context.
 * @param {string=} opt_textCssClass The css class for the text.
 * @param {string=} opt_title Overrides the displayed text on the key.
 * @param {number=} opt_fixedColumns The fixed number of olumns to display
 *     accent keys.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.content.FunctionalKey}
 */
i18n.input.chrome.inputview.elements.content.CompactKeyModel =
    function(marginLeftPercent, marginRightPercent, isGrey, moreKeys,
        moreKeysShiftOperation, textOnShift, textOnContext,
        opt_textCssClass, opt_title, opt_fixedColumns) {

  /**
   * The left margin.
   *
   * @type {number}
   */
  this.marginLeftPercent = marginLeftPercent || 0;

  /**
   * The right margin.
   *
   * @type {number}
   */
  this.marginRightPercent = marginRightPercent || 0;

  /**
   * True if it is grey.
   *
   * @type {boolean}
   */
  this.isGrey = !!isGrey;

  /**
   * The more keys array.
   *
   * @type {!Array.<string>}
   */
  this.moreKeys = moreKeys || [];

  /**
   * The type of shift operation of moreKeys.
   *
   * @type {MoreKeysShiftOperation}
   */
  this.moreKeysShiftOperation = moreKeysShiftOperation ?
      moreKeysShiftOperation : MoreKeysShiftOperation.TO_UPPER_CASE;

  /**
   * The text when shift is pressed down.
   *
   * @type {string}
   */
  this.textOnShift = textOnShift;

  /**
   * The css class for the text.
   *
   * @type {string}
   */
  this.textCssClass = opt_textCssClass || '';

  /**
   * Map for changing the key based on the current input context.
   *
   * @type {Object.<string, !Object>}
   */
  this.textOnContext = textOnContext || {};

  /**
   * The fixed number of columns when display accent keys in a multi-row popup
   * window.
   *
   * @type {number}
   */
  this.fixedColumns = opt_fixedColumns || 0;

  /**
   * Alternate title for the key. Title is displayed, whereas text is
   * what is actually committed.
   */
  this.title = opt_title || '';
};
});  // goog.scope

