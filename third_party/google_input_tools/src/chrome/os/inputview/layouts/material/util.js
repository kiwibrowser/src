// Copyright 2015 The ChromeOS IME Authors. All Rights Reserved.
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
goog.provide('i18n.input.chrome.inputview.layouts.material.util');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.ElementType');



goog.scope(function() {
var ElementType = i18n.input.chrome.inputview.elements.ElementType;
var material = i18n.input.chrome.inputview.layouts.material;


/**
 * The id for the key.
 *
 * @type {number}
 * @private
 */
material.util.keyId_ = 0;


/**
 * The prefix of the key id, it's overwritten by layout file.
 *
 * @type {string}
 */
material.util.keyIdPrefix = '';


/**
 * Resets id counter for keys in preparation for processing a new layout.
 * @param {string} prefix  The prefix for the key id.
 */
material.util.setPrefix = function(prefix) {
  material.util.keyIdPrefix = prefix;
  material.util.keyId_ = 0;
};


/**
 * Creates a sequence of key with the same specification.
 *
 * @param {!Object} spec The specification.
 * @param {number} num How many keys.
 * @return {!Array.<Object>} The keys.
 */
material.util.createKeySequence = function(spec,
    num) {
  var sequence = [];
  for (var i = 0; i < num; i++) {
    sequence.push(material.util.createKey(spec));
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
material.util.createKey = function(spec, opt_id) {
  var id = material.util.keyIdPrefix +
      material.util.keyId_++;
  return material.util.createElem(
      ElementType.SOFT_KEY_VIEW, spec, id);
};


/**
 * Creates a linear layout.
 *
 * @param {!Object} spec The specification.
 * @param {string=} opt_id The id.
 * @return {!Object} The linear layout.
 */
material.util.createLinearLayout = function(spec,
    opt_id) {
  return material.util.createElem(
      ElementType.LINEAR_LAYOUT, spec, opt_id, spec['iconCssClass']);
};


/**
 * Creates an extended layout.
 *
 * @param {!Object} spec The specification.
 * @param {string=} opt_id The id.
 * @return {!Object} The extended layout.
 */
material.util.createExtendedLayout = function(spec,
    opt_id) {
  return material.util.createElem(
      ElementType.EXTENDED_LAYOUT, spec, opt_id, spec['iconCssClass']);
};


/**
 * Creates a handwriting layout.
 *
 * @param {!Object} spec The specification.
 * @param {string=} opt_id The id.
 * @return {!Object} The handwriting layout.
 */
material.util.createHandwritingLayout =
    function(spec, opt_id) {
  return material.util.createElem(
      ElementType.HANDWRITING_LAYOUT, spec, opt_id);
};


/**
 * Creates a vertical layout.
 *
 * @param {!Object} spec The specification.
 * @param {string=} opt_id The id.
 * @return {!Object} The vertical layout.
 */
material.util.createVerticalLayout = function(spec,
    opt_id) {
  return material.util.createElem(
      ElementType.VERTICAL_LAYOUT, spec, opt_id);
};


/**
 * Creates a layout view.
 *
 * @param {!Object} spec The specification.
 * @param {string=} opt_id The id.
 * @return {!Object} The view.
 */
material.util.createLayoutView = function(spec,
    opt_id) {
  return material.util.createElem(
      ElementType.LAYOUT_VIEW, spec, opt_id);
};


/**
 * Creates a candidate view.
 *
 * @param {!Object} spec The specification.
 * @param {string=} opt_id The id.
 * @return {!Object} The view.
 */
material.util.createCandidateView = function(spec,
    opt_id) {
  return material.util.createElem(
      ElementType.CANDIDATE_VIEW, spec, opt_id);
};


/**
 * Creates a canvas view.
 *
 * @param {!Object} spec The specification.
 * @param {string=} opt_id The id.
 * @return {!Object} The view.
 */
material.util.createCanvasView = function(spec,
    opt_id) {
  return material.util.createElem(
      ElementType.CANVAS_VIEW, spec, opt_id);
};


/**
 * Creates the keyboard.
 *
 * @param {Object} spec The specification.
 * @param {string=} opt_id The id.
 * @return {Object} The keyboard.
 */
material.util.createKeyboard = function(spec,
    opt_id) {
  return material.util.createElem(
      ElementType.KEYBOARD, spec, opt_id);
};


/**
 * Creates an element which could be any type, such as soft key view, layout,
 *     etc.
 *
 * @param {!ElementType} type The type.
 * @param {Object} spec The specification.
 * @param {string=} opt_id The id.
 * @param {i18n.input.chrome.inputview.Css=} opt_iconCssClass The Css class.
 * @return {!Object} The element.
 */
material.util.createElem = function(type, spec,
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
