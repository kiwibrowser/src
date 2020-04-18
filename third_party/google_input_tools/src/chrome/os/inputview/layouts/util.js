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
goog.provide('i18n.input.chrome.inputview.layouts.util');
goog.require('i18n.input.chrome.ElementType');



goog.scope(function() {
var ElementType = i18n.input.chrome.ElementType;
var util = i18n.input.chrome.inputview.layouts.util;


/**
 * The id for the key.
 *
 * @type {number}
 * @private
 */
util.keyId_ = 0;


/**
 * The prefix of the key id, it's overwritten by layout file.
 *
 * @type {string}
 */
util.keyIdPrefix = '';


/**
 * Resets id counter for keys in preparation for processing a new layout.
 * @param {string} prefix  The prefix for the key id.
 */
util.setPrefix = function(prefix) {
  util.keyIdPrefix = prefix;
  util.keyId_ = 0;
};


/**
 * Creates a sequence of key with the same specification.
 *
 * @param {!Object} spec The specification.
 * @param {number} num How many keys.
 * @return {!Array.<Object>} The keys.
 */
util.createKeySequence = function(spec,
    num) {
  var sequence = [];
  for (var i = 0; i < num; i++) {
    sequence.push(util.createKey(spec));
  }
  return sequence;
};


/**
 * Creates a soft key view.
 *
 * @param {!Object} spec The specification.
 * @param {string=} opt_id The id.
 * @return {!Object} The soft key view.
 */
util.createKey = function(spec, opt_id) {
  var id = util.keyIdPrefix +
      util.keyId_++;
  return util.createElem(
      ElementType.SOFT_KEY_VIEW, spec, id);
};


/**
 * Creates a linear layout.
 *
 * @param {!Object} spec The specification.
 * @param {string=} opt_id The id.
 * @return {!Object} The linear layout.
 */
util.createLinearLayout = function(spec,
    opt_id) {
  return util.createElem(
      ElementType.LINEAR_LAYOUT, spec, opt_id, spec['iconCssClass']);
};


/**
 * Creates an extended layout.
 *
 * @param {!Object} spec The specification.
 * @param {string=} opt_id The id.
 * @return {!Object} The extended layout.
 */
util.createExtendedLayout = function(spec,
    opt_id) {
  return util.createElem(
      ElementType.EXTENDED_LAYOUT, spec, opt_id, spec['iconCssClass']);
};


/**
 * Creates a handwriting layout.
 *
 * @param {!Object} spec The specification.
 * @param {string=} opt_id The id.
 * @return {!Object} The handwriting layout.
 */
util.createHandwritingLayout =
    function(spec, opt_id) {
  return util.createElem(
      ElementType.HANDWRITING_LAYOUT, spec, opt_id);
};


/**
 * Creates a vertical layout.
 *
 * @param {!Object} spec The specification.
 * @param {string=} opt_id The id.
 * @return {!Object} The vertical layout.
 */
util.createVerticalLayout = function(spec,
    opt_id) {
  return util.createElem(
      ElementType.VERTICAL_LAYOUT, spec, opt_id);
};


/**
 * Creates a layout view.
 *
 * @param {!Object} spec The specification.
 * @param {string=} opt_id The id.
 * @return {!Object} The view.
 */
util.createLayoutView = function(spec,
    opt_id) {
  return util.createElem(
      ElementType.LAYOUT_VIEW, spec, opt_id);
};


/**
 * Creates a candidate view.
 *
 * @param {!Object} spec The specification.
 * @param {string=} opt_id The id.
 * @return {!Object} The view.
 */
util.createCandidateView = function(spec,
    opt_id) {
  return util.createElem(
      ElementType.CANDIDATE_VIEW, spec, opt_id);
};


/**
 * Creates a canvas view.
 *
 * @param {!Object} spec The specification.
 * @param {string=} opt_id The id.
 * @return {!Object} The view.
 */
util.createCanvasView = function(spec,
    opt_id) {
  return util.createElem(
      ElementType.CANVAS_VIEW, spec, opt_id);
};


/**
 * Creates the keyboard.
 *
 * @param {Object} spec The specification.
 * @param {string=} opt_id The id.
 * @return {Object} The keyboard.
 */
util.createKeyboard = function(spec,
    opt_id) {
  return util.createElem(
      ElementType.KEYBOARD, spec, opt_id);
};


/**
 * Creates an element which could be any type, such as soft key view, layout,
 *     etc.
 *
 * @param {!ElementType} type The type.
 * @param {Object} spec The specification.
 * @param {string=} opt_id The id.
 * @param {string=} opt_iconCssClass The Css class.
 * @return {!Object} The element.
 */
util.createElem = function(type, spec,
    opt_id, opt_iconCssClass) {
  var newSpec = {};
  for (var key in spec) {
    newSpec[key] = spec[key];
  }
  newSpec['type'] = type;
  if (opt_id) {
    newSpec['id'] = opt_id;
  }
  if (opt_iconCssClass) {
    newSpec['iconCssClass'] = opt_iconCssClass;
  }
  return {
    'spec': newSpec
  };
};

});  // goog.scope
