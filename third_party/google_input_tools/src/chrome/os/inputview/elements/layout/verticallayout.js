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
goog.provide('i18n.input.chrome.inputview.elements.layout.VerticalLayout');

goog.require('goog.dom.classlist');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.Element');
goog.require('i18n.input.chrome.inputview.elements.Weightable');
goog.require('i18n.input.chrome.inputview.util');


goog.scope(function() {



/**
 * The vertical layout.
 *
 * @param {string} id The id.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @param {!i18n.input.chrome.ElementType=} opt_type The sub
 *     type.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.Element}
 * @implements {i18n.input.chrome.inputview.elements.Weightable}
 */
i18n.input.chrome.inputview.elements.layout.VerticalLayout = function(id,
    opt_eventTarget, opt_type) {
  var type = opt_type || i18n.input.chrome.ElementType.
      VERTICAL_LAYOUT;
  goog.base(this, id, type, opt_eventTarget);
};
goog.inherits(i18n.input.chrome.inputview.elements.layout.VerticalLayout,
    i18n.input.chrome.inputview.elements.Element);
var VerticalLayout = i18n.input.chrome.inputview.elements.layout.VerticalLayout;


/**
 * The height in weight unit.
 *
 * @type {number}
 * @private
 */
VerticalLayout.prototype.heightInWeight_ = 0;


/**
 * The width in weight unit.
 *
 * @type {number}
 * @private
 */
VerticalLayout.prototype.widthInWeight_ = 0;


/** @override */
VerticalLayout.prototype.createDom = function() {
  goog.base(this, 'createDom');

  goog.dom.classlist.add(this.getElement(),
      i18n.input.chrome.inputview.Css.VERTICAL_LAYOUT);

  this.getElement()['view'] = null;
};


/** @override */
VerticalLayout.prototype.enterDocument =
    function() {
  goog.base(this, 'enterDocument');

  this.calculate_();
};


/**
 * Calculate all necessary information after enters the document.
 *
 * @private
 */
VerticalLayout.prototype.calculate_ = function() {
  for (var i = 0; i < this.getChildCount(); i++) {
    var child = /** @type {i18n.input.chrome.inputview.elements.Weightable} */ (
        this.getChildAt(i));
    if (this.widthInWeight_ < child.getWidthInWeight()) {
      this.widthInWeight_ = child.getWidthInWeight();
    }
    this.heightInWeight_ += child.getHeightInWeight();
  }
};


/** @override */
VerticalLayout.prototype.getHeightInWeight = function() {
  return this.heightInWeight_;
};


/** @override */
VerticalLayout.prototype.getWidthInWeight = function() {
  return this.widthInWeight_;
};


/** @override */
VerticalLayout.prototype.resize = function(
    width, height) {
  goog.base(this, 'resize', width, height);

  this.getElement().style.width = width + 'px';
  var weightArray = [];
  for (var i = 0; i < this.getChildCount(); i++) {
    var child = /** @type {i18n.input.chrome.inputview.elements.Weightable} */ (
        this.getChildAt(i));
    weightArray.push(child.getHeightInWeight());
  }
  var splitedHeight = i18n.input.chrome.inputview.util.splitValue(weightArray,
      height);
  for (var i = 0; i < this.getChildCount(); i++) {
    var child = /** @type {i18n.input.chrome.inputview.elements.Element} */ (
        this.getChildAt(i));
    child.resize(width, splitedHeight[i]);
  }
};

});  // goog.scope
