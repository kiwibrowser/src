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
goog.provide('i18n.input.chrome.inputview.elements.content.PageIndicator');

goog.require('goog.dom');
goog.require('goog.dom.TagName');
goog.require('goog.dom.classlist');
goog.require('goog.style');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.Element');

goog.scope(function() {
var ElementType = i18n.input.chrome.ElementType;



/**
 * The indicator of the current page index.
 *
 * @param {string} id The id.
 * @param {!i18n.input.chrome.ElementType} type The element
 *     type.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.Element}
 */
i18n.input.chrome.inputview.elements.content.PageIndicator = function(id, type,
    opt_eventTarget) {
  goog.base(this, id, type, opt_eventTarget);
};
goog.inherits(i18n.input.chrome.inputview.elements.content.PageIndicator,
    i18n.input.chrome.inputview.elements.Element);
var PageIndicator = i18n.input.chrome.inputview.elements.content.PageIndicator;


/** @override */
PageIndicator.prototype.createDom = function() {
  goog.base(this, 'createDom');
  var dom = this.getDomHelper();
  var elem = this.getElement();
  goog.dom.classlist.add(elem,
      i18n.input.chrome.inputview.Css.INDICATOR_BACKGROUND);
  this.bgElem = goog.dom.createDom(goog.dom.TagName.DIV);
  goog.dom.classlist.add(this.bgElem,
      i18n.input.chrome.inputview.Css.INDICATOR);
  dom.appendChild(elem, this.bgElem);
};


/**
 * Slide the indicator.
 *
 * @param {number} deltaX The x-coordinate of slide distance.
 * @param {number} totalPages The total number of pages.
 */
PageIndicator.prototype.slide = function(deltaX, totalPages) {
  var marginLeft = goog.style.getMarginBox(this.bgElem).left +
      deltaX / totalPages;
  this.bgElem.style.marginLeft = marginLeft + 'px';
};


/**
 * Move the indicator to indicate a page.
 *
 * @param {number} pageNum The page that needs to be indicated.
 * @param {number} totalPages The total number of pages.
 */
PageIndicator.prototype.gotoPage = function(pageNum, totalPages) {
  var width = goog.style.getSize(this.getElement()).width;
  this.bgElem.style.marginLeft = width / totalPages * pageNum + 'px';
  if (totalPages >= 2) {
    this.bgElem.style.width = width / totalPages + 'px';
  } else {
    this.bgElem.style.width = 0;
  }
};
});  // goog.scope


