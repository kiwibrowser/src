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
goog.provide('i18n.input.chrome.inputview.elements.layout.SoftKeyView');

goog.require('goog.dom.classlist');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.Element');
goog.require('i18n.input.chrome.inputview.elements.Weightable');



goog.scope(function() {



/**
 * The soft key view.
 *
 * @param {string} id The id.
 * @param {number=} opt_widthInWeight The weight of width.
 * @param {number=} opt_heightInWeight The weight of height.
 * @param {string=} opt_condition The condition to control whether to render
 *     this soft key view.
 * @param {string=} opt_giveWeightTo The id of the soft key view to give the
 *     weight if this one is not shown.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.Element}
 * @implements {i18n.input.chrome.inputview.elements.Weightable}
 */
i18n.input.chrome.inputview.elements.layout.SoftKeyView = function(id,
    opt_widthInWeight, opt_heightInWeight, opt_condition,
    opt_giveWeightTo, opt_eventTarget) {
  goog.base(this, id, i18n.input.chrome.ElementType.
      SOFT_KEY_VIEW, opt_eventTarget);

  /**
   * The weight of the width.
   *
   * @type {number}
   */
  this.widthInWeight = opt_widthInWeight || 1;

  /**
   * The weight of the height.
   *
   * @type {number}
   */
  this.heightInWeight = opt_heightInWeight || 1;

  /**
   * The condition to control the visibility of this soft key view.
   *
   * @type {string}
   */
  this.condition = opt_condition || '';

  /**
   * The id of another soft key view to give the weight if this
   * one is not shown.
   *
   * @type {string}
   */
  this.giveWeightTo = opt_giveWeightTo || '';
};
goog.inherits(i18n.input.chrome.inputview.elements.layout.SoftKeyView,
    i18n.input.chrome.inputview.elements.Element);
var SoftKeyView = i18n.input.chrome.inputview.elements.layout.SoftKeyView;


/**
 * The soft key bound to this view.
 *
 * @type {!i18n.input.chrome.inputview.elements.content.SoftKey}
 */
SoftKeyView.prototype.softKey;


/**
 * The weight granted when certain condition happens and another soft key
 * view is not shown, so its weight transfer here.
 *
 * @type {number}
 */
SoftKeyView.prototype.dynamicaGrantedWeight = 0;


/** @override */
SoftKeyView.prototype.createDom = function() {
  goog.base(this, 'createDom');

  var elem = this.getElement();
  goog.dom.classlist.add(elem, i18n.input.chrome.inputview.Css.SOFT_KEY_VIEW);
};


/** @override */
SoftKeyView.prototype.getWidthInWeight =
    function() {
  return this.isVisible() ? this.widthInWeight + this.dynamicaGrantedWeight : 0;
};


/** @override */
SoftKeyView.prototype.getHeightInWeight =
    function() {
  return this.isVisible() ? this.heightInWeight : 0;
};


/** @override */
SoftKeyView.prototype.resize = function(
    width, height) {
  goog.base(this, 'resize', width, height);

  var elem = this.getElement();
  elem.style.width = width + 'px';
  elem.style.height = height + 'px';
  if (this.softKey) {
    this.softKey.resize(width, height);
  }
};


/**
 * Binds a soft key to this soft key view.
 *
 * @param {!i18n.input.chrome.inputview.elements.content.SoftKey} softKey The
 *     soft key.
 */
SoftKeyView.prototype.bindSoftKey = function(
    softKey) {
  this.softKey = softKey;
  this.removeChildren(true);
  this.addChild(softKey, true);
};

});  // goog.scope
