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
goog.provide('i18n.input.chrome.inputview.elements.layout.LinearLayout');

goog.require('goog.dom.classlist');
goog.require('goog.style');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.Element');
goog.require('i18n.input.chrome.inputview.elements.Weightable');
goog.require('i18n.input.chrome.inputview.util');


goog.scope(function() {



/**
 * The linear layout.
 *
 * @param {string} id The id.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @param {i18n.input.chrome.inputview.Css=} opt_iconCssClass The css class.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.Element}
 * @implements {i18n.input.chrome.inputview.elements.Weightable}
 */
i18n.input.chrome.inputview.elements.layout.LinearLayout = function(id,
    opt_eventTarget, opt_iconCssClass) {
  goog.base(this, id, i18n.input.chrome.ElementType.
      LINEAR_LAYOUT, opt_eventTarget);

  /**
   * The icon Css class for the linearlayout
   *
   * @type {i18n.input.chrome.inputview.Css}
   */
  this.iconCssClass = opt_iconCssClass ||
      i18n.input.chrome.inputview.Css.LINEAR_LAYOUT;
};
goog.inherits(i18n.input.chrome.inputview.elements.layout.LinearLayout,
    i18n.input.chrome.inputview.elements.Element);
var LinearLayout = i18n.input.chrome.inputview.elements.layout.LinearLayout;


/**
 * The height in weight unit.
 *
 * @type {number}
 * @private
 */
LinearLayout.prototype.heightInWeight_ = 0;


/**
 * The width in weight unit.
 *
 * @type {number}
 * @private
 */
LinearLayout.prototype.widthInWeight_ = 0;


/** @override */
LinearLayout.prototype.createDom = function() {
  goog.base(this, 'createDom');
  goog.dom.classlist.add(this.getElement(), this.iconCssClass);
  this.getElement()['view'] = null;
};


/** @override */
LinearLayout.prototype.enterDocument = function() {
  goog.base(this, 'enterDocument');

  this.calculate_();
};


/**
 * Calculate all necessary information after enters the document.
 *
 * @private
 */
LinearLayout.prototype.calculate_ = function() {
  for (var i = 0; i < this.getChildCount(); i++) {
    var child = /** @type {i18n.input.chrome.inputview.elements.Weightable} */ (
        this.getChildAt(i));
    if (this.heightInWeight_ < child.getHeightInWeight()) {
      this.heightInWeight_ = child.getHeightInWeight();
    }
    this.widthInWeight_ += child.getWidthInWeight();
  }
};


/** @override */
LinearLayout.prototype.getHeightInWeight = function() {
  return this.heightInWeight_;
};


/** @override */
LinearLayout.prototype.getWidthInWeight = function() {
  return this.widthInWeight_;
};


/** @override */
LinearLayout.prototype.resize = function(width, height) {
  goog.base(this, 'resize', width, height);

  var elem = this.getElement();
  var borderBox = goog.style.getBorderBox(elem);
  var paddingBox = goog.style.getPaddingBox(elem);
  var marginBox = goog.style.getMarginBox(elem);
  var w = width - borderBox.left - borderBox.right - paddingBox.left -
      paddingBox.right - marginBox.left - marginBox.right;
  var weightArray = [];
  for (var i = 0; i < this.getChildCount(); i++) {
    var child = /** @type {i18n.input.chrome.inputview.elements.Weightable} */ (
        this.getChildAt(i));
    weightArray.push(child.getWidthInWeight());
  }
  var splitedWidth = i18n.input.chrome.inputview.util.splitValue(weightArray,
      w);
  for (var i = 0; i < this.getChildCount(); i++) {
    var child = /** @type {i18n.input.chrome.inputview.elements.Element} */ (
        this.getChildAt(i));
    child.resize(splitedWidth[i], height);
  }
};

});  // goog.scope
