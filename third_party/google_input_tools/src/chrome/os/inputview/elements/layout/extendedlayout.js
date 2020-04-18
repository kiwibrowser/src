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
goog.provide('i18n.input.chrome.inputview.elements.layout.ExtendedLayout');

goog.require('goog.dom.classlist');
goog.require('goog.style');
goog.require('goog.style.transform');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.Element');
goog.require('i18n.input.chrome.inputview.elements.Weightable');


goog.scope(function() {
var Css = i18n.input.chrome.inputview.Css;



/**
 * The extended layout. Each of its children has the same width as its parent.
 * It can be wider than its parent.
 *
 * @param {string} id The id.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @param {i18n.input.chrome.inputview.Css=} opt_iconCssClass The css class.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.Element}
 * @implements {i18n.input.chrome.inputview.elements.Weightable}
 */
i18n.input.chrome.inputview.elements.layout.ExtendedLayout = function(id,
    opt_eventTarget, opt_iconCssClass) {
  goog.base(this, id, i18n.input.chrome.ElementType.
      EXTENDED_LAYOUT, opt_eventTarget);

  /**
   * The icon Css class for the extendedlayout.
   *
   * @type {i18n.input.chrome.inputview.Css}
   */
  this.iconCssClass = Css.LINEAR_LAYOUT;
};
goog.inherits(i18n.input.chrome.inputview.elements.layout.ExtendedLayout,
    i18n.input.chrome.inputview.elements.Element);
var ExtendedLayout = i18n.input.chrome.inputview.elements.layout.ExtendedLayout;


/**
 * The height in weight unit.
 *
 * @type {number}
 * @private
 */
ExtendedLayout.prototype.heightInWeight_ = 0;


/**
 * The width in weight unit.
 *
 * @type {number}
 * @private
 */
ExtendedLayout.prototype.widthInWeight_ = 0;


/**
 * The current X (horizontal) position of the layout.
 *
 * @type {number}
 * @private
 */
ExtendedLayout.prototype.xPosition_ = 0;


/**
 * The time of transition for every transition distance.
 *
 * @private {number}
 */
ExtendedLayout.BASE_TRANSITION_DURATION_ = 0.2;


/**
 * The transition distance used to calculate transition time.
 *
 * @private {number}
 */
ExtendedLayout.BASE_TRANSITION_DISTANCE_ = 100;


/** @override */
ExtendedLayout.prototype.getHeightInWeight = function() {
  return this.heightInWeight_;
};


/** @override */
ExtendedLayout.prototype.getWidthInWeight = function() {
  return this.widthInWeight_;
};


/** @override */
ExtendedLayout.prototype.createDom = function() {
  goog.base(this, 'createDom');
  this.elem = this.getElement();
  goog.dom.classlist.addAll(this.elem,
      [this.iconCssClass, Css.EMOJI_FONT]);
};


/** @override */
ExtendedLayout.prototype.enterDocument = function() {
  goog.base(this, 'enterDocument');
  this.calculate_();
};


/** @override */
ExtendedLayout.prototype.resize = function(width, height) {
  if (width == this.width && height == this.height) {
    return;
  }
  for (var i = 0, len = this.getChildCount(); i < len; i++) {
    var child =  /** @type {i18n.input.chrome.inputview.elements.Element} */ (
        this.getChildAt(i));
    child.resize(width, height);
  }
  this.getElement().style.width = width * this.getChildCount() + 'px';
  goog.base(this, 'resize', width * this.getChildCount(), height);
};


/**
 * Calculate the height and weight。
 *
 * @private
 */
ExtendedLayout.prototype.calculate_ = function() {
  for (var i = 0; i < this.getChildCount(); i++) {
    var child = /** @type {i18n.input.chrome.inputview.elements.Weightable} */ (
        this.getChildAt(i));
    if (this.heightInWeight_ < child.getHeightInWeight()) {
      this.heightInWeight_ = child.getHeightInWeight();
    }
    this.widthInWeight_ = child.getWidthInWeight();
  }
};


/**
 * Switch to a page of the emojiSlider.
 *
 * @param {number} pageNum The page to switch to.
 */
ExtendedLayout.prototype.gotoPage = function(pageNum) {
  var width = goog.style.getSize(this.getElement()).width;
  var childNum = this.getChildCount();
  this.xPosition_ = 0 - width / childNum * pageNum;
  goog.style.transform.setTranslation(this.elem, this.xPosition_, 0);
};


/**
 * Slide to a position.
 *
 * @param {number} deltaX The slide distance of x-coordinate.
 */
ExtendedLayout.prototype.slide = function(deltaX) {
  this.elem.style.transition = '';
  this.xPosition_ += deltaX;
  goog.style.transform.setTranslation(this.elem, this.xPosition_, 0);
};


/**
 * Adjust the the horizontal position of the layout to align with a page.
 *
 * @param {number=} opt_distance The distance to adjust to.
 * @return {number} The page to adjust to after calculation.
 */
ExtendedLayout.prototype.adjustXPosition = function(opt_distance) {
  var childNum = this.getChildCount();
  var width = goog.style.getSize(this.elem).width / childNum;
  var absXPosition = Math.abs(this.xPosition_);
  var prev = Math.floor(absXPosition / width);
  var next = prev + 1;
  var pageNum = 0;
  if (opt_distance) {
    if (opt_distance >= 0) {
      pageNum = prev;
    } else {
      pageNum = next;
    }
  } else if (absXPosition - prev * width < next * width - absXPosition) {
    pageNum = prev;
  } else {
    pageNum = next;
  }
  if (pageNum < 0) {
    pageNum = 0;
  } else if (pageNum >= childNum) {
    pageNum = childNum - 1;
  }
  if (opt_distance) {
    this.elem.style.transition = 'transform ' +
        ExtendedLayout.BASE_TRANSITION_DURATION_ + 's';
  } else {
    var transitionDuration = Math.abs(absXPosition - pageNum * width) /
        ExtendedLayout.BASE_TRANSITION_DISTANCE_ *
        ExtendedLayout.BASE_TRANSITION_DURATION_;
    this.elem.style.transition = 'transform ' +
        transitionDuration + 's ease-in';
  }
  this.gotoPage(pageNum);
  return pageNum;
};


/**
 * Update the category.
 *
 * @param {number} pageNum The page number to switch to.
 */
ExtendedLayout.prototype.updateCategory = function(pageNum) {
  this.elem.style.transition = '';
  this.gotoPage(pageNum);
};
});  // goog.scope
